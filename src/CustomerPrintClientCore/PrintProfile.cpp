#include "PrintProfile.h"

#include "../Core/jsonxx.h"

void SetFlag(uint32_t& flags, uint32_t flag, bool value) {
    if (value) {
        flags |= flag;
    }
    else {
        flag &= ~flag;
    }
}

PrintProfile::PrintProfile()
    : Flags(0)
{
}

PrintProfile::PrintProfile(jsonxx::Object const& json)
    : Flags(0)
{
    if (!json.has<jsonxx::Object>("paper")) return;
    jsonxx::Object const& jsonPaper = json.get<jsonxx::Object>("paper");
    if (!ReadString(jsonPaper, "name", Paper.Name) ||
        !ReadNumber(jsonPaper, "width", Paper.Width) ||
        !ReadNumber(jsonPaper, "height", Paper.Height) ||
        !ReadNumber(jsonPaper, "printWidth", Paper.PrintWidth) ||
        !ReadNumber(jsonPaper, "printHeight", Paper.PrintHeight) ||
        !ReadNumber(jsonPaper, "marginTop", Paper.MarginTop) ||
        !ReadNumber(jsonPaper, "marginTop", Paper.MarginLeft)) {
    
        return;
    }

    if (!json.has<jsonxx::Object>("printerSettings")) return;
    jsonxx::Object const& jsonPrinterSettings = json.get<jsonxx::Object>("printerSettings");
    if (!ReadNumber(jsonPrinterSettings, "dpix", PrinterSettings.DpiX) ||
        !ReadNumber(jsonPrinterSettings, "dpiy", PrinterSettings.DpiY)) {

        return;
    }

    if (!json.has<jsonxx::Object>("profileSettings")) return;
    jsonxx::Object const& jsonProfileSettings = json.get<jsonxx::Object>("profileSettings");
    ProfileSettings.IsColor(jsonProfileSettings.get<jsonxx::Boolean>("isColor", false));
    ProfileSettings.IsDuplex(jsonProfileSettings.get<jsonxx::Boolean>("isDuplex", false));

    if (!ReadString(json, "puid", Id) ||
        !ReadString(json, "name", Name) ||
        !ReadNumber(json, "price", Price) ||
        !ReadNumber(json, "intPrice", IntPrice)) {
        
        return;
    }
    Online(json.get<jsonxx::Boolean>("online", false));
    IsMailgate(json.get<jsonxx::Boolean>("isMailgate", false));
    IsBinary(json.get<jsonxx::Boolean>("isBinary", false));

    SetFlag(Flags, 0x00000001, true);
}