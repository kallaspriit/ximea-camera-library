#ifndef DISPLAY_H
#define DISPLAY_H

#include <FL/Fl.H>

class Fl_Double_Window;
class Canvas;
class Command;

class DisplayWindow {
    public:
        DisplayWindow(int width, int height);
        ~DisplayWindow();

        void setImage(unsigned char* image);
        static bool windowsVisible() { return Fl::check() != 0; }

    private:
        int width;
        int height;
        Fl_Double_Window* window;
        Canvas* canvas;
};

#endif // DISPLAY_H
