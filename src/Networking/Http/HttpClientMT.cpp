#include "HttpClientMT.h"

#include "TcpException.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../../Core/Log.h"
#include "../../Core/Format.h"
#include "../StringResources.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <list>
#include <mutex>
#include <unordered_map>

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
using ssl_socket = asio::ssl::stream<tcp::socket>;

typedef std::shared_ptr<asio::ip::tcp::resolver> ResolverPtr;
typedef std::function<void()> VoidCallback;
typedef asio::basic_socket<tcp, tcp::socket::lowest_layer_type::executor_type> BasicSocket;

#define THROW_IF_ERROR(sessionId, error, msg) if (error) { \
    if (error == asio::error::operation_aborted) { \
        throw TcpOperationException(sessionId, Http::OperationCanceled, true); \
    } else { \
        throw TcpOperationException(sessionId, Format("%s: %s", msg, error.message().c_str()), false); \
    } \
} \

#define THROW_IF(cond, sessionId, msg) if (cond) { \
    std::ostringstream _ost; \
    _ost << msg; \
    throw TcpOperationException(sessionId, _ost.str(), false); \
}

template<Protocol Proto>
struct ProtoTraits {
    typedef tcp::socket SocketType;
    typedef std::shared_ptr<SocketType> SocketPtr;

    static std::unique_ptr<asio::ssl::context> CreateSslContext() {
        return nullptr;
    }
    static BasicSocket& GetTcpSocket(SocketPtr sock) {
        return *sock;
    }
    static void CloseSocket(SocketPtr sock) {
        error_code error;
        sock->shutdown(tcp::socket::shutdown_both, error);
        sock->close(error);
    }
};

template<>
struct ProtoTraits<Protocol::Https> {
    typedef ssl_socket SocketType;
    typedef std::shared_ptr<SocketType> SocketPtr;

    static std::unique_ptr<asio::ssl::context> CreateSslContext() {
        return std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23);
    }
    static BasicSocket& GetTcpSocket(SocketPtr sock) {
        return sock->lowest_layer();
    }
    static void CloseSocket(SocketPtr sock) {
        error_code error;
        sock->shutdown(error);
        GetTcpSocket(sock).shutdown(tcp::socket::shutdown_both, error);
        GetTcpSocket(sock).close(error);
    }
};

struct HttpSessionBase {
    HttpSessionBase(HttpRequest&& request, HttpRequestCallback callback, int retry)
        : Request(std::move(request))
        , Callback(callback)
        , Retry(retry)
    {
    }
    HttpRequest Request;
    HttpRequestCallback Callback;
    int Retry;
    tcp::resolver::results_type ResolveResult;
    VoidCallback Cancel;
    ResponsePtr Response;
    asio::streambuf ReadBuffer;
    asio::streambuf WriteBuffer;

    virtual ~HttpSessionBase() = default;
};
typedef std::shared_ptr<HttpSessionBase> HttpSessionBasePtr;

template<Protocol Proto>
struct HttpSession : public HttpSessionBase
{
    typedef typename ProtoTraits<Proto>::SocketType SocketType;
    typedef std::shared_ptr<SocketType> SocketPtr;
    HttpSession(HttpRequest&& request, HttpRequestCallback callback, int retry = 1)
        : HttpSessionBase(std::move(request), callback, retry)
    {
    }
    virtual ~HttpSession() override {
        if (Socket) {
            ProtoTraits<Proto>::CloseSocket(Socket);
        }
    }
    SocketPtr Socket;
};

class TcpOperationException : public TcpException {
public:
    TcpOperationException(RequestIdType sessionId, std::string const& whatArg, bool wasCancel)
        : TcpException(whatArg, wasCancel)
        , m_sessionId(sessionId) {
    }

    TcpOperationException(RequestIdType sessionId, char const* whatArg, bool wasCancel)
        : TcpException(whatArg, wasCancel)
        , m_sessionId(sessionId) {
    }
    
