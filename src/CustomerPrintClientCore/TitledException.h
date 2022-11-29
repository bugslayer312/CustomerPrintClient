#pragma once

#include <stdexcept>
#include <string>

enum class ExceptionCategory {
    Undefined = 0,
    Job = 1,
    Update
};

class TitledException : public std::runtime_error {
public:
    TitledException(std::string const& title, char const* what, ExceptionCategory category);
    TitledException(std::string const& title, std::string const& what, ExceptionCategory category);
    TitledException(TitledException const& other);
    std::string const& GetTitle() const;
    ExceptionCategory GetCategory() const;

private:
    std::string const m_title;
    ExceptionCategory m_category;
};