#pragma once

#include <string>
#include <vector>
#include <memory>

namespace jsonxx {

class Object;

} // namespace jsonxx

void SetFlag(uint32_t& flags, uint32_t flag, bool value);

struct PrintProfile {
    struct PaperType {
        std::string Name;
        std::uint32_t Width;
        std::uint32_t Height;
        std::uint32_t PrintWidth;
        std::uint32_t PrintHeight;
        std::uint16_t MarginTop;
        std::uint16_t MarginLeft;
    } Paper;

    struct PrinterSettingsType {
        std::uint16_t DpiX;
        std::uint16_t DpiY;
    } PrinterSettings;

    struct ProfileSettingsType {
    private:
        uint32_t Flags = 0;
    public:
        bool IsColor() const {
            return (Flags & 0x00000001) > 0;
        }
        void IsColor(bool value) {
            SetFlag(Flags, 0x00000001, value);
        }
         bool IsDuplex() const {
            return (Flags & 0x00000002) > 0;
        }
        void IsDuplex(bool value) {
            SetFlag(Flags, 0x00000002, value);
        }
    } ProfileSettings;

private:
    uint32_t Flags;
public:
    std::string Id;
    std::string Name;
    float Price;
    std::uint32_t IntPrice;
    bool Online() const {
        return (Flags & 0x00000002) > 0;
    }
    void Online(bool value) {
        SetFlag(Flags, 0x00000002, value);
    }
    bool IsMailgate() const {
        return (Flags & 0x00000004) > 0;
    }
    void IsMailgate(bool value) {
        SetFlag(Flags, 0x00000004, value);
    }
    bool IsBinary() const {
        return (Flags & 0x00000008) > 0;
    }
    void IsBinary(bool value) {
        SetFlag(Flags, 0x00000008, value);
    }

    PrintProfile();
    PrintProfile(jsonxx::Object const& json);
    PrintProfile(PrintProfile const&) = default;
    PrintProfile(PrintProfile&&) = default;
    PrintProfile& operator=(PrintProfile const&) = default;
    PrintProfile& operator=(PrintProfile&&) = default;
    operator bool() const {
        return (Flags & 0x00000001) > 0;
    }
};

typedef std::shared_ptr<PrintProfile> PrintProfilePtr;
typedef std::vector<PrintProfilePtr> PrintProfileCollection;