    RequestIdType GetSessionId() const {
        return m_sessionId;
    }
private:
    RequestIdType m_sessionId;
};

class HttpClientMTImplBase {
public:
    HttpClientMTImplBase(std::string const& serverName, int port, std::size_t threadCount)
        : m_serverName(serverName)
        , m_port(port)
        , m_closing(false)
        , m_threadCount(threadCount)
    {
    }

    HttpRequest CreateRequest(HttpMethod method, std::string const& path, ParamList const& params) {
        HttpRequest result(method, path, params);
        result.AddHeader("Host", m_serverName)
            .AddHeader("Connection", "keep-alive")
            .AddHeader("Accept", "*/*");
        return result;
    }

    virtual ~HttpClientMTImplBase() = default;
    virtual void PostRequest(HttpRequest&& request, HttpRequestCallback callback) = 0;
    virtual void CancelRequest(RequestIdType id) = 0;

protected:
    static std::string const& GetRequestPathCRef(HttpSessionBasePtr session) {
        return session->Request.m_path;
    }
    static std::string& GetResponseHttpVersionRef(HttpSessionBasePtr session) {
        return session->Response->m_httpVersion;
    }
    static std::uint32_t& GetResponseStatusCodeRef(HttpSessionBasePtr session) {
        return session->Response->m_statusCode;
    }
    static std::string& GetResponseStatusMessageRef(HttpSessionBasePtr session) {
        return session->Response->m_statusMessage;
    }
    static ParamMap& GetResponseHeadersRef(HttpSessionBasePtr session) {
        return session->Response->m_headers;
    }
    static std::stringstream& GetResponseContentRef(HttpSessionBasePtr session) {
        return session->Response->m_content;
    }
    static ResponsePtr CreateResponse() {
        ResponsePtr result(new HttpResponse());
        return result;
    }

    static bool IsValidHttpVersion(std::string const& httpVersion) {
        return httpVersion == HttpVersion_1_1 || httpVersion == HttpVersion_1_0;
    }

protected:
    std::string const m_serverName;
    int const m_port;
    bool m_closing;
    std::size_t const m_threadCount;
    std::unique_ptr<asio::ssl::context> m_sslContext;
    asio::io_service m_ios;
    std::unique_ptr<asio::io_service::work> m_worker;
    std::list<std::unique_ptr<std::thread>> m_threads;
};

template<Protocol Proto>
class Impl : public HttpClientMTImplBase {
public:
    typedef typename ProtoTraits<Proto>::SocketType SocketType;
    typedef std::shared_ptr<SocketType> SocketPtr;
    typedef HttpSession<Proto> HttpSessionType;
    typedef std::shared_ptr<HttpSessionType> HttpSessionPtr;

    Impl(std::string const& serverName, int port, std::size_t threadCount)
        : HttpClientMTImplBase(serverName, port, threadCount)
    {
        m_sslContext = ProtoTraits<Proto>::CreateSslContext();
        m_worker.reset(new asio::io_service::work(m_ios));
        for (std::size_t i(0); i < threadCount; ++i) {
            m_threads.push_back(std::make_unique<std::thread>(&Impl<Proto>::ThreadLoop, this));
        }
    }

    virtual ~Impl() override {
        m_closing = true;
        {
            std::lock_guard<std::mutex> lock(m_activeSessionsMutex);
            for (auto& pair : m_activeSessions) {
                HttpSessionPtr session = pair.second;
                session->Cancel();
            }
            m_activeSessions.clear();
        }
        {
            std::lock_guard<std::mutex> lockSockets(m_socketMutex);
            for (SocketPtr sock : m_socketPool) {
                ProtoTraits<Proto>::CloseSocket(sock);
            }
            m_socketPool.clear();
        }
        m_worker.reset();
        for (auto& threadPtr : m_threads) {
            threadPtr->join();
        }
    }

    virtual void CancelRequest(RequestIdType id) override {
        std::lock_guard<std::mutex> lock(m_activeSessionsMutex);
        auto found = m_activeSessions.find(id);
        if (found != m_activeSessions.end()) {
            HttpSessionPtr session = found->second;
            if (session->Cancel) {
                session->Cancel();
            }
        }
    }

