#include "Camera.h"

#include <iostream>

Camera::Camera() : opened(false), yuvInitialized(false) {
    image.size = sizeof(XI_IMG);
    image.bp = NULL;
    image.bp_size = 0;
    device = NULL;
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

const Camera::Frame& Camera::getFrame() {
    xiGetImage(device, 5000, &image);

    frame.data = (unsigned char*)image.bp;
    frame.size = image.bp_size;
    frame.number = image.nframe;
    frame.width = image.width;
    frame.height = image.height;
    frame.timestamp = (double)image.tsSec + (double)image.tsUSec / 1000000.0d;
    frame.fresh = frame.number != lastFrameNumber;

    lastFrameNumber = frame.number;

    return frame;
}

const Camera::FrameYUV& Camera::getFrameYUV() {
    xiGetImage(device, 5000, &image);

    frameYUV.data = (unsigned char*)image.bp;
    frameYUV.size = image.bp_size;
    frameYUV.number = image.nframe;
    frameYUV.width = image.width;
    frameYUV.height = image.height;
    frameYUV.timestamp = (double)image.tsSec + (double)image.tsUSec / 1000000.0d;
    frameYUV.fresh = frame.number != lastFrameNumber;

    if (!yuvInitialized) {
        frameYUV.strideY = frameYUV.width;
        frameYUV.strideU = (frameYUV.width + 1) / 2;
        frameYUV.strideV = (frameYUV.width + 1) / 2;

        frameYUV.dataY = new uint8[frameYUV.strideY * frameYUV.height];
        frameYUV.dataU = new uint8[frameYUV.strideU * (frameYUV.height + 1) / 2];
        frameYUV.dataV = new uint8[frameYUV.strideV * (frameYUV.height + 1) / 2];

        yuvInitialized = true;
    }

    lastFrameNumber = frame.number;

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

    return frameYUV;
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