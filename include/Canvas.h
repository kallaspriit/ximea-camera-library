#ifndef CANVAS_H
#define CANVAS_H

#include <FL/Fl_Widget.H>
#include <list>

class Canvas : public Fl_Widget
{
    public:
        Canvas(int width, int height);

        void setImage(unsigned char *image);
        void setStride(int stride) { this->stride = stride; }
        void draw();

    private:
        int width;
        int height;
        int stride;
        unsigned char* image;
};

#endif // CANVAS_H
