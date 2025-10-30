#include "../include/ColorConverter.h"
#include <iostream>
#include <opencv2/opencv.hpp>

// Global variables
cv::Mat display;
ColorModels currentColors;
bool trackbarChanged = false;

// Trackbar callback function
void onTrackbarChange(int, void *) { trackbarChanged = true; }

void updateDisplay() {
  display = cv::Mat(600, 600, CV_8UC3, cv::Scalar(50, 50, 50));

  // Draw color palette
  ColorConverter::drawColorPalette(display, currentColors.rgb);

  // Draw color components
  ColorConverter::drawColorComponents(display, currentColors);

  // Draw instructions
  cv::putText(display, "Color Models Converter - CMYK-RGB-HSV",
              cv::Point(50, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7,
              cv::Scalar(255, 255, 255), 1);
  cv::putText(display, "Use trackbars to adjust color components",
              cv::Point(50, 500), cv::FONT_HERSHEY_SIMPLEX, 0.5,
              cv::Scalar(200, 200, 200), 1);

  cv::imshow("Color Models Converter", display);
}

int main() {
  // Initialize with white color
  currentColors = ColorConverter::updateFromRgb(cv::Vec3b(255, 255, 255));

  // Create window
  cv::namedWindow("Color Models Converter", cv::WINDOW_AUTOSIZE);

  // Create trackbars for RGB
  int r = currentColors.rgb[2];
  int g = currentColors.rgb[1];
  int b = currentColors.rgb[0];
  cv::createTrackbar("Red", "Color Models Converter", &r, 255,
                     onTrackbarChange);
  cv::createTrackbar("Green", "Color Models Converter", &g, 255,
                     onTrackbarChange);
  cv::createTrackbar("Blue", "Color Models Converter", &b, 255,
                     onTrackbarChange);

  // Create trackbars for HSV
  int h = static_cast<int>(currentColors.hsv[0]);
  int s = static_cast<int>(currentColors.hsv[1]);
  int v = static_cast<int>(currentColors.hsv[2]);
  cv::createTrackbar("Hue", "Color Models Converter", &h, 360,
                     onTrackbarChange);
  cv::createTrackbar("Saturation%", "Color Models Converter", &s, 100,
                     onTrackbarChange);
  cv::createTrackbar("Value%", "Color Models Converter", &v, 100,
                     onTrackbarChange);

  // Create trackbars for CMYK
  int c = static_cast<int>(currentColors.cmyk[0]);
  int m = static_cast<int>(currentColors.cmyk[1]);
  int y = static_cast<int>(currentColors.cmyk[2]);
  int k = static_cast<int>(currentColors.cmyk[3]);
  cv::createTrackbar("Cyan%", "Color Models Converter", &c, 100,
                     onTrackbarChange);
  cv::createTrackbar("Magenta%", "Color Models Converter", &m, 100,
                     onTrackbarChange);
  cv::createTrackbar("Yellow%", "Color Models Converter", &y, 100,
                     onTrackbarChange);
  cv::createTrackbar("Black%", "Color Models Converter", &k, 100,
                     onTrackbarChange);

  updateDisplay();

  std::cout << "Color Models Converter Started" << std::endl;
  std::cout << "Adjust any trackbar to see real-time color conversion"
            << std::endl;

  while (true) {
    if (trackbarChanged) {
      // Get current trackbar positions
      int currentR = cv::getTrackbarPos("Red", "Color Models Converter");
      int currentG = cv::getTrackbarPos("Green", "Color Models Converter");
      int currentB = cv::getTrackbarPos("Blue", "Color Models Converter");

      int currentH = cv::getTrackbarPos("Hue", "Color Models Converter");
      int currentS =
          cv::getTrackbarPos("Saturation%", "Color Models Converter");
      int currentV = cv::getTrackbarPos("Value%", "Color Models Converter");

      int currentC = cv::getTrackbarPos("Cyan%", "Color Models Converter");
      int currentM = cv::getTrackbarPos("Magenta%", "Color Models Converter");
      int currentY = cv::getTrackbarPos("Yellow%", "Color Models Converter");
      int currentK = cv::getTrackbarPos("Black%", "Color Models Converter");

      // Update color based on which trackbar was changed
      // This is a simplified approach - in a more complex implementation
      // you would track which specific trackbar was changed
      currentColors = ColorConverter::updateFromRgb(
          cv::Vec3b(currentB, currentG, currentR));

      // Update other trackbars to match
      cv::setTrackbarPos("Hue", "Color Models Converter",
                         static_cast<int>(currentColors.hsv[0]));
      cv::setTrackbarPos("Saturation%", "Color Models Converter",
                         static_cast<int>(currentColors.hsv[1]));
      cv::setTrackbarPos("Value%", "Color Models Converter",
                         static_cast<int>(currentColors.hsv[2]));

      cv::setTrackbarPos("Cyan%", "Color Models Converter",
                         static_cast<int>(currentColors.cmyk[0]));
      cv::setTrackbarPos("Magenta%", "Color Models Converter",
                         static_cast<int>(currentColors.cmyk[1]));
      cv::setTrackbarPos("Yellow%", "Color Models Converter",
                         static_cast<int>(currentColors.cmyk[2]));
      cv::setTrackbarPos("Black%", "Color Models Converter",
                         static_cast<int>(currentColors.cmyk[3]));

      updateDisplay();
      trackbarChanged = false;
    }

    char key = cv::waitKey(30);
    if (key == 27 || key == 'q') { // ESC or Q to quit
      break;
    } else if (key == 'r') { // R to reset to white
      currentColors = ColorConverter::updateFromRgb(cv::Vec3b(255, 255, 255));
      cv::setTrackbarPos("Red", "Color Models Converter", 255);
      cv::setTrackbarPos("Green", "Color Models Converter", 255);
      cv::setTrackbarPos("Blue", "Color Models Converter", 255);
      updateDisplay();
    }
  }

  cv::destroyAllWindows();
  return 0;
}