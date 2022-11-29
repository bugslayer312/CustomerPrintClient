#include "CloudPrintClient.h"
#include "../Networking/Http/HttpClientMT.h"
#include "../Networking/Http/HttpRequest.h"
#include "../Networking/Http/HttpResponse.h"
#include "../Networking/Http/HttpException.h"
#include "../Networking/StringResources.h"
#include "../Core/jsonxx.h"
#include "../Core/Log.h"
#include "PrintOffice.h"
#include "PrintJobInfo.h"

char const* const BadParameters = "Bad parameters";
char const* const NotAuthorized = "Not authorized";

std::size_t const HttpThreadCount = 2;

struct CloudPrintClient::LoginContext {
    std::string UserId;
    std::string Token;
    std::string RefreshToken;
};

CloudPrintClient::CloudPrintClient(std::string const& serverName)
    : m_httpClient(new HttpClientMT(serverName, 443, Protocol::Https, HttpThreadCount))
{
}

CloudPrintClient::~CloudPrintClient()
{
}

/*void CloudPrintClient::AsyncConnect(VoidCallbackEx callback) {
    m_httpClient->PostRequest(m_httpClient->CreateRequest(HttpMethod::Get, "", {}),
        [this, callback] (ResponsePtr response, std::exception_ptr ex) {
            callback(ex);
        });
}

void CloudPrintClient::WaitConnected() {
    if (m_connectFuture) {
        m_connectFuture->get();
        m_isConnected = true;
        m_connectFuture.reset();
    }
}

bool CloudPrintClient::IsConnected() const {
    return m_isConnected;
} */

