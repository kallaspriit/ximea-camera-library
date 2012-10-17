#include "Camera.h"
#include "Util.h"

#include <iostream>

Camera::Camera() : opened(false), yuvInitialized(false) {
    image.size = sizeof(XI_IMG);
    image.bp = NULL;
    image.bp_size = 0;
    device = NULL;

    frameRaw.data = NULL;
    frameYUV.data = NULL;
    frameYUV.dataY = NULL;
    frameYUV.dataU = NULL;
    frameYUV.dataV = NULL;
}

Camera::~Camera() {
    close();
}

bool Camera::open(int serial) {
    DWORD deviceCount = 0;
    xiGetNumberDevices(&deviceCount);

    if (deviceCount == 0) {
        std::cout << "- Failed to detect any cameras" << std::endl;

        return false;
    }

    int sn = 0;

    bool found = false;

    for (unsigned int i = 0; i < deviceCount; i++) {
        xiOpenDevice(i, &device);

        xiGetParamInt(device, XI_PRM_DEVICE_SN, &sn);
        std::cout << "! Found camera with serial number: " << sn << std::endl;

        if (serial == 0 || serial == sn) {
            found = true;
        } else {
            xiCloseDevice(device);
        }
    }

    if (!found) {
        std::cout << "- No camera with serial #" << serial << " could be found" << std::endl;

        return false;
    }

    xiSetParamInt(device, XI_PRM_EXPOSURE, 16000);
    //xiSetParamInt(device, XI_PRM_IMAGE_DATA_FORMAT, XI_MONO8);
    xiSetParamInt(device, XI_PRM_IMAGE_DATA_FORMAT, XI_RGB24);
    //xiSetParamInt(device, XI_PRM_BUFFER_POLICY, XI_BP_UNSAFE);
    //xiSetParamInt(device, XI_PRM_FRAMERATE, 60);
    //xiSetParamInt(device, XI_PRM_DOWNSAMPLING, 2); // @TEMP
    //xiSetParamInt(device, XI_PRM_DOWNSAMPLING_TYPE, XI_BINNING);
    //xiSetParamFloat(device, XI_PRM_GAIN, 5.0f);
    //xiSetParamInt(device, XI_PRM_ACQ_BUFFER_SIZE, 70*1000*1000);
    xiSetParamInt(device, XI_PRM_BUFFERS_QUEUE_SIZE, 1);
    xiSetParamInt(device, XI_PRM_RECENT_FRAME, 1);
    xiSetParamInt(device, XI_PRM_AUTO_WB, 0);
    //xiSetParamFloat(device, XI_PRM_WB_KR, 1.0f);
    //xiSetParamFloat(device, XI_PRM_WB_KG, 1.0f);
    //xiSetParamFloat(device, XI_PRM_WB_KB, 1.0f);
    //xiSetParamFloat(device, XI_PRM_GAMMAY, 1.0f);
    //xiSetParamFloat(device, XI_PRM_GAMMAC, 1.0f);
    xiSetParamFloat(device, XI_PRM_SHARPNESS, 0.0f);
    xiSetParamInt(device, XI_PRM_AEAG, 0);
    //xiSetParamInt(device, XI_PRM_BPC, 1); // fixes bad pixel
    //xiSetParamInt(device, XI_PRM_HDR, 1);

    opened = true;

    return true;
}

void Camera::startAcquisition() {
    if (!opened) {
        return;
    }

    xiStartAcquisition(device);
}

const Camera::FrameRaw* Camera::getFrame() {
    xiGetImage(device, 100, &image);

    if (image.bp == NULL) {
        return NULL;
    }

    frameRaw.data = (unsigned char*)image.bp;
    frameRaw.size = image.bp_size;
    frameRaw.number = image.nframe;
    frameRaw.width = image.width;
    frameRaw.height = image.height;
    frameRaw.timestamp = (double)image.tsSec + (double)image.tsUSec / 1000000.0d;
    frameRaw.fresh = frameRaw.number != lastFrameNumber;

    lastFrameNumber = frameRaw.number;

    return &frameRaw;
}

