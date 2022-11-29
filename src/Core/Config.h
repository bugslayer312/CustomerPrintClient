#pragma once

#include <string>
#include <memory>

namespace jsonxx {
class Object;
} // namespace jsonxx

class Config {
public:
    Config(std::string const& path);
    ~Config();
    bool Load();
    void Save();
    bool GetString(std::string const& name, std::string& result) const;
    void SetString(std::string const& name, std::string const& value);
    bool GetNumber(std::string const& name, double& result) const;
    void SetNumber(std::string const& name, double value);
    bool GetBoolean(std::string const& name, bool& result) const;
    void SetBoolean(std::string const& name, bool value);
    void SetNull(std::string const& name);

private:
    std::string const m_path;
    std::unique_ptr<jsonxx::Object> m_json;
};