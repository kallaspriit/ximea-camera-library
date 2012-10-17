#include "Vision.h"

#include <iostream>

Vision::Vision(int width, int height) : blobber(NULL), width(width), height(height), image(NULL), classification(NULL) {
    blobber = new Blobber();

    blobber->initialize(width, height);
    //blobber->loadOptions(const_cast<char* >("options.txt"));
    //blobber->enable(BLOBBER_DENSITY_MERGE);
    blobber->setMapFilter(this);

    blobber->addColor(
        255, 128, 0,
        "ball",
        32, 125,
        34, 88,
        174, 255
    );
    blobber->addColor(
        255, 255, 255,
        "white",
        106, 213,
        74, 189,
        101, 137
    );

    std::cout << "Color count: " << blobber->getColorCount() << std::endl;
}

Vision::~Vision() {
    if (blobber != NULL) {
        delete blobber;

        blobber = NULL;
    }
}

void Vision::onFrameReceived(unsigned char* content) {
    image = content;

    blobber->processFrame((Blobber::Pixel*)content);
}

void Vision::filterMap(unsigned int* map) {

}

unsigned char* Vision::getClassification() {
    if (image == NULL) {
        return NULL;
    }

    if (classification == NULL) {
        classification = new unsigned char[width * height * 3];
    }

    blobber->getClassification((Blobber::Rgb*)classification, (Blobber::Pixel*)image);

    return classification;
}
