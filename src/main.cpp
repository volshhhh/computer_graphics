#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include "ColorConverter.h"

// Global variables
cv::Mat display;
ColorModels currentColors;
bool trackbarChanged = false;
std::string lastChangedModel = "RGB";

// Trackbar callback functions for each model
void onRgbTrackbarChange(int, void*) {
    trackbarChanged = true;
    lastChangedModel = "RGB";
}

void onHsvTrackbarChange(int, void*) {
    trackbarChanged = true;
    lastChangedModel = "HSV";
}

void onCmykTrackbarChange(int, void*) {
    trackbarChanged = true;
    lastChangedModel = "CMYK";
}

void updateDisplay() {
    display = cv::Mat(700, 800, CV_8UC3, cv::Scalar(50, 50, 50));
    
    // Draw color palette
    ColorConverter::drawColorPalette(display, currentColors.rgb);
    
    // Draw color components
    ColorConverter::drawColorComponents(display, currentColors);
    
    // Draw instructions and current model
    cv::putText(display, "Color Models Converter - CMYK-RGB-HSV", cv::Point(50, 30), 
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    cv::putText(display, "Last changed: " + lastChangedModel, cv::Point(50, 60), 
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0), 1);
    cv::putText(display, "Use trackbars to adjust color components in any model", cv::Point(50, 600), 
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
    cv::putText(display, "Press 'r' to reset, 'q' or ESC to quit", cv::Point(50, 625), 
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
    
    cv::imshow("Color Models Converter", display);
}

void updateAllTrackbars(const ColorModels& colors) {
    // Update RGB trackbars
    cv::setTrackbarPos("Red", "Color Models Converter", colors.rgb[2]);
    cv::setTrackbarPos("Green", "Color Models Converter", colors.rgb[1]);
    cv::setTrackbarPos("Blue", "Color Models Converter", colors.rgb[0]);
    
    // Update HSV trackbars
    cv::setTrackbarPos("Hue", "Color Models Converter", static_cast<int>(colors.hsv[0]));
    cv::setTrackbarPos("Saturation%", "Color Models Converter", static_cast<int>(colors.hsv[1]));
    cv::setTrackbarPos("Value%", "Color Models Converter", static_cast<int>(colors.hsv[2]));
    
    // Update CMYK trackbars
    cv::setTrackbarPos("Cyan%", "Color Models Converter", static_cast<int>(colors.cmyk[0]));
    cv::setTrackbarPos("Magenta%", "Color Models Converter", static_cast<int>(colors.cmyk[1]));
    cv::setTrackbarPos("Yellow%", "Color Models Converter", static_cast<int>(colors.cmyk[2]));
    cv::setTrackbarPos("Black%", "Color Models Converter", static_cast<int>(colors.cmyk[3]));
}

int main() {
    // Initialize with white color
    currentColors = ColorConverter::updateFromRgb(cv::Vec3b(255, 255, 255));
    
    // Create window
    cv::namedWindow("Color Models Converter", cv::WINDOW_AUTOSIZE);
    cv::resizeWindow("Color Models Converter", 800, 700);
    
    // Create trackbars for RGB with specific callbacks
    int r = currentColors.rgb[2];
    int g = currentColors.rgb[1];
    int b = currentColors.rgb[0];
    cv::createTrackbar("Red", "Color Models Converter", &r, 255, onRgbTrackbarChange);
    cv::createTrackbar("Green", "Color Models Converter", &g, 255, onRgbTrackbarChange);
    cv::createTrackbar("Blue", "Color Models Converter", &b, 255, onRgbTrackbarChange);
    
    // Create trackbars for HSV with specific callbacks
    int h = static_cast<int>(currentColors.hsv[0]);
    int s = static_cast<int>(currentColors.hsv[1]);
    int v = static_cast<int>(currentColors.hsv[2]);
    cv::createTrackbar("Hue", "Color Models Converter", &h, 360, onHsvTrackbarChange);
    cv::createTrackbar("Saturation%", "Color Models Converter", &s, 100, onHsvTrackbarChange);
    cv::createTrackbar("Value%", "Color Models Converter", &v, 100, onHsvTrackbarChange);
    
    // Create trackbars for CMYK with specific callbacks
    int c = static_cast<int>(currentColors.cmyk[0]);
    int m = static_cast<int>(currentColors.cmyk[1]);
    int y = static_cast<int>(currentColors.cmyk[2]);
    int k_val = static_cast<int>(currentColors.cmyk[3]);
    cv::createTrackbar("Cyan%", "Color Models Converter", &c, 100, onCmykTrackbarChange);
    cv::createTrackbar("Magenta%", "Color Models Converter", &m, 100, onCmykTrackbarChange);
    cv::createTrackbar("Yellow%", "Color Models Converter", &y, 100, onCmykTrackbarChange);
    cv::createTrackbar("Black%", "Color Models Converter", &k_val, 100, onCmykTrackbarChange);
    
    updateDisplay();
    
    std::cout << "Color Models Converter Started" << std::endl;
    std::cout << "You can now adjust color components in ANY model (RGB, HSV, or CMYK)" << std::endl;
    std::cout << "The last changed model will be displayed on screen" << std::endl;
    
    while (true) {
        if (trackbarChanged) {
            // Get current trackbar positions
            int currentR = cv::getTrackbarPos("Red", "Color Models Converter");
            int currentG = cv::getTrackbarPos("Green", "Color Models Converter");
            int currentB = cv::getTrackbarPos("Blue", "Color Models Converter");
            
            int currentH = cv::getTrackbarPos("Hue", "Color Models Converter");
            int currentS = cv::getTrackbarPos("Saturation%", "Color Models Converter");
            int currentV = cv::getTrackbarPos("Value%", "Color Models Converter");
            
            int currentC = cv::getTrackbarPos("Cyan%", "Color Models Converter");
            int currentM = cv::getTrackbarPos("Magenta%", "Color Models Converter");
            int currentY = cv::getTrackbarPos("Yellow%", "Color Models Converter");
            int currentK = cv::getTrackbarPos("Black%", "Color Models Converter");
            
            // Update color based on which model was changed last
            if (lastChangedModel == "RGB") {
                currentColors = ColorConverter::updateFromRgb(cv::Vec3b(currentB, currentG, currentR));
            } 
            else if (lastChangedModel == "HSV") {
                cv::Vec3f hsvValues(currentH, static_cast<float>(currentS), static_cast<float>(currentV));
                currentColors = ColorConverter::updateFromHsv(hsvValues);
            }
            else if (lastChangedModel == "CMYK") {
                cv::Vec4f cmykValues(
                    static_cast<float>(currentC), 
                    static_cast<float>(currentM), 
                    static_cast<float>(currentY), 
                    static_cast<float>(currentK)
                );
                currentColors = ColorConverter::updateFromCmyk(cmykValues);
            }
            
            // Update all trackbars to reflect the new color
            updateAllTrackbars(currentColors);
            updateDisplay();
            trackbarChanged = false;
        }
        
        char key = cv::waitKey(30);
        if (key == 27 || key == 'q') { // ESC or Q to quit
            break;
        } else if (key == 'r') { // R to reset to white
            currentColors = ColorConverter::updateFromRgb(cv::Vec3b(255, 255, 255));
            updateAllTrackbars(currentColors);
            lastChangedModel = "RGB";
            updateDisplay();
        } else if (key == '1') { // Switch to RGB mode
            lastChangedModel = "RGB";
            updateDisplay();
        } else if (key == '2') { // Switch to HSV mode
            lastChangedModel = "HSV";
            updateDisplay();
        } else if (key == '3') { // Switch to CMYK mode
            lastChangedModel = "CMYK";
            updateDisplay();
        }
    }
    
    cv::destroyAllWindows();
    return 0;
}