void CloudPrintClient::AsyncLogin(std::string const& email, std::string const& pass, VoidCallbackEx callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, "/api/v1/oauth/login", {});
    jsonxx::Object content;
    content << "email" << email;
    content << "password" << pass;
    request.SetContent(content);

    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        jsonxx::Object contentResponse;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = contentResponse.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && contentResponse.has<jsonxx::Object>("body")) {
                jsonxx::Object& body = contentResponse.get<jsonxx::Object>("body");
                if (body.has<jsonxx::String>("token") && body.has<jsonxx::String>("refresh")) {
                    m_loginCtx.reset(new LoginContext());
                    m_loginCtx->Token = body.get<jsonxx::String>("token");
                    m_loginCtx->RefreshToken = body.get<jsonxx::String>("refresh");
                    return callback(nullptr);
                }
            }
            return callback(std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(std::make_exception_ptr(HttpException(code, contentParsed && contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };
    
    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncSignUp(std::string const& email, std::string const& pass, std::string const& name,
                                   VoidCallbackEx callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, "/api/v1/clients", {});
    jsonxx::Object content;
    content << "email" << email;
    content << "password" << pass;
    content << "username" << name;
    request.SetContent(content);

    auto httpCallback = [this, email, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        jsonxx::Object contentResponse;
        bool const contentParsed = contentResponse.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && contentResponse.has<jsonxx::String>("body")) {
                Log("SignUp for %s. uuid:%s", email.c_str(), contentResponse.get<jsonxx::String>("body").c_str());
                return callback(nullptr);
            }
            return callback(std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(std::make_exception_ptr(HttpException(code, contentParsed &&
            contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncResetPassword(std::string const& email, VoidCallbackEx callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, "/api/v1/password/recovery", {});
    jsonxx::Object content;
    content << "email" << email;
    request.SetContent(content);

    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        if (code == 200) {
            return callback(nullptr);
        }
        jsonxx::Object contentResponse;
        callback(std::make_exception_ptr(HttpException(code, contentResponse.parse(response->GetContent()) &&
            contentResponse.has<jsonxx::String>("error") ? contentResponse.get<jsonxx::String>("error")
            : response->GetStatusMessage())));
    };
    
    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncLoginAnonymously(VoidCallbackEx callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, "/api/v1/clients/anonymous", {});
    request.AddHeader(HttpHeaders::ContentType, ContentTypeToString(ContentType::ApplicationJson));
    
    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        jsonxx::Object content;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200){
            if (contentParsed && content.has<jsonxx::Object>("body")) {
                jsonxx::Object& body = content.get<jsonxx::Object>("body");
                if (body.has<jsonxx::String>("uuid") && body.has<jsonxx::String>("token") 
                    && body.has<jsonxx::String>("refresh")) {
                    m_loginCtx.reset(new LoginContext());
                    m_loginCtx->UserId = body.get<jsonxx::String>("uuid");
                    m_loginCtx->Token = body.get<jsonxx::String>("token");
                    m_loginCtx->RefreshToken = body.get<jsonxx::String>("refresh");
                    return callback(nullptr);
                }
            }
            return callback(std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncLogout(VoidCallbackEx callback) {
    if (!m_loginCtx) {
        return callback(nullptr);
    }
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, "/api/v1/oauth/logout", {});
    request.AddHeader(HttpHeaders::ContentType, ContentTypeToString(ContentType::ApplicationJson))
           .AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);

    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        if (code == 200) {
            m_loginCtx.reset(nullptr);
            return callback(nullptr);
        }
        jsonxx::Object content;
        callback(std::make_exception_ptr(HttpException(code, content.parse(response->GetContent()) &&
            content.has<jsonxx::String>("error") ? content.get<jsonxx::String>("error")
            : response->GetStatusMessage())));
    };
    
    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncGetUserInfo(GetUserInfoCallbackEx callback) {
    if (!m_loginCtx) {
        return callback(UserInfo(), nullptr);
    }
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, "/api/v1/clients/me", {});
    jsonxx::Object content;
    request.AddHeader(HttpHeaders::ContentType, ContentTypeToString(ContentType::ApplicationJson))
            .AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);

    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(UserInfo(), ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        jsonxx::Object content;
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Object>("body")) {
                jsonxx::Object& body = content.get<jsonxx::Object>("body");
                if (body.has<jsonxx::String>("uuid") && body.has<jsonxx::String>("email") 
                    && body.has<jsonxx::String>("username")) {
                    m_loginCtx->UserId = body.get<jsonxx::String>("uuid");
                    UserInfo result(body.get<jsonxx::String>("username"), body.get<jsonxx::String>("email"));
                    if (result.Name == "anonymous") {
                        result.Status = UserInfoStatus::LoggedAnonymous;
                    }
                    return callback(result, nullptr);
                }
            }
            return callback(UserInfo(), std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(UserInfo(), std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncSearchPrintOffices(float latitude, float longitude, std::uint32_t radiusMeters,
                                                SetOfStringPtr excludeKeys, SearchPrintOfficesCallbackEx callback) {
    
    if (!m_loginCtx) {
        return callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
    }
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, "/api/v1/clients/search", 
        {{"lat", std::to_string(latitude)},
         {"lon", std::to_string(longitude)},
         {"r", std::to_string(radiusMeters)}});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);

    auto httpCallback = [this, excludeKeys, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        PrintOfficeListPtr result(new PrintOfficeList());
        jsonxx::Object content;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Array>("body")) {
                jsonxx::Array& body = content.get<jsonxx::Array>("body");
                std::string officeId;
                for (std::size_t i(0), cnt(body.size()); i < cnt; ++i) {
                    jsonxx::Object const& jsonOffice = body.get<jsonxx::Object>(i);
                    if (!PrintOffice::TryReadId(jsonOffice, officeId) || excludeKeys->find(officeId) != excludeKeys->end()) {
                        continue;
                    }
                    PrintOfficePtr office(new PrintOffice(jsonOffice));
                    if (*office) {
                        result->push_back(office);
                    }
                    else {
                        // Log("Skipped invalid office: %s\n", jsonOffice.json().c_str());
                    }
                }
                return callback(result, nullptr);
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::AsyncGetPrintOffice(std::string const& officeId, GetPrintOfficeCallbackEx callback) {
    if (!m_loginCtx) {
        return callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
    }
    std::string path("/api/v1/clients/offices/");
    path += officeId;
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, path, {});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);
    
    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        jsonxx::Object content;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Object>("body")) {
                jsonxx::Object& jsonOffice = content.get<jsonxx::Object>("body");
                PrintOfficePtr office(new PrintOffice(jsonOffice));
                if (*office) {
                    return callback(office, nullptr);
                }
                else {
                    // Log("Skipped invalid office: %s\n", jsonOffice.json().c_str());
                }
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

RequestIdType CloudPrintClient::AsyncCreateJob(std::string const& officeId, std::string const& profileId,
                                               std::string const& jobName, std::string const& fmt,
                                               std::size_t pageCount, int copies, CreateJobCallbackEx callback) {

    if (!m_loginCtx) {
        callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
        return 0;
    }
    std::string path("/api/v1/clients/jobs/");
    path += officeId + "/";
    path += profileId;
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, path, {});
    jsonxx::Object content;
    content << "name" << jobName;
    content << "format" << fmt;
    content << "amount" << pageCount;
    content << "payLater" << true;
    {
        jsonxx::Object contentSettings;
        contentSettings << "copies" << copies;
        contentSettings << "range" << "";
        content << "settings" << contentSettings;
    }
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token)
           .SetContent(content);
    
    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        jsonxx::Object contentResponse;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = contentResponse.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && contentResponse.has<jsonxx::Object>("body")) {
                jsonxx::Object& body = contentResponse.get<jsonxx::Object>("body");
                if (body.has<jsonxx::String>("juid") && body.has<jsonxx::String>("accessCode")) {
                    CreateJobResultPtr createJobResult(new CreateJobResult());
                    createJobResult->JobId = body.get<jsonxx::String>("juid");
                    createJobResult->AccessCode = body.get<jsonxx::String>("accessCode");
                    return callback(createJobResult, nullptr);
                }
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr,
            std::make_exception_ptr(HttpException(code, contentParsed && contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
    return request.GetId();;
}

RequestIdType CloudPrintClient::AsyncUploadPage(std::string const& jobId, std::size_t pageNum, std::string const& filename,
                                                std::unique_ptr<std::istream> fileContent, std::size_t fileSize,
                                                ContentType contentType, UploadPageResultCallbackEx callback) {
    if (!m_loginCtx) {
        callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
        return 0;
    }
    std::string path("/api/v1/clients/jobs/uploads/");
    path += jobId + "/";
    path += std::to_string(pageNum);
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Post, path, {});
    std::unique_ptr<HttpMultipartFormDataContent> content(new HttpMultipartFormDataContent());
    content->AddFile("page", filename, std::move(fileContent), fileSize, contentType);
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token)
           .SetContent(std::move(content));
    
    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        jsonxx::Object contentResponse;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = contentResponse.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && contentResponse.has<jsonxx::Object>("body")) {
                jsonxx::Object& body = contentResponse.get<jsonxx::Object>("body");
                if (body.has<jsonxx::String>("url") && body.has<jsonxx::String>("pguid")) {
                    UploadPageResultPtr uploadJobResult(new UploadPageResult());
                    uploadJobResult->Url = body.get<jsonxx::String>("url");
                    uploadJobResult->PageId = body.get<jsonxx::String>("pguid");
                    return callback(uploadJobResult, nullptr);
                }
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr,
            std::make_exception_ptr(HttpException(code, contentParsed && contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
    return request.GetId();
}

RequestIdType CloudPrintClient::AsyncCompleteJob(std::string const& jobId, VoidCallbackEx callback) {
    if (!m_loginCtx) {
        callback(std::make_exception_ptr(HttpException(500, NotAuthorized)));
        return 0;
    }
    std::string path("/api/v1/clients/jobs/");
    path += jobId;
    path += "/uploaded";
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Put, path, {});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);
    
    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        if (code == 200) {
            return callback(nullptr);
        }
        jsonxx::Object contentResponse;
        bool const contentParsed = contentResponse.parse(response->GetContent());
        callback(std::make_exception_ptr(HttpException(code, contentParsed && contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
    return request.GetId();
}

RequestIdType CloudPrintClient::AsyncCancelJob(std::string const& jobId, VoidCallbackEx callback) {
    if (!m_loginCtx) {
        callback(std::make_exception_ptr(HttpException(500, NotAuthorized)));
        return 0;
    }
    std::string path("/api/v1/clients/jobs/");
    path += jobId;
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Delete, path, {});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);
    
    auto httpCallback = [this, callback] (ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        if (code == 200) {
            return callback(nullptr);
        }
        jsonxx::Object contentResponse;
        bool const contentParsed = contentResponse.parse(response->GetContent());
        callback(std::make_exception_ptr(HttpException(code, contentParsed && contentResponse.has<jsonxx::String>("error") ?
            contentResponse.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
    return request.GetId();
}

RequestIdType CloudPrintClient::AsyncGetJob(std::string const& jobId, GetJobCallbackEx callback) {
    if (!m_loginCtx) {
        callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
        return 0;
    }
    std::string path("/api/v1/clients/jobs/");
    path += jobId;
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, path, {});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);
    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        jsonxx::Object content;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Object>("body")) {
                PrintJobInfoListPtr result(new PrintJobInfoList());
                jsonxx::Object& jsonJob = content.get<jsonxx::Object>("body");
                PrintJobInfoPtr job(new PrintJobInfo(jsonJob));
                if (*job) {
                    return callback(job, nullptr);
                }
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
    return request.GetId();
}

void CloudPrintClient::AsyncGetJobsList(GetJobsListCallbackEx callback, int offset /* 0 */, int limit /* 50 */) {
    if (!m_loginCtx) {
        return callback(nullptr, std::make_exception_ptr(HttpException(500, NotAuthorized)));
    }
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, "/api/v1/clients/jobs/",
        {{"limit", std::to_string(limit)},
         {"offset", std::to_string(offset)}});
    request.AddHeader(HttpHeaders::Authorization, m_loginCtx->Token);

    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        jsonxx::Object content;
        std::uint32_t const code = response->GetStatusCode();
        bool const contentParsed = content.parse(response->GetContent());
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Array>("body")) {
                PrintJobInfoListPtr result(new PrintJobInfoList());
                jsonxx::Array& body = content.get<jsonxx::Array>("body");
                for (std::size_t i(0), cnt(body.size()); i < cnt; ++i) {
                    jsonxx::Object const& jsonJob = body.get<jsonxx::Object>(i);
                    PrintJobInfoPtr job(new PrintJobInfo(jsonJob));
                    if (*job) {
                        result->push_back(job);
                    }
                    else {
                        Log("Skipped invalid printJob: %s\n", jsonJob.json().c_str());
                    }
                }
                /* result->sort([](PrintJobInfoPtr const& el1, PrintJobInfoPtr const& el2) {
                    return el1->UpdateTime >= el2->UpdateTime;
                }); */
                return callback(result, nullptr);
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void CloudPrintClient::CancelRequest(RequestIdType cancelId) {
    m_httpClient->CancelRequest(cancelId);
}