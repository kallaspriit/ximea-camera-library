#include "DisplayWindow.h"
#include "Camera.h"
#include "Util.h"
#include "FpsCounter.h"

#include <iostream>

int main() {
    int downsampling = 2;
    int width = 1280 / downsampling;
    int height = 1024 / downsampling;

    DisplayWindow window(width, height);
    Camera camera;
    FpsCounter fps;

    if (!camera.open()) {
        std::cout << "Opening camera failed";

        return -1;
    }

    camera.setFormat(XI_RGB24);
    //camera.setFormat(XI_RAW8);
    camera.setDownsampling(downsampling);

    camera.startAcquisition();

    while (DisplayWindow::windowsVisible()) {
        const Camera::Frame& frame = camera.getFrame();
        //const Camera::FrameYUV& frame = camera.getFrameYUV();

        if (!frame.fresh) {
            //std::cout << "Repeating frame #" << frame.number << std::endl;

            continue;
        }

        window.setImage(frame.data);
        //window.setImage(frame.dataY);

        fps.step();

        if (fps.isChanged()) {
            std::cout << "FPS: " << fps.getFps() << std::endl;
        }
    }

    return 0;
}
