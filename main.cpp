#include "DisplayWindow.h"
#include "Camera.h"
#include "Util.h"
#include "FpsCounter.h"
#include "Vision.h"
#include "jpge.h"

#include <iostream>
#include <cmath>

#define USE_RGB

int main() {
    int downsampling = 2;
    int width = 1280 / downsampling;
    int height = 1024 / downsampling;

    DisplayWindow windowY(width, height, 1, "Y - component");
    DisplayWindow windowU(width / 2, height / 2, 1, "U - component");
    DisplayWindow windowV(width / 2, height / 2, 1, "V - component");
    DisplayWindow windowYUV(width, height, 2, "YUV combined");
    DisplayWindow windowClassification(width, height, 3, "Color classification");

    #ifdef USE_RGB
    DisplayWindow windowRGB(width, height, 3, "YUV to RGB");
    #endif

    Camera camera;
    FpsCounter fps;
    Vision vision(width, height);

    if (!camera.open()) {
        std::cout << "Opening camera failed";

        return -1;
    }

    //camera.setFormat(XI_RGB24);
    camera.setFormat(XI_RAW8);
    camera.setDownsampling(downsampling);

    camera.startAcquisition();

    #ifdef USE_RGB
    unsigned char rgb[width * height * 3];

    //char filename[256];
    //int jpegBufferSize = 1024 * 100;
    //unsigned char jpegBuffer[jpegBufferSize];
    #endif

    while (DisplayWindow::windowsVisible()) {
        //const Camera::Frame& frame = camera.getFrame();
        //const Camera::FrameYUV& frame = camera.getFrameYUV();
        const Camera::FrameYUV& frame = camera.getFrameYUYV();

        if (!frame.fresh) {
            std::cout << "Repeating frame #" << frame.number << std::endl;

            continue;
        }

        windowY.setImage(frame.dataY);
        windowU.setImage(frame.dataU);
        windowV.setImage(frame.dataV);
        windowYUV.setImage(frame.dataYUV);

        vision.onFrameReceived(frame.dataYUV);

        unsigned char* classification = vision.classify();

        windowClassification.setImage(classification);

        #ifdef USE_RGB
        //Util::yuvToRgb(width, height, frame.dataYUV, rgb);
        Util::yuyvToRgb(width, height, frame.dataYUV, rgb);
        windowRGB.setImage(rgb);

        /*
        sprintf(filename, "frame-%d.jpg", frame.number);

        //jpge::compress_image_to_jpeg_file(filename, width, height, 3, rgb);

        std::cout << "Real jpeg size: " << jpegBufferSize << std::endl;

        jpegBufferSize = 1024 * 100;

        std::cout << "Jpeg buffer size: " << jpegBufferSize;

        jpge::compress_image_to_jpeg_file_in_memory(jpegBuffer, jpegBufferSize, width, height, 3, rgb);
        std::string base64img = Util::base64Encode(jpegBuffer, jpegBufferSize);

        std::cout << "; real jpeg size: " << jpegBufferSize << std::endl;
        std::cout << base64img << std::endl;
        */

        #endif

        fps.step();

        if (fps.isChanged()) {
            std::cout << "FPS: " << fps.getFps() << std::endl;
        }
    }

    return 0;
}
