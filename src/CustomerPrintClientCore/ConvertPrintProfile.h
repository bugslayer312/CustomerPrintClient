#pragma once

#include "../Printing/IPreviewRenderer.h"
#include "PrintProfile.h"

IPreviewRenderer::PrintProfile ConvertPrintProfile(PrintProfilePtr printProfile);