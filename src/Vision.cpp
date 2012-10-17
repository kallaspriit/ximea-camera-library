#include "Vision.h"

#include <iostream>

Vision::Vision(int width, int height) : blobber(NULL), width(width), height(height), image(NULL), classification(NULL) {
    blobber = new Blobber();

    blobber->initialize(width, height);
    //blobber->loadOptions(const_cast<char* >("options.txt"));
    //blobber->enable(BLOBBER_DENSITY_MERGE);
    blobber->setMapFilter(this);

    /*
    [Colors]
    (255,128,  0) 0.5000 11 Ball
    (255,255,  0) 0.4000 1 YellowGoal
    (  0,  0,255) 0.4000 1 BlueGoal
    (255,255,255) 0.5000 10 White
    (  0,255,  0) 0.5000 1 Green

    [Thresholds]
    ( 32:125, 34: 88,174:255)
    ( 76:150,  8: 73,115:157)
    (  0: 37,145:171,110:144)
    (106:213, 74:189,101:137)
    ( 49:155, 81:140, 25: 96)
    */

    blobber->addColor(
        "ball",
        255, 128, 0,
        30, 35,
        110, 130,
        110, 130
    );
    blobber->addColor(
        "white",
        255, 255, 255,
        106, 213,
        74, 189,
        101, 137
    );
    /*blobber->getColor("ball")->updateThresholds(
        106, 213,
        74, 189,
        101, 137
    );*/

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

unsigned char* Vision::classify() {
    if (image == NULL) {
        return NULL;
    }

    if (classification == NULL) {
        classification = new unsigned char[width * height * 3];
    }

    blobber->classify((Blobber::Rgb*)classification, (Blobber::Pixel*)image);

    return classification;
}
