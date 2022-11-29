#pragma once

#include <cstdint>

enum class PrintOrientation {
    Portrait,
    Landscape
};

enum class PrintMode {
    OriginalSize,
    DisplaySize,
    StretchToPage
};

enum class HorizontalPageAlign {
    Left,
    Center,
    Right
};

enum class VerticalPageAlign {
    Top,
    Center,
    Bottom
};

class PrintSettings {
public:
    PrintSettings();
    
    PrintOrientation Orientation() const;
    PrintSettings& Orientation(PrintOrientation value);
    PrintMode Mode() const;
    PrintSettings& Mode(PrintMode value);
    HorizontalPageAlign HorAlign() const;
    PrintSettings& HorAlign(HorizontalPageAlign value);
    VerticalPageAlign VertAlign() const;
    PrintSettings& VertAlign(VerticalPageAlign value);
    
    bool operator==(PrintSettings const& rhs) const;
    bool operator!=(PrintSettings const& rhs) const;

private:
    std::uint32_t m_data;
};