#include "TitledException.h"

TitledException::TitledException(std::string const& title, char const* what, ExceptionCategory category)
    : std::runtime_error(what)
    , m_title(title)
    , m_category(category)
{
}

TitledException::TitledException(std::string const& title, std::string const& what, ExceptionCategory category)
    : std::runtime_error(what)
    , m_title(title)
    , m_category(category)
{
}

TitledException::TitledException(TitledException const& other)
    : std::runtime_error(other)
    , m_title(other.m_title)
    , m_category(other.m_category)
{
}

std::string const& TitledException::GetTitle() const {
    return m_title;
}

ExceptionCategory TitledException::GetCategory() const {
    return m_category;
}