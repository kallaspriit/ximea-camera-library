#ifndef VISION_H
#define VISION_H

#include "Blobber.h"

class Vision : public Blobber::MapFilter {
    public:
        Vision(int width, int height);
        ~Vision();

        void onFrameReceived(unsigned char* content);
        void filterMap(unsigned int* map);
        unsigned int* getColorMap() { return blobber->getMap(); }
        unsigned char* classify();

    private:
        Blobber* blobber;
        int width;
        int height;
        unsigned char* image;
        unsigned char* classification;
};

#endif // VISION_H
