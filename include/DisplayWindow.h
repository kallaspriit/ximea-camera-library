#ifndef DISPLAY_H
#define DISPLAY_H

#include "Canvas.h"
#include <FL/Fl.H>
#include <string>

class Fl_Double_Window;
class Canvas;
class Command;

class DisplayWindow {
    public:
        DisplayWindow(int width, int height, int delta = 3, std::string name = "Window");
        ~DisplayWindow();

        void setImage(unsigned char* image);
        void setDelta(int delta) { canvas->setDelta(delta); }
        static bool windowsVisible() { return Fl::check() != 0; }

    private:
        int width;
        int height;
        Fl_Double_Window* window;
        Canvas* canvas;
};

#endif // DISPLAY_H
