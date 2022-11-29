#include "PrintSettings.h"

PrintSettings::PrintSettings()
    : m_data(0)
{
    Orientation(PrintOrientation::Portrait)
        .Mode(PrintMode::DisplaySize)
        .HorAlign(HorizontalPageAlign::Center)
        .VertAlign(VerticalPageAlign::Top);
}

PrintOrientation PrintSettings::Orientation() const {
    return static_cast<PrintOrientation>(m_data & 0x00000001);
}

PrintSettings& PrintSettings::Orientation(PrintOrientation value) {
    m_data &= ~0x00000001;
    m_data |= 0x00000001 & static_cast<std::uint32_t>(value);
    return *this;
}

PrintMode PrintSettings::Mode() const {
    return static_cast<PrintMode>((m_data & 0x00000006) >> 1);
}

PrintSettings& PrintSettings::Mode(PrintMode value) {
    m_data &= ~0x00000006;
    m_data |= 0x00000006 & (static_cast<std::uint32_t>(value) << 1);
    return *this;
}

HorizontalPageAlign PrintSettings::HorAlign() const {
    return static_cast<HorizontalPageAlign>((m_data & 0x00000018) >> 3);
}

PrintSettings& PrintSettings::HorAlign(HorizontalPageAlign value) {
    m_data &= ~0x00000018;
    m_data |= 0x00000018 & (static_cast<std::uint32_t>(value) << 3);
    return *this;
}

VerticalPageAlign PrintSettings::VertAlign() const {
    return static_cast<VerticalPageAlign>((m_data & 0x00000060) >> 5);
}

PrintSettings& PrintSettings::VertAlign(VerticalPageAlign value) {
    m_data &= ~0x00000060;
    m_data |= 0x00000060 & (static_cast<std::uint32_t>(value) << 5);
    return *this;
}

bool PrintSettings::operator==(PrintSettings const& rhs) const {
    return m_data == rhs.m_data;
}

bool PrintSettings::operator!=(PrintSettings const& rhs) const {
    return !(*this == rhs);
}