#include "Canvas.h"

#include <FL/fl_draw.H>

Canvas::Canvas(int width, int height) : Fl_Widget(0, 0, width, height, "Canvas"), width(width), height(height) {
    stride = 3;
}

void Canvas::setImage(unsigned char *image) {
    this->image = image;

    redraw();
}

void Canvas::draw() {
    if (image != NULL) {
        fl_draw_image(image, 0, 0, width, height, stride);
    }
}
