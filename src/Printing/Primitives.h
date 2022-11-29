#pragma once

namespace Drawing {

struct Point {
    int X;
    int Y;

    Point();
    Point(int x, int y);
};

struct Size {
    int Width;
    int Height;

    Size();
    Size(int width, int height);
};

struct Rect {
    int X;
    int Y;
    int Width;
    int Height;

    Rect();
    Rect(int x, int y, int width, int height);
    Rect(Point const& origin, Size const& size);

    int GetRight() const;
    int GetBottom() const;
};

}