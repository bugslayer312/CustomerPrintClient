#pragma once

#include <string>

enum class UserInfoStatus {
    NotLogged,
    LoggedAnonymous,
    LoggedWithCreds
};

struct UserInfo {
    UserInfoStatus Status;
    std::string Name;
    std::string Email;

    UserInfo() : Status(UserInfoStatus::NotLogged){}
    UserInfo(UserInfoStatus status) : Status(status){}
    UserInfo(const char* name, const char* email) : Status(UserInfoStatus::LoggedWithCreds), Name(name), Email(email){}
    UserInfo(std::string const& name, std::string const& email)
        : Status(UserInfoStatus::LoggedWithCreds), Name(name), Email(email){}
};