#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util {
    public:
        static double millitime();
        static void yuvToRgb(int width, int height, unsigned char* yuv, unsigned char* rgb);
        static void yuyvToRgb(int width, int height, unsigned char* yuv, unsigned char* rgb);
        static std::string base64Encode(const unsigned char* data, unsigned int len);

    private:
        static const std::string base64Chars;
};

#endif // UTIL_H
