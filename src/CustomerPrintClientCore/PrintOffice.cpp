#include "PrintOffice.h"

#include "../Core/jsonxx.h"
#include "../Core/Log.h"

#include <algorithm>

PrintOffice::PrintOffice()
    : Flags(0)
{
}

PrintOffice::PrintOffice(jsonxx::Object const& json)
    : Flags(0)
{
    if (!ReadString(json, "ouid", Id) ||
        !ReadString(json, "name", Name) ||
        !ReadString(json, "description", Description) ||
        !ReadString(json, "address", Address) ||
        !ReadString(json, "email", Email) ||
        !ReadString(json, "phone", Phone) ||
        !ReadString(json, "country", Country) ||
        !ReadString(json, "currency", Currency) ||
        !ReadString(json, "symbol", CurrencySymbol) ||
        !ReadNumber(json, "distance", Distance) ||
        !ReadNumber(json, "latitude", Latitude) ||
        !ReadNumber(json, "longitude", Longitude) ||
        !ReadNumber(json, "minStripePayment", MinStripePayment) ||
        !ReadNumber(json, "intMinStripePayment", IntMinStripePaiment) ||
        !ReadNumber(json, "rating", Rating)) {

        return;
    }
    
    Online(json.get<jsonxx::Boolean>("online"));

    if (!json.has<jsonxx::Object>("officeClass")) {
        return;
    }
    jsonxx::Object const& jsonOfficeClass = json.get<jsonxx::Object>("officeClass");
    OfficeClass.IsForeign(json.get<jsonxx::Boolean>("isForeign", false));
    OfficeClass.CanRendered(json.get<jsonxx::Boolean>("canRendered", false));
    OfficeClass.CanMailgate(json.get<jsonxx::Boolean>("canMailgate", false));
    OfficeClass.CanBinary(json.get<jsonxx::Boolean>("canBinary", false));
    OfficeClass.OnlyMailgate(json.get<jsonxx::Boolean>("onlyMailgate", false));
    OfficeClass.OnlyRendered(json.get<jsonxx::Boolean>("onlyRendered", false));
    OfficeClass.OnlyBinary(json.get<jsonxx::Boolean>("onlyBinary", false));

    if (json.has<jsonxx::Array>("profiles")) {
        jsonxx::Array const& jsonProfiles = json.get<jsonxx::Array>("profiles");
        PrintProfiles.reserve(jsonProfiles.size());
        for (std::size_t i(0), cnt(jsonProfiles.size()); i < cnt; ++i) {
            jsonxx::Object const& jsonProfile = jsonProfiles.get<jsonxx::Object>(i);
            PrintProfilePtr profile(new PrintProfile(jsonProfile));
            if (*profile) {
                PrintProfiles.push_back(profile);
            }
            else {
                // Log("Skipped invalid profile for office \"%s\": %s\n", Id.c_str(), jsonProfile.json().c_str());
            }
        }
    }

    if (!PrintProfiles.empty()) {
        SetFlag(Flags, 0x00000001, true);
        SelectedProfileId = PrintProfiles.front()->Id;
    }
}

bool PrintOffice::TryReadId(jsonxx::Object const& json, std::string& id) {
    return ReadString(json, "ouid", id);
}

std::size_t PrintOffice::GetPrintProfileCount() const {
    return PrintProfiles.size();
}

PrintProfilePtr PrintOffice::GetPrintProfile(std::size_t idx) const {
    return PrintProfiles[idx];
}

PrintProfilePtr PrintOffice::GetPrintProfile(std::string const& id) const {
    if (!id.empty()) {
        auto found = std::find_if(PrintProfiles.cbegin(), PrintProfiles.cend(), [&id](PrintProfilePtr const& pp) {
            return pp->Id == id;
        });
        if (found != PrintProfiles.cend()) {
            return *found;
        }
    }
    return nullptr;
}

PrintProfilePtr PrintOffice::GetSelectedPrintProfile() const {
    PrintProfilePtr result;
    if (!PrintProfiles.empty()) {
        result = GetPrintProfile(SelectedProfileId);
        if (!result) {
            result = PrintProfiles.front();
        }
    }
    return result;
}

std::string const& PrintOffice::GetSelectedPrintProfileId() const {
    return SelectedProfileId;
}

void PrintOffice::SelectPrintProfile(std::string const id) {
    SelectedProfileId = id;
}