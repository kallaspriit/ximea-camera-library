#include "DisplayWindow.h"

#include <FL/Fl_Double_Window.H>

DisplayWindow::DisplayWindow(int width, int height, int delta, std::string name) : width(width), height(height), window(NULL), canvas(NULL) {
    window = new Fl_Double_Window(width, height, name.c_str());
    canvas = new Canvas(width, height, delta);
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
