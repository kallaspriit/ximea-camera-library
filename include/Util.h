#ifndef UTIL_H
#define UTIL_H


class Util {
    public:
        static double millitime();
        static void yuvToRgb(int width, int height, unsigned char* yuv, unsigned char* rgb);
};

#endif // UTIL_H
