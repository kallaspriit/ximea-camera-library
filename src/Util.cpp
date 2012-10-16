#include "Util.h"

#include <sys/time.h>

double Util::millitime()
{
    timeval timeOfDay;

    gettimeofday(&timeOfDay, 0);

    long seconds  = timeOfDay.tv_sec;
    long useconds = timeOfDay.tv_usec;

    return (double)seconds + (double)useconds / 1000000.0d;
}

/*
void Util::yuvToRgb(int width, int height, unsigned char *data, unsigned char *out) {
    int w2 = width / 2;

    for(int x=0; x<w2; x++) {
        for(int y=0; y<height; y++) {
            int i = (y*w2+x)*4;
            int y0 = data[i];
            int u = data[i+1];
            int y1 = data[i+2];
            int v = data[i+3];

            int r = y0 + (1.370705 * (v-128));
            int g = y0 - (0.698001 * (v-128)) - (0.337633 * (u-128));
            int b = y0 + (1.732446 * (u-128));

            if(r > 255) r = 255;
            if(g > 255) g = 255;
            if(b > 255) b = 255;
            if(r < 0) r = 0;
            if(g < 0) g = 0;
            if(b < 0) b = 0;

            i = (y*width+2*x)*3;

            out[i] = (unsigned char)(r);
            out[i+1] = (unsigned char)(g);
            out[i+2] = (unsigned char)(b);

            r = y1 + (1.370705 * (v-128));
            g = y1 - (0.698001 * (v-128)) - (0.337633 * (u-128));
            b = y1 + (1.732446 * (u-128));

            if(r > 255) r = 255;
            if(g > 255) g = 255;
            if(b > 255) b = 255;
            if(r < 0) r = 0;
            if(g < 0) g = 0;
            if(b < 0) b = 0;

            out[i+3] = (unsigned char)(r);
            out[i+4] = (unsigned char)(g);
            out[i+5] = (unsigned char)(b);
        }
    }
}
*/

void Util::yuvToRgb(int width, int height, unsigned char* yuv, unsigned char* rgb) {
    int x, y, pos, sy, su, sv, r, g, b;

    for(y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = width * y + x;

            sy = yuv[pos * 3];
            su = yuv[pos * 3 + 1];
            sv = yuv[pos * 3 + 2];

            r = sy + (1.370705 * (sv - 128));
            g = sy - (0.698001 * (sv - 128)) - (0.337633 * (su - 128));
            b = sy + (1.732446 * (su - 128));

            if(r > 255) r = 255;
            if(g > 255) g = 255;
            if(b > 255) b = 255;
            if(r < 0) r = 0;
            if(g < 0) g = 0;
            if(b < 0) b = 0;

            rgb[pos * 3] = r;
            rgb[pos * 3 + 1] = g;
            rgb[pos * 3 + 2] = b;
        }
    }
}
