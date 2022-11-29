#include "PdfError.h"

#include "../../Core/Format.h"

#include <fpdfview.h>
#include <fpdf_ext.h>

namespace Pdf {

std::string const UnknownError("Unknown Error");
std::string const FileError("File not found or could not be opened.");
std::string const FormatError("File not in PDF format or corrupted.");
std::string const PasswordError("Password required or incorrect password.");
std::string const SecurityError("Unsupported security scheme.");
std::string const PageError("Page not found or content error.");

std::string ErrorMsg(unsigned long errorCode) {
    switch (errorCode)
	{
	case FPDF_ERR_UNKNOWN:
        return UnknownError;
	case FPDF_ERR_FILE:
		return FileError;
	case FPDF_ERR_FORMAT:
		return FormatError;
	case FPDF_ERR_PASSWORD:
		return PasswordError;
	case FPDF_ERR_SECURITY:
		return SecurityError;
	case FPDF_ERR_PAGE:
		return PageError;
	};
    if (errorCode != FPDF_ERR_SUCCESS) {
        return Format("%s (%d)", UnknownError.c_str(), errorCode);
    }
    return std::string();
}

std::string UnsupportedFeautureString(int type) {
    std::string feature;
	switch (type) {
    case FPDF_UNSP_DOC_XFAFORM:
        feature = "XFA";
        break;
    case FPDF_UNSP_DOC_PORTABLECOLLECTION:
        feature = "Portfolios_Packages";
        break;
    case FPDF_UNSP_DOC_ATTACHMENT:
    case FPDF_UNSP_ANNOT_ATTACHMENT:
        // Ignore annotations unsupported feature
        break;
    case FPDF_UNSP_DOC_SECURITY:
        feature = "Rights_Management";
        break;
    case FPDF_UNSP_DOC_SHAREDREVIEW:
        feature = "Shared_Review";
        break;
    case FPDF_UNSP_DOC_SHAREDFORM_ACROBAT:
    case FPDF_UNSP_DOC_SHAREDFORM_FILESYSTEM:
    case FPDF_UNSP_DOC_SHAREDFORM_EMAIL:
        feature = "Shared_Form";
        break;
    case FPDF_UNSP_ANNOT_3DANNOT:
        feature = "3D";
        break;
    case FPDF_UNSP_ANNOT_MOVIE:
        feature = "Movie";
        break;
    case FPDF_UNSP_ANNOT_SOUND:
        feature = "Sound";
        break;
    case FPDF_UNSP_ANNOT_SCREEN_MEDIA:
    case FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA:
        feature = "Screen";
        break;
    case FPDF_UNSP_ANNOT_SIG:
        feature = "Digital_Signature";
        break;
    default:
        feature = "Unknown";
        break;
	}
    return feature;
}

} // namespace Pdf