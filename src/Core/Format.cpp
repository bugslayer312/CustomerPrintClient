#include "Format.h"

#include <cstdarg>
#include <memory>

std::string Format(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size = 1024;
    std::unique_ptr<char[]> buf(new char[size]); 
    while (true)
    {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(buf.get(), size, fmt, args2);
        if ((res >= 0) && (res < static_cast<int>(size)))
        {
            va_end(args);
            va_end(args2);
            return std::string(buf.get());
        }
        if (res < 0)
            size *= 2;
        else
            size = static_cast<size_t>(res) + 1;
        buf.reset(new char[size]);
        va_end(args2);
    }
    return std::string();
}