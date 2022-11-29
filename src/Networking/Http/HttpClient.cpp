#include "HttpClient.h"

#include "TcpException.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../../Core/Format.h"
#include "../../Core/Log.h"
#include "../StringResources.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <fstream>
#include <exception>
#include <sstream>
#include <thread>
#include <chrono>
#include <list>

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
using ssl_socket = asio::ssl::stream<tcp::socket>;

typedef std::shared_ptr<asio::ip::tcp::resolver> ResolverPtr;
typedef std::function<void()> VoidCallback;
typedef asio::basic_socket<tcp, tcp::socket::lowest_layer_type::executor_type> BasicSocket;

#define THROW_IF_ERROR(ctx, error, msg) if (error) { \
    if (error == asio::error::operation_aborted) { \
        throw TcpOperationException(ctx, Http::OperationCanceled, true); \
    } else { \
        throw TcpOperationException(ctx, Format("%s: %s", msg, error.message().c_str()), false); \
    } \
} \

#define THROW_IF(cond, ctx, msg) if (cond) { \
    std::ostringstream _ost; \
    _ost << msg; \
    throw TcpOperationException(ctx, _ost.str(), false); \
}

struct HttpRequestContext {
    HttpRequestContext(HttpRequest&& request, HttpRequestCallback callback, int retry = 1)
        : Request(std::move(request))
        , Callback(callback)
        , Retry(retry)
    {
    }
    HttpRequest Request;
    asio::streambuf ReadBuffer;
    asio::streambuf WriteBuffer;
    ResponsePtr Response;
    HttpRequestCallback Callback;
    int Retry;
};
typedef std::shared_ptr<HttpRequestContext> HttpRequestContextPtr;

class TcpOperationException : public TcpException {
public:
    TcpOperationException(HttpRequestContextPtr context, std::string const& whatArg, bool wasCancel)
        : TcpException(whatArg, wasCancel)
        , m_context(context) {
    }

    TcpOperationException(HttpRequestContextPtr context, char const* whatArg, bool wasCancel)
        : TcpException(whatArg, wasCancel)
        , m_context(context) {
    }
    
    HttpRequestContextPtr GetContext() const {
        return m_context;
    }
private:
    HttpRequestContextPtr m_context;
};

class HttpClientImplBase {
public:
    HttpClientImplBase(std::string const& serverName, int port)
        : m_serverName(serverName)
        , m_port(port)
        , m_closing(false)
        , m_isProcessing(false)
        , m_hostResolved(false)        
    {
    }

    HttpRequest CreateRequest(HttpMethod method, std::string const& path, ParamList const& params) {
        HttpRequest result(method, path, params);
        result.AddHeader("Host", m_serverName)
            .AddHeader("Connection", "keep-alive")
            .AddHeader("Accept", "*/*");
        return result;
    }

    virtual ~HttpClientImplBase() = default;
    virtual void PostRequest(HttpRequest&& request, HttpRequestCallback callback) = 0;
    virtual void Cancel() = 0;
    virtual void Close() = 0;

protected:
    static std::string const& GetRequestPathCRef(HttpRequestContextPtr ctx) {
        return ctx->Request.m_path;
    }
    static std::string& GetResponseHttpVersionRef(HttpRequestContextPtr ctx) {
        return ctx->Response->m_httpVersion;
    }
    static std::uint32_t& GetResponseStatusCodeRef(HttpRequestContextPtr ctx) {
        return ctx->Response->m_statusCode;
    }
    static std::string& GetResponseStatusMessageRef(HttpRequestContextPtr ctx) {
        return ctx->Response->m_statusMessage;
    }
    static ParamMap& GetResponseHeadersRef(HttpRequestContextPtr ctx) {
        return ctx->Response->m_headers;
    }
    static std::stringstream& GetResponseContentRef(HttpRequestContextPtr ctx) {
        return ctx->Response->m_content;
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
    bool m_isProcessing;
    bool m_hostResolved;
    tcp::resolver::results_type m_hostResolveResult;
    std::unique_ptr<asio::ssl::context> m_sslContext;
    asio::io_service m_ios;
    std::unique_ptr<asio::io_service::work> m_worker;
    std::unique_ptr<std::thread> m_thread;
    VoidCallback m_cancel;
    std::list<HttpRequestContextPtr> m_waitList;
    std::mutex m_mutex;
};

template<Protocol Proto>
struct ProtoTraits {
    typedef tcp::socket SocketType;
};

template<>
struct ProtoTraits<Protocol::Https> {
    typedef ssl_socket SocketType;
};

template<Protocol Proto>
class Impl : public HttpClientImplBase {
public:
    typedef typename ProtoTraits<Proto>::SocketType SocketType;
    typedef std::shared_ptr<SocketType> SocketPtr;
    Impl(std::string const& serverName, int port, std::size_t threadCount)
        : HttpClientImplBase(serverName, port) {
        m_sslContext = CreateSslContext();
        m_worker.reset(new asio::io_service::work(m_ios));
        m_thread.reset(new std::thread([this](){
            Log("Start ios thread: %d\n", std::this_thread::get_id());
            while(!m_closing) {
                try {
                    m_ios.run();
                }
                catch(TcpOperationException& ex) {
                    HttpRequestContextPtr ctx = ex.GetContext();
                    if (ctx->Retry) {
                        Log("Retry request\n");
                        CloseSocket();
                        HttpRequestContextPtr retryCtx(new HttpRequestContext(std::move(ctx->Request),
                            ctx->Callback, --ctx->Retry));
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_waitList.push_front(retryCtx);
                    }
                    else {
                        ctx->Callback(nullptr, std::current_exception());
                    }
                }
                catch(std::exception& ex) {
                    Log(ex.what());
                }
                catch(...) {
                    Log("HttpClient: Unknown exception\n");
                }
                PollNext();
            }
            Log("Stop ios thread\n");
        }));
    }

