#ifndef CAMERA_H
#define CAMERA_H

#include "xiApi.h"
#include "xiExt.h"
#include "libyuv.h"

#include <string>

class Camera {
    public:
        struct YUYV {
            int y1, u, y2, v;
        };

        struct FrameRaw {
            unsigned char* data;
            int size;
            int width;
            int height;
            int number;
            bool fresh;
            double timestamp;
        };

        struct FrameYUYV : public FrameRaw {
            int strideY;
            int strideU;
            int strideV;
            unsigned char* dataY;
            unsigned char* dataU;
            unsigned char* dataV;
            unsigned char* dataYUYV;

            YUYV getPixelAt(int x, int y) const {
                int pos = y * strideY + x;
                YUYV pixel;

                pixel.y1 = dataYUYV[pos];
                pixel.u = dataYUYV[pos + 1];
                pixel.y2 = dataYUYV[pos + 2];
                pixel.v = dataYUYV[pos + 3];

                return pixel;
            }
        };

        Camera();
        ~Camera();

        bool open(int serial = 0);
        void startAcquisition();
        void stopAcquisition();
        const FrameRaw* getFrame();
        //const FrameYUV& getFrameYUV();
        const FrameYUYV* getFrameYUYV();
        void close();

        std::string getName() { return getStringParam(XI_PRM_DEVICE_NAME); }
        std::string getDeviceType() { return getStringParam(XI_PRM_DEVICE_TYPE); }
        std::string getApiVersion() { return getStringParam(XI_PRM_API_VERSION XI_PRM_INFO); }
        std::string getDriverVersion() { return getStringParam(XI_PRM_DRV_VERSION XI_PRM_INFO); }
        int getSerialNumber() { return getIntParam(XI_PRM_DEVICE_SN); }
        bool supportsColor() { return getIntParam(XI_PRM_IMAGE_IS_COLOR) == 1; }
        int getAvailableBandwidth() { return getIntParam(XI_PRM_AVAILABLE_BANDWIDTH); }

        void setFormat(int format) { setIntParam(XI_PRM_IMAGE_DATA_FORMAT, format); }
        void setExposure(int microseconds) { setIntParam(XI_PRM_EXPOSURE, microseconds); }
        void setGain(float value) { setIntParam(XI_PRM_GAIN, value); }
        void setDownsampling(int times) { setIntParam(XI_PRM_DOWNSAMPLING, times); }
        void setWhiteBalanceRed(float value) { setFloatParam(XI_PRM_WB_KR, value); }
        void setWhiteBalanceGreen(float value) { setFloatParam(XI_PRM_WB_KG, value); }
        void setWhiteBalanceBlue(float value) { setFloatParam(XI_PRM_WB_KB, value); }
        void setLuminosityGamma(float value) { setFloatParam(XI_PRM_GAMMAY, value); }
        void setChromaticityGamma(float value) { setFloatParam(XI_PRM_GAMMAC, value); }

        std::string getStringParam(const char* name);
        int getIntParam(const char* name);
        float getFloatParam(const char* name);

        void setStringParam(const char* name, std::string value);
        void setIntParam(const char* name, int value);
        void setFloatParam(const char* name, float value);

    private:
        XI_IMG image;
        FrameRaw frameRaw;
        FrameYUYV frameYUV;
        HANDLE device;
        bool opened;
        bool yuvInitialized;
        int lastFrameNumber;
};

#endif // CAMERA_H