    virtual void PostRequest(HttpRequest&& request, HttpRequestCallback callback) override {
        HttpSessionPtr session(new HttpSessionType(std::move(request), callback));
        m_ios.post(std::bind(&Impl<Proto>::StartSession, this, session));
    }

private:
    SocketPtr CreateSocket() {
        return SocketPtr(new SocketType(m_ios));
    }

    SocketPtr GetCachedSocket() {
        SocketPtr result;
        std::lock_guard<std::mutex> lock(m_socketMutex);
        if (!m_socketPool.empty()) {
            result = m_socketPool.front();
            m_socketPool.pop_front();
        }
        return result;
    }

    void CacheSocket(SocketPtr sock) {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        int const cntToRemove = static_cast<int>(m_socketPool.size()) - static_cast<int>(m_threadCount) + 1;
        for (int i(0); i < cntToRemove; ++i) {
            SocketPtr socket = m_socketPool.front();
            m_socketPool.pop_front();
            ProtoTraits<Proto>::CloseSocket(socket);
        }
        m_socketPool.push_back(sock);
    }

    void ThreadLoop() {
        Log("Start ios thread: %d\n", std::this_thread::get_id());
        while(!m_closing) {
            try {
                m_ios.run();
            }
            catch(TcpOperationException& ex) {
                HttpSessionPtr session;
                {
                    std::lock_guard<std::mutex> lock(m_activeSessionsMutex);
                    auto found = m_activeSessions.find(ex.GetSessionId());
                    if (found != m_activeSessions.end()) {
                        session = found->second;
                        session->Cancel = VoidCallback();
                        m_activeSessions.erase(found);
                    }
                }
                if (session) {
                    if (!ex.WasCancel() && session->Retry) {
                        Log("Retry request\n");
                        HttpSessionPtr retrySession(new HttpSessionType(std::move(session->Request),
                            session->Callback, --session->Retry));
                        m_ios.post(std::bind(&Impl<Proto>::StartSession, this, retrySession));
                    }
                    else {
                        session->Callback(nullptr, std::current_exception());
                    }
                }
                else {
                    Log("Can't find active session id:%d\n", ex.GetSessionId());
                }
            }
            catch(std::exception& ex) {
                Log(ex.what());
            }
        }
        Log("Stop ios thread: %d\n", std::this_thread::get_id());
    }

    void StartSession(HttpSessionPtr session) {
        std::lock_guard<std::mutex> lock(m_activeSessionsMutex);
        m_activeSessions.insert({session->Request.GetId(), session});
        session->Socket = GetCachedSocket();
        if (session->Socket) {
            session->Cancel = std::bind([](SocketPtr sock){
                    ProtoTraits<Proto>::GetTcpSocket(sock).cancel();
                }, session->Socket);
            SendRequest(session);
        }
        else {
            Connect(session);
        }
    }

    void Connect(HttpSessionPtr session) {
        if (session->ResolveResult.empty()) {
            Log("Resolving host %s[port:%d]...\n", m_serverName.c_str(), m_port);
            ResolverPtr resolver(new tcp::resolver(m_ios));
            session->Cancel = std::bind([](ResolverPtr res){ res->cancel(); }, resolver);
            resolver->async_resolve(m_serverName, std::to_string(m_port), tcp::resolver::resolver_base::numeric_service,
                boost::bind(&Impl<Proto>::HandleResolve, this, session, resolver, asio::placeholders::error,
                asio::placeholders::results));
        }
        else {
            Log("Connecting...\n");
            session->Socket = CreateSocket();
            session->Cancel = std::bind([](SocketPtr sock){
                    ProtoTraits<Proto>::GetTcpSocket(sock).cancel();
                }, session->Socket);
            asio::async_connect(ProtoTraits<Proto>::GetTcpSocket(session->Socket), session->ResolveResult,
                boost::bind(&Impl<Proto>::HandleConnect, this, session, asio::placeholders::error, asio::placeholders::endpoint));
        }
    }

