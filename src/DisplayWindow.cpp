#include "DisplayWindow.h"
#include "Canvas.h"

#include <FL/Fl_Double_Window.H>

DisplayWindow::DisplayWindow(int width, int height) : width(width), height(height), window(NULL), canvas(NULL) {
    window = new Fl_Double_Window(width, height);
    canvas = new Canvas(width, height);
    window->end();
    window->show();
}

DisplayWindow::~DisplayWindow() {
    if (canvas != NULL) {
        delete canvas;
        canvas = NULL;
    }

    if (window != NULL) {
        delete window;
        window = NULL;
    }
}

void DisplayWindow::setImage(unsigned char* image) {
    canvas->setImage(image);
}