/*
const Camera::FrameYUV& Camera::getFrameYUV() {
    xiGetImage(device, 100, &image);

    frameYUV.data = (unsigned char*)image.bp;
    frameYUV.size = image.bp_size;
    frameYUV.number = image.nframe;
    frameYUV.width = image.width;
    frameYUV.height = image.height;
    frameYUV.timestamp = (double)image.tsSec + (double)image.tsUSec / 1000000.0d;
    frameYUV.fresh = frameYUV.number != lastFrameNumber;

    if (!yuvInitialized) {
        frameYUV.strideY = frameYUV.width;
        frameYUV.strideU = (frameYUV.width + 1) / 2;
        frameYUV.strideV = (frameYUV.width + 1) / 2;

        frameYUV.dataY = new uint8[frameYUV.width * frameYUV.height];
        frameYUV.dataU = new uint8[(frameYUV.width / 2) * (frameYUV.height / 2)];
        frameYUV.dataV = new uint8[(frameYUV.width / 2) * (frameYUV.height / 2)];
        frameYUV.dataYUV = new uint8[frameYUV.width * frameYUV.height * 3];

        yuvInitialized = true;
    }

    lastFrameNumber = frameYUV.number;

    libyuv::BayerRGGBToI420(
        frameYUV.data,
        frameYUV.width,
        frameYUV.dataY,
        frameYUV.strideY,
        frameYUV.dataU,
        frameYUV.strideU,
        frameYUV.dataV,
        frameYUV.strideV,
        frameYUV.width,
        frameYUV.height
    );

    int row;
    int col;
    int indexUV;

    for (int i = 0; i < frameYUV.width * frameYUV.height; i++) {
        row = i / frameYUV.width;
        col = i - row * frameYUV.width;
        indexUV = (row / 2) * (frameYUV.width / 2) + (col / 2);

        frameYUV.dataYUV[i * 3] = frameYUV.dataY[i];
        frameYUV.dataYUV[i * 3 + 1] = frameYUV.dataU[indexUV];
        frameYUV.dataYUV[i * 3 + 2] = frameYUV.dataV[indexUV];
    }

    return frameYUV;
}
*/

const Camera::FrameYUYV* Camera::getFrameYUYV() {
    //double s = Util::millitime();

    xiGetImage(device, 100, &image);

    if (image.bp == NULL) {
        return NULL;
    }

    //std::cout << "Get: " << (Util::millitime() - s) << std::endl;

    frameYUV.data = (unsigned char*)image.bp;
    frameYUV.size = image.bp_size;
    frameYUV.number = image.nframe;
    frameYUV.width = image.width;
    frameYUV.height = image.height;
    frameYUV.timestamp = (double)image.tsSec + (double)image.tsUSec / 1000000.0d;
    frameYUV.fresh = frameYUV.number != lastFrameNumber;

    if (!yuvInitialized) {
        frameYUV.strideY = frameYUV.width;
        frameYUV.strideU = (frameYUV.width + 1) / 2;
        frameYUV.strideV = (frameYUV.width + 1) / 2;

        frameYUV.dataY = new uint8[frameYUV.width * frameYUV.height];
        frameYUV.dataU = new uint8[(frameYUV.width / 2) * (frameYUV.height / 2)];
        frameYUV.dataV = new uint8[(frameYUV.width / 2) * (frameYUV.height / 2)];
        frameYUV.dataYUYV = new uint8[frameYUV.width * frameYUV.height * 3];

        yuvInitialized = true;
    }

    lastFrameNumber = frameYUV.number;

    //s = Util::millitime();

    libyuv::BayerRGGBToI420(
        frameYUV.data,
        frameYUV.width,
        frameYUV.dataY,
        frameYUV.strideY,
        frameYUV.dataU,
        frameYUV.strideU,
        frameYUV.dataV,
        frameYUV.strideV,
        frameYUV.width,
        frameYUV.height
    );

    //std::cout << "RGGB > I420: " << (Util::millitime() - s) << std::endl;

    //s = Util::millitime();

    int row;
    int col;
    int indexUV;
    int elements = frameYUV.width * frameYUV.height;
    int halfWidth = frameYUV.width / 2;
    bool alt = false;

    for (int i = 0; i < elements; i++) {
        row = i / frameYUV.width;
        col = i - row * frameYUV.width;
        indexUV = (row >> 1) * halfWidth + (col >> 1);

        frameYUV.dataYUYV[i << 1] = frameYUV.dataY[i];
        frameYUV.dataYUYV[(i << 1) + 1] = alt ? frameYUV.dataV[indexUV] : frameYUV.dataU[indexUV];

        alt = !alt;
    }

    //std::cout << "I420 > YUYV: " << (Util::millitime() - s) << std::endl;

    return &frameYUV;
}

void Camera::stopAcquisition() {
    if (!opened) {
        return;
    }

    xiStopAcquisition(device);
}

void Camera::close() {
    if (opened) {
        xiCloseDevice(device);

        device = NULL;
    }
}

std::string Camera::getStringParam(const char* name) {
    char stringParam[254];

    xiGetParamString(device, name, stringParam, sizeof(stringParam));

    return std::string(stringParam);
}

int Camera::getIntParam(const char* name) {
    int intParam = 0;

    xiGetParamInt(device, name, &intParam);

    return intParam;
}

float Camera::getFloatParam(const char* name) {
    float floatParam = 0;

    xiGetParamFloat(device, name, &floatParam);

    return floatParam;
}

void Camera::setStringParam(const char* name, std::string value) {
    xiSetParamString(device, name, (void*)value.c_str(), value.length());
}

void Camera::setIntParam(const char* name, int value) {
    xiSetParamInt(device, name, value);
}

void Camera::setFloatParam(const char* name, float value) {
    xiSetParamFloat(device, name, value);
}
