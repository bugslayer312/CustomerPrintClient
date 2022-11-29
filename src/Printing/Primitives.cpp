#include "Primitives.h"

namespace Drawing {

// Point

Point::Point() : X(0), Y(0)
{
}

Point::Point(int x, int y) : X(x), Y(y)
{
}

// Size

Size::Size() : Width(0), Height(0)
{
}

Size::Size(int width, int height) : Width(width), Height(height)
{
}

// Rect

Rect::Rect() : X(0), Y(0), Width(0), Height(0)
{
}

Rect::Rect(int x, int y, int width, int height) : X(x), Y(y), Width(width), Height(height)
{
}

Rect::Rect(Point const& origin, Size const& size) : X(origin.X), Y(origin.Y), Width(size.Width), Height(size.Height)
{
}

int Rect::GetRight() const {
    return X + Width;
}

int Rect::GetBottom() const {
    return Y + Height;
}

} // namespace Drawing