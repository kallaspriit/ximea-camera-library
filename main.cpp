#include "DisplayWindow.h"
#include "Camera.h"
#include "Util.h"
#include "FpsCounter.h"

#include <iostream>
#include <cmath>

int main() {
    int downsampling = 2;
    int width = 1280 / downsampling;
    int height = 1024 / downsampling;

    DisplayWindow windowY(width, height, 1, "Y - component");
    DisplayWindow windowU(width / 2, height / 2, 1, "U - component");
    DisplayWindow windowV(width / 2, height / 2, 1, "V - component");
    DisplayWindow windowYUV(width, height, 3, "YUV combined");
    DisplayWindow windowRGB(width, height, 3, "YUV to RGB");

    Camera camera;
    FpsCounter fps;

    if (!camera.open()) {
        std::cout << "Opening camera failed";

        return -1;
    }

    //camera.setFormat(XI_RGB24);
    camera.setFormat(XI_RAW8);
    camera.setDownsampling(downsampling);

    camera.startAcquisition();

    unsigned char rgb[width * height * 3];

    while (DisplayWindow::windowsVisible()) {
        //const Camera::Frame& frame = camera.getFrame();
        const Camera::FrameYUV& frame = camera.getFrameYUV();

        if (!frame.fresh) {
            std::cout << "Repeating frame #" << frame.number << std::endl;

            continue;
        }

        Util::yuvToRgb(width, height, frame.dataYUV, rgb);

        //windowY.setImage(frame.data);
        windowY.setImage(frame.dataY);
        windowU.setImage(frame.dataU);
        windowV.setImage(frame.dataV);
        windowYUV.setImage(frame.dataYUV);
        windowRGB.setImage(rgb);

        fps.step();

        if (fps.isChanged()) {
            std::cout << "FPS: " << fps.getFps() << std::endl;
        }
    }

    return 0;
}