    ~Impl() override {
        Close();
    }

    void Close() override {
        m_closing = true;
        Cancel();
        CloseSocket();
        m_worker.reset(nullptr);
        m_thread->join();
    }

    void Cancel() override {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto ctx: m_waitList) {
                ctx->Callback(nullptr, std::make_exception_ptr(TcpException("Canceled", true)));
            }
            m_waitList.clear();
        }
        if (m_cancel) {
            m_cancel();
            m_cancel = VoidCallback();
        }
    }

    void PostRequest(HttpRequest&& request, HttpRequestCallback callback) override {
        HttpRequestContextPtr ctx(new HttpRequestContext(std::move(request), callback));
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isProcessing) {
            m_waitList.push_back(ctx);
        }
        else {
            m_isProcessing = true;
            m_ios.post(std::bind(&Impl<Proto>::SendRequestProc, this, ctx));
        }
    }

private:
    static std::unique_ptr<asio::ssl::context> CreateSslContext() {
        return nullptr;
    }

    SocketPtr CreateSocket() {
        SocketPtr result(new SocketType(m_ios));
        m_cancel = std::bind([](SocketPtr sock){ sock->cancel(); }, result);
        return result;
    }

    static BasicSocket& GetTcpSocket(SocketPtr sock) {
        return *sock;
    }

    void CloseSocket() {
        if (m_socket) {
            error_code error;
            m_socket->shutdown(tcp::socket::shutdown_both, error);
            m_socket->close(error);
            m_socket.reset();
        }
    }

    void PollNext() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_waitList.empty()) {
            m_isProcessing = false;
        }
        else {
            m_isProcessing = true;
            HttpRequestContextPtr ctx = m_waitList.front();
            m_waitList.pop_front();
            m_ios.post(std::bind(&Impl<Proto>::SendRequestProc, this, ctx));
        }
    }

    void SendRequestProc(HttpRequestContextPtr ctx) {
        if (m_socket) {
            if (GetRequestPathCRef(ctx).empty()) {
                FinishReadContent(ctx);
            }
            else {
                std::ostringstream osst;
                osst << ctx->Request;
                Log("Request:\n%s\n", osst.str().c_str());
                std::ostream ost(&ctx->WriteBuffer);
                ost << ctx->Request;
                asio::async_write(*m_socket, ctx->WriteBuffer, boost::bind(&Impl<Proto>::HandleSendRequest
                    , this, ctx, asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
        }
        else {
            Connect(ctx);
        }
    }

    void Connect(HttpRequestContextPtr ctx) {
        if (m_hostResolved) {
            Log("Connecting...\n");
            SocketPtr sock = CreateSocket();
            asio::async_connect(GetTcpSocket(sock), m_hostResolveResult, boost::bind(&Impl<Proto>::HandleConnect
                , this, ctx, sock, asio::placeholders::error, asio::placeholders::endpoint));
        }
        else {
            Log("Resolving host...\n");
            ResolverPtr resolver(new tcp::resolver(m_ios));
            m_cancel = std::bind([](ResolverPtr res){ res->cancel(); }, resolver);
            resolver->async_resolve(m_serverName, std::to_string(m_port), tcp::resolver::resolver_base::numeric_service
                , boost::bind(&Impl<Proto>::HandleResolve, this, ctx, resolver, asio::placeholders::error
                , asio::placeholders::results));
        }
    }

    void HandleResolve(HttpRequestContextPtr ctx, ResolverPtr resolver, error_code const& error
                        , tcp::resolver::results_type results) {
        THROW_IF_ERROR(ctx, error, Format(Http::ResolveFailed, m_serverName.c_str()).c_str());
        m_hostResolveResult = results;
        m_hostResolved = true;
        Connect(ctx);
    }

    void HandleConnect(HttpRequestContextPtr ctx, SocketPtr sock, error_code const& error, tcp::endpoint const& ep) {
        if (error) {
            m_cancel = VoidCallback();
        }
        THROW_IF_ERROR(ctx, error, Format(Http::ConnectionFailed, m_serverName.c_str(), ep.address().to_string().c_str()
            , ep.port()).c_str());
        HandShake(ctx, sock);
    }

    void HandShake(HttpRequestContextPtr ctx, SocketPtr sock) {
        HandleHandShake(ctx, sock, error_code());
    }

    void HandleHandShake(HttpRequestContextPtr ctx, SocketPtr sock, error_code const& error) {
        if (error) {
            m_cancel = VoidCallback();
        }
        THROW_IF_ERROR(ctx, error, Http::SslHandShakeFailed);
        m_socket = sock;
        SendRequestProc(ctx);
    }

    void HandleSendRequest(HttpRequestContextPtr ctx, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(ctx, error, Http::SendRequestFailed);
        asio::async_read_until(*m_socket, ctx->ReadBuffer, "\r\n"
            , boost::bind(&Impl<Proto>::HandleReadStatusLine, this, ctx
            , asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void HandleReadStatusLine(HttpRequestContextPtr ctx, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(ctx, error, Http::ReadResponseFailed);
        std::istream ist(&ctx->ReadBuffer);
        std::string httpVersion;
        bool debug = false;
        if (debug)
        {
            std::string resp("F:\\Samples\\test\\response.txt");
            std::ofstream ost10(resp, std::ios::trunc|std::ios::binary|std::ios::out);
            ost10 << ist.rdbuf();
        }
        ist >> httpVersion;
        if (!IsValidHttpVersion(httpVersion)) {
            std::ostringstream ost(httpVersion);
            ost << " " << &ctx->ReadBuffer;
            Log("%s\n", ost.str().c_str());
        }
        THROW_IF(!IsValidHttpVersion(httpVersion), ctx, Format(Http::ResponseInvalidHttpVersion, httpVersion.c_str()).c_str());
        ctx->Response = CreateResponse();
        GetResponseHttpVersionRef(ctx) = httpVersion;
        ist >> GetResponseStatusCodeRef(ctx);
        ist.get();
        THROW_IF(!ist, ctx, Http::ResponseInvalidStatusCode);
        std::getline(ist, GetResponseStatusMessageRef(ctx), '\r');
        THROW_IF(ist.get() != '\n', ctx, Http::ResponseInvalidStatusMsg);
        asio::async_read_until(*m_socket, ctx->ReadBuffer, "\r\n\r\n"
            , boost::bind(&Impl<Proto>::HandleReadHeaders, this, ctx, asio::placeholders::error
            , asio::placeholders::bytes_transferred));
    }

    void HandleReadHeaders(HttpRequestContextPtr ctx, error_code const& error, std::size_t bytes_transferred) {
        THROW_IF_ERROR(ctx, error, Http::ResponseHeadersReadFailed);
        std::string header, headerName, headerValue;
        std::istream ist(&ctx->ReadBuffer);
        ParamMap& responseHeaders = GetResponseHeadersRef(ctx);
        while(true) {
            std::getline(ist, header, '\r');
            THROW_IF(ist.get() != '\n', ctx, Http::ResponseInvalidHeader);
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
        std::size_t const contentLength = ctx->Response->GetContentLength();
        //Log("HandleReadHeaders contentLength:%d readBuffer.size:%d\n", contentLength, ctx->ReadBuffer.size());
        if (contentLength == HttpResponse::InvalidContentLength) {
            ReadContentAll(ctx);    
        } else if (contentLength <= ctx->ReadBuffer.size()) {
            HandleReadContentByLength(ctx, contentLength, error_code(), contentLength);
        } else {
            asio::async_read(*m_socket, ctx->ReadBuffer, asio::transfer_exactly(contentLength - ctx->ReadBuffer.size())
                , boost::bind(&Impl<Proto>::HandleReadContentByLength, this, ctx, contentLength
                , asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
    }

    void HandleReadContentByLength(HttpRequestContextPtr ctx, std::size_t contentLength
                                    , error_code const& error, std::size_t bytes_transferred) {
        //bool b = error && error != asio::error::eof;
        //Log("HandleReadContentByLength contentLength:%d error:%d bytes_transferred:%d readBuffer.size:%d\n", contentLength, b,
        //    bytes_transferred, ctx->ReadBuffer.size());
        THROW_IF(error && error != asio::error::eof || contentLength != ctx->ReadBuffer.size(), ctx, Http::ResponseBodyReadFailed);
        FinishReadContent(ctx);
    }

    void ReadContentAll(HttpRequestContextPtr ctx) {
        asio::async_read(*m_socket, ctx->ReadBuffer, asio::transfer_at_least(1)
            , boost::bind(&Impl<Proto>::HandleReadContentAll, this, ctx
            , asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void HandleReadContentAll(HttpRequestContextPtr ctx, error_code const& error, std::size_t bytes_transferred) {
        if (!error) {
            ReadContentAll(ctx);
        } else {
            THROW_IF(error != asio::error::eof, ctx, Http::ResponseBodyReadFailed);
            FinishReadContent(ctx);
        }
    }

    void FinishReadContent(HttpRequestContextPtr ctx) {
        if (ctx->Response) {
            GetResponseContentRef(ctx) << &ctx->ReadBuffer;
        }
        ctx->Callback(ctx->Response, nullptr);
        PollNext();
    }

private:
    SocketPtr m_socket;
};

template<>
std::unique_ptr<asio::ssl::context> Impl<Protocol::Https>::CreateSslContext() {
    return std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23);
}

template<>
typename Impl<Protocol::Https>::SocketPtr Impl<Protocol::Https>::CreateSocket() {
    SocketPtr result(new SocketType(m_ios, *m_sslContext));
    result->set_verify_mode(asio::ssl::verify_none);
    result->set_verify_callback(asio::ssl::rfc2818_verification(m_serverName));
    m_cancel = std::bind([](SocketPtr sock){ sock->lowest_layer().cancel(); }, result);
    return result;
}

template<>
BasicSocket& Impl<Protocol::Https>::GetTcpSocket(Impl<Protocol::Https>::SocketPtr sock) {
    return sock->lowest_layer();
}

template<>
void Impl<Protocol::Https>::CloseSocket() {
    if (m_socket) {
        error_code error;
        m_socket->shutdown(error);
        GetTcpSocket(m_socket).shutdown(tcp::socket::shutdown_both, error);
        GetTcpSocket(m_socket).close(error);
        m_socket.reset();
    }
}

template<>
void Impl<Protocol::Https>::HandShake(HttpRequestContextPtr ctx, SocketPtr sock) {
    Log("SSL handshake...\n");
    sock->async_handshake(asio::ssl::stream_base::client
        , boost::bind(&Impl<Protocol::Https>::HandleHandShake, this, ctx, sock, asio::placeholders::error));
}

HttpClient::HttpClient(std::string const& serverName, int port, Protocol proto, std::size_t threadCount)
{
    if (proto == Protocol::Http) {
        m_impl.reset(new Impl<Protocol::Http>(serverName, port, threadCount));
    }
    else {
        m_impl.reset(new Impl<Protocol::Https>(serverName, port, threadCount));
    }
}

HttpClient::~HttpClient()
{
}

void HttpClient::Cancel()
{
    m_impl->Cancel();
}

HttpRequest HttpClient::CreateRequest(HttpMethod method, std::string const& path, ParamList const& params) {
    return m_impl->CreateRequest(method, path, params);
}

void HttpClient::PostRequest(HttpRequest&& request, HttpRequestCallback callback) {
    m_impl->PostRequest(std::move(request), callback);
}