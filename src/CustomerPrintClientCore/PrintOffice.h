#pragma once

#include "PrintProfile.h"
#include "Types.h"

class CoreManager;
class IPrintOfficeCallback;

struct PrintOfficePreview {
    std::string Id;
    float Latitude;
    float Longitude;
    std::string Name;
    std::string Address;
    std::uint32_t Distance;
    int Rating;
    virtual bool Mailgate() const = 0;

    virtual ~PrintOfficePreview() = default;
};

struct PrintOffice : PrintOfficePreview {
    std::string Phone;
    std::string Email;
    std::string Description;
    std::string Country;
    std::string Currency;
    std::string CurrencySymbol;
    float MinStripePayment;
    std::uint32_t IntMinStripePaiment;

    struct OfficeClassType {
        bool IsForeign() const {
            return (Flags & 0x00000001) > 0;
        }
        void IsForeign(bool value) {
            SetFlag(Flags, 0x00000001, value);
        }
        bool CanRendered() const {
            return (Flags & 0x00000002) > 0;
        }
        void CanRendered(bool value) {
            SetFlag(Flags, 0x00000002, value);
        }
        bool CanMailgate() const {
            return (Flags & 0x00000004) > 0;
        }
        void CanMailgate(bool value) {
            SetFlag(Flags, 0x00000004, value);
        }
        bool CanBinary() const {
            return (Flags & 0x00000008) > 0;
        }
        void CanBinary(bool value) {
            SetFlag(Flags, 0x00000008, value);
        }
        bool OnlyMailgate() const {
            return (Flags & 0x00000010) > 0;
        }
        void OnlyMailgate(bool value) {
            SetFlag(Flags, 0x00000010, value);
        }
        bool OnlyRendered() const {
            return (Flags & 0x00000020) > 0;
        }
        void OnlyRendered(bool value) {
            SetFlag(Flags, 0x00000020, value);
        }
        bool OnlyBinary() const {
            return (Flags & 0x00000040) > 0;
        }
        void OnlyBinary(bool value) {
            SetFlag(Flags, 0x00000040, value);
        }
    private:
        std::uint32_t Flags = 0;
    } OfficeClass;

    bool Online() const {
        return (Flags & 0x00000002) > 0;
    }
    void Online(bool value) {
        SetFlag(Flags, 0x00000002, value);
    }

    PrintOffice();
    PrintOffice(jsonxx::Object const& json);
    PrintOffice(PrintOffice const&) = default;
    PrintOffice(PrintOffice&&) = default;
    virtual ~PrintOffice() = default;
    PrintOffice& operator=(PrintOffice const&) = default;
    PrintOffice& operator=(PrintOffice&&) = default;
    operator bool() const {
        return (Flags & 0x00000001) > 0;
    }
    bool Mailgate() const override {
        return OfficeClass.OnlyMailgate();
    }

    static bool TryReadId(jsonxx::Object const& json, std::string& id);

    std::size_t GetPrintProfileCount() const;
    PrintProfilePtr GetPrintProfile(std::size_t idx) const;
    PrintProfilePtr GetPrintProfile(std::string const& id) const;
    PrintProfilePtr GetSelectedPrintProfile() const;
    std::string const& GetSelectedPrintProfileId() const;
    void SelectPrintProfile(std::string const id);

    friend class CoreManager;

private:
    std::uint32_t Flags;
    PrintProfileCollection PrintProfiles;
    std::string SelectedProfileId;
    IPrintOfficeCallback* Callback = nullptr;
};