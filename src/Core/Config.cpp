#include "Config.h"
#include "Format.h"
#include "jsonxx.h"

#include <fstream>

template<class VType>
struct TypeToJsonType {
    typedef jsonxx::String Type;
};

template<>
struct TypeToJsonType<double> {
    typedef jsonxx::Number Type;
};

template<>
struct TypeToJsonType<bool> {
    typedef jsonxx::Boolean Type;
};

template<class VType>
bool GetValue(jsonxx::Object* jsonObj, std::string const& name, VType& result) {
    typedef typename TypeToJsonType<VType>::Type JsonType;
    std::string sectionName;
    std::size_t searchPos(0), delPos(std::string::npos);
    while ((delPos = name.find('/', searchPos)) != std::string::npos) {
        sectionName = name.substr(searchPos, delPos - searchPos);
        if (jsonObj->has<jsonxx::Object>(sectionName)) {
            jsonObj = &jsonObj->get<jsonxx::Object>(sectionName);
            searchPos = delPos + 1;
        }
        else {
            return false;
        }
    }
    sectionName = name.substr(searchPos);
    if (jsonObj->has<JsonType>(sectionName)) {
        result = jsonObj->get<JsonType>(sectionName);
        return true;
    }
    return false;
}

template<class VType>
void SetValue(jsonxx::Object* jsonObj, std::string const& name, VType value) {
    std::string sectionName;
    std::size_t searchPos(0), delPos(std::string::npos);
    while ((delPos = name.find('/', searchPos)) != std::string::npos) {
        sectionName = name.substr(searchPos, delPos - searchPos);
        if (!jsonObj->has<jsonxx::Object>(sectionName)) {
            *jsonObj << sectionName << jsonxx::Object();
        }
        jsonObj = &jsonObj->get<jsonxx::Object>(sectionName);
        searchPos = delPos + 1;
    }
    sectionName = name.substr(searchPos);
    *jsonObj << sectionName << value;
}

Config::Config(std::string const& path)
    : m_path(path)
    , m_json(new jsonxx::Object())
{
}

Config::~Config() {
}

bool Config::Load() {
    std::ifstream ifs(m_path, std::ios::in|std::ios::binary);
    if (ifs) {
        return m_json->parse(ifs);
    }
    return true;
}

void Config::Save() {
    std::ofstream ofs(m_path, std::ios::out|std::ios::ate);
    ofs << m_json->json();
}

bool Config::GetString(std::string const& name, std::string& result) const {
    return GetValue(m_json.get(), name, result);
}

void Config::SetString(std::string const& name, std::string const& value) {
    SetValue(m_json.get(), name, value);
}

bool Config::GetNumber(std::string const& name, double& result) const {
    return GetValue(m_json.get(), name, result);
}

void Config::SetNumber(std::string const& name, double value) {
    SetValue(m_json.get(), name, value);
}

bool Config::GetBoolean(std::string const& name, bool& result) const {
    return GetValue(m_json.get(), name, result);
}

void Config::SetBoolean(std::string const& name, bool value) {
    SetValue(m_json.get(), name, value);
}

void Config::SetNull(std::string const& name) {
    jsonxx::Object* jsonObj = m_json.get();
    std::string sectionName;
    std::size_t searchPos(0), delPos(std::string::npos);
    while ((delPos = name.find('/', searchPos)) != std::string::npos) {
        sectionName = name.substr(searchPos, delPos - searchPos);
        if (jsonObj->has<jsonxx::Object>(sectionName)) {
            jsonObj = &jsonObj->get<jsonxx::Object>(sectionName);
            searchPos = delPos + 1;
        }
        else {
            return;
        }
    }
    sectionName = name.substr(searchPos);
    *jsonObj << sectionName << jsonxx::Null();
}