    void HandleResolve(HttpSessionPtr session, ResolverPtr resolver, error_code const& error,
                       tcp::resolver::results_type results) {
        THROW_IF_ERROR(session->Request.GetId(), error, Format(Http::ResolveFailed, m_serverName.c_str()).c_str());
        session->ResolveResult = results;
        session->Cancel = VoidCallback();
        Connect(session);
    }

    void HandleConnect(HttpSessionPtr session, error_code const& error, tcp::endpoint const& ep) {
        THROW_IF_ERROR(session->Request.GetId(), error, Format(Http::ConnectionFailed, m_serverName.c_str(),
            ep.address().to_string().c_str(), ep.port()).c_str());
        HandShake(session);
    }

    void HandShake(HttpSessionPtr session) {
        HandleHandShake(session, error_code());
    }

    void HandleHandShake(HttpSessionPtr session, error_code const& error) {
        THROW_IF_ERROR(session->Request.GetId(), error, Http::SslHandShakeFailed);
        SendRequest(session);
    }

    void SendRequest(HttpSessionPtr session) {
        std::ostringstream osst;
        osst << session->Request;
        Log("Request:\n%s\n", osst.str().c_str());
        std::ostream ost(&session->WriteBuffer);
        ost << session->Request;
        asio::async_write(*session->Socket, session->WriteBuffer, boost::bind(&Impl<Proto>::HandleSendRequest,
            this, session, asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void HandleSendRequest(HttpSessionPtr session, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(session->Request.GetId(), error, Http::SendRequestFailed);
        asio::async_read_until(*session->Socket, session->ReadBuffer, "\r\n",
            boost::bind(&Impl<Proto>::HandleReadStatusLine, this, session,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void HandleReadStatusLine(HttpSessionPtr session, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(session->Request.GetId(), error, Http::ReadResponseFailed);
        std::istream ist(&session->ReadBuffer);
        std::string httpVersion;
        bool debug = false;
        ist >> httpVersion;
        if (!IsValidHttpVersion(httpVersion)) {
            std::ostringstream ost(httpVersion);
            ost << " " << &session->ReadBuffer;
            Log("%s\n", ost.str().c_str());
        }
        THROW_IF(!IsValidHttpVersion(httpVersion), session->Request.GetId(),
            Format(Http::ResponseInvalidHttpVersion, httpVersion.c_str()).c_str());
        session->Response = CreateResponse();
        GetResponseHttpVersionRef(session) = httpVersion;
        ist >> GetResponseStatusCodeRef(session);
        ist.get();
        THROW_IF(!ist, session->Request.GetId(), Http::ResponseInvalidStatusCode);
        std::getline(ist, GetResponseStatusMessageRef(session), '\r');
        THROW_IF(ist.get() != '\n', session->Request.GetId(), Http::ResponseInvalidStatusMsg);
        asio::async_read_until(*session->Socket, session->ReadBuffer, "\r\n\r\n",
            boost::bind(&Impl<Proto>::HandleReadHeaders, this, session, asio::placeholders::error,
            asio::placeholders::bytes_transferred));
    }

    void HandleReadHeaders(HttpSessionPtr session, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(session->Request.GetId(), error, Http::ResponseHeadersReadFailed);
        std::string header, headerName, headerValue;
        std::istream ist(&session->ReadBuffer);
        ParamMap& responseHeaders = GetResponseHeadersRef(session);
        while(true) {
            std::getline(ist, header, '\r');
            THROW_IF(ist.get() != '\n', session->Request.GetId(), Http::ResponseInvalidHeader);
            if (header.empty()) break;
            std::size_t pos = header.find(':');
            if (pos != std::string::npos) {
                headerName = header.substr(0, pos);
                pos = header.find_first_not_of(' ', pos+1);
                if (pos < header.length() - 1) {
                    headerValue = header.substr(pos);
                }
                else {
                    headerValue = "";
                }
                responseHeaders[headerName] = headerValue;
            }
        }
        std::size_t const contentLength = session->Response->GetContentLength();
        //Log("HandleReadHeaders contentLength:%d readBuffer.size:%d\n", contentLength, session->ReadBuffer.size());
        if (contentLength == HttpResponse::InvalidContentLength) {
            ReadContentAll(session);    
        } else if (contentLength <= session->ReadBuffer.size()) {
            HandleReadContentByLength(session, contentLength, error_code(), contentLength);
        } else {
            asio::async_read(*session->Socket, session->ReadBuffer,
                asio::transfer_exactly(contentLength - session->ReadBuffer.size()),
                boost::bind(&Impl<Proto>::HandleReadContentByLength, this, session, contentLength,
                asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
    }

    void HandleReadContentByLength(HttpSessionPtr session, std::size_t contentLength,
                                   error_code const& error, std::size_t bytes_transferred) {
        THROW_IF(error && error != asio::error::eof || contentLength != session->ReadBuffer.size(),
            session->Request.GetId(), Http::ResponseBodyReadFailed);
        FinishReadContent(session);
    }

    void ReadContentAll(HttpSessionPtr session) {
        asio::async_read(*session->Socket, session->ReadBuffer, asio::transfer_at_least(1),
            boost::bind(&Impl<Proto>::HandleReadContentAll, this, session,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void HandleReadContentAll(HttpSessionPtr session, error_code const& error, std::size_t bytes_transferred) {
        if (!error) {
            ReadContentAll(session);
        } else {
            THROW_IF(error != asio::error::eof, session->Request.GetId(), Http::ResponseBodyReadFailed);
            FinishReadContent(session);
        }
    }

    void FinishReadContent(HttpSessionPtr session) {
        if (session->Response) {
            GetResponseContentRef(session) << &session->ReadBuffer;
        }
        {
            std::lock_guard<std::mutex> lock(m_activeSessionsMutex);
            CacheSocket(std::move(session->Socket));
            session->Cancel = VoidCallback();
            m_activeSessions.erase(session->Request.GetId());
        }
        session->Callback(session->Response, nullptr);
    }

private:
    std::list<SocketPtr> m_socketPool;
    std::unordered_map<RequestIdType, HttpSessionPtr> m_activeSessions;
    std::mutex m_socketMutex, m_activeSessionsMutex;
};

// Impl<Protocol::Https>

template<>
typename Impl<Protocol::Https>::SocketPtr Impl<Protocol::Https>::CreateSocket() {
    SocketPtr result(new SocketType(m_ios, *m_sslContext));
    result->set_verify_mode(asio::ssl::verify_none);
    result->set_verify_callback(asio::ssl::rfc2818_verification(m_serverName));
    return result;
}

template<>
void Impl<Protocol::Https>::HandShake(HttpSessionPtr session) {
    Log("SSL handshake...\n");
    session->Socket->async_handshake(asio::ssl::stream_base::client,
        boost::bind(&Impl<Protocol::Https>::HandleHandShake, this, session, asio::placeholders::error));
}

// HttpClientMT

HttpClientMT::HttpClientMT(std::string const& serverName, int port, Protocol proto, std::size_t threadCount)
{
    if (proto == Protocol::Http) {
        m_impl.reset(new Impl<Protocol::Http>(serverName, port, threadCount));
    }
    else {
        m_impl.reset(new Impl<Protocol::Https>(serverName, port, threadCount));
    }
}

HttpClientMT::~HttpClientMT()
{
}

void HttpClientMT::CancelRequest(RequestIdType id)
{
    m_impl->CancelRequest(id);
}

HttpRequest HttpClientMT::CreateRequest(HttpMethod method, std::string const& path, ParamList const& params) {
    return m_impl->CreateRequest(method, path, params);
}

void HttpClientMT::PostRequest(HttpRequest&& request, HttpRequestCallback callback) {
    m_impl->PostRequest(std::move(request), callback);
}