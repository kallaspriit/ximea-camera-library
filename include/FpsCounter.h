#ifndef FPSCOUNTER_H
#define FPSCOUNTER_H

class FpsCounter {
    public:
        FpsCounter(int interval = 60);
        void step();
        bool isChanged();
        int getFps();

    private:
        int interval;
        double startTime;
        bool changed;
        int frames;
        int fps;

};

#endif // FPSCOUNTER_H
