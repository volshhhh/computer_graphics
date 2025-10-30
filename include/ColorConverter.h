#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

struct ColorModels {
    cv::Vec3b rgb;
    cv::Vec3f hsv;
    cv::Vec4f cmyk;
};

class ColorConverter {
public:
    static cv::Vec3f rgbToHsv(const cv::Vec3b& rgb);
    static cv::Vec3b hsvToRgb(const cv::Vec3f& hsv);
    static cv::Vec4f rgbToCmyk(const cv::Vec3b& rgb);
    static cv::Vec3b cmykToRgb(const cv::Vec4f& cmyk);
    
    static ColorModels updateFromRgb(const cv::Vec3b& rgb);
    static ColorModels updateFromHsv(const cv::Vec3f& hsv);
    static ColorModels updateFromCmyk(const cv::Vec4f& cmyk);
    
    static void drawColorPalette(cv::Mat& image, const cv::Vec3b& color);
    static void drawColorComponents(cv::Mat& image, const ColorModels& colors);
};