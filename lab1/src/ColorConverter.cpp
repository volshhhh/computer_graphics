#include "ColorConverter.h"
#include <algorithm>
#include <cmath>

cv::Vec3f ColorConverter::rgbToHsv(const cv::Vec3b& rgb) {
    float r = rgb[2] / 255.0f;
    float g = rgb[1] / 255.0f;
    float b = rgb[0] / 255.0f;
    
    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    float delta = maxVal - minVal;
    
    float h = 0, s = 0, v = maxVal;
    
    if (delta > 0.0001f) {
        s = delta / maxVal;
        
        if (maxVal == r) {
            h = 60 * fmod(((g - b) / delta), 6.0f);
        } else if (maxVal == g) {
            h = 60 * (((b - r) / delta) + 2);
        } else {
            h = 60 * (((r - g) / delta) + 4);
        }
        
        if (h < 0) h += 360;
    }
    
    return cv::Vec3f(h, s * 100, v * 100);
}

cv::Vec3b ColorConverter::hsvToRgb(const cv::Vec3f& hsv) {
    float h = hsv[0];
    float s = hsv[1] / 100.0f;
    float v = hsv[2] / 100.0f;
    
    if (s < 0.001f) {
        // Grayscale
        uchar gray = static_cast<uchar>(v * 255);
        return cv::Vec3b(gray, gray, gray);
    }
    
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    
    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return cv::Vec3b(
        static_cast<uchar>((b + m) * 255),
        static_cast<uchar>((g + m) * 255),
        static_cast<uchar>((r + m) * 255)
    );
}

cv::Vec4f ColorConverter::rgbToCmyk(const cv::Vec3b& rgb) {
    float r = rgb[2] / 255.0f;
    float g = rgb[1] / 255.0f;
    float b = rgb[0] / 255.0f;
    
    float k = 1 - std::max({r, g, b});
    
    if (k > 0.999f) {
        // Pure black
        return cv::Vec4f(0, 0, 0, 100);
    }
    
    float c = (1 - r - k) / (1 - k);
    float m = (1 - g - k) / (1 - k);
    float y = (1 - b - k) / (1 - k);
    
    return cv::Vec4f(c * 100, m * 100, y * 100, k * 100);
}

cv::Vec3b ColorConverter::cmykToRgb(const cv::Vec4f& cmyk) {
    float c = cmyk[0] / 100.0f;
    float m = cmyk[1] / 100.0f;
    float y = cmyk[2] / 100.0f;
    float k = cmyk[3] / 100.0f;
    
    float r = (1 - c) * (1 - k);
    float g = (1 - m) * (1 - k);
    float b = (1 - y) * (1 - k);
    
    return cv::Vec3b(
        static_cast<uchar>(b * 255),
        static_cast<uchar>(g * 255),
        static_cast<uchar>(r * 255)
    );
}

ColorModels ColorConverter::updateFromRgb(const cv::Vec3b& rgb) {
    ColorModels result;
    result.rgb = rgb;
    result.hsv = rgbToHsv(rgb);
    result.cmyk = rgbToCmyk(rgb);
    return result;
}

ColorModels ColorConverter::updateFromHsv(const cv::Vec3f& hsv) {
    ColorModels result;
    result.rgb = hsvToRgb(hsv);
    result.hsv = hsv;
    result.cmyk = rgbToCmyk(result.rgb);
    return result;
}

ColorModels ColorConverter::updateFromCmyk(const cv::Vec4f& cmyk) {
    ColorModels result;
    result.rgb = cmykToRgb(cmyk);
    result.hsv = rgbToHsv(result.rgb);
    result.cmyk = cmyk;
    return result;
}

void ColorConverter::drawColorPalette(cv::Mat& image, const cv::Vec3b& color) {
    cv::rectangle(image, cv::Rect(50, 80, 200, 200), cv::Scalar(color[0], color[1], color[2]), cv::FILLED);
    cv::rectangle(image, cv::Rect(50, 80, 200, 200), cv::Scalar(255, 255, 255), 3);
    cv::putText(image, "Current Color", cv::Point(85, 295), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
}

void ColorConverter::drawColorComponents(cv::Mat& image, const ColorModels& colors) {
    int baseY = 450;
    
    // RGB components
    cv::putText(image, "RGB Model:", cv::Point(50, baseY), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
    cv::putText(image, cv::format("R: %d", colors.rgb[2]), cv::Point(70, baseY + 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
    cv::putText(image, cv::format("G: %d", colors.rgb[1]), cv::Point(70, baseY + 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    cv::putText(image, cv::format("B: %d", colors.rgb[0]), cv::Point(70, baseY + 75), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
    
    // HSV components
    cv::putText(image, "HSV Model:", cv::Point(250, baseY), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
    cv::putText(image, cv::format("H: %.1f", colors.hsv[0]), cv::Point(270, baseY + 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    cv::putText(image, cv::format("S: %.1f%%", colors.hsv[1]), cv::Point(270, baseY + 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    cv::putText(image, cv::format("V: %.1f%%", colors.hsv[2]), cv::Point(270, baseY + 75), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    
    // CMYK components
    cv::putText(image, "CMYK Model:", cv::Point(450, baseY), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
    cv::putText(image, cv::format("C: %.1f%%", colors.cmyk[0]), cv::Point(470, baseY + 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
    cv::putText(image, cv::format("M: %.1f%%", colors.cmyk[1]), cv::Point(470, baseY + 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255), 1);
    cv::putText(image, cv::format("Y: %.1f%%", colors.cmyk[2]), cv::Point(470, baseY + 75), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
    cv::putText(image, cv::format("K: %.1f%%", colors.cmyk[3]), cv::Point(470, baseY + 100), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
}

std::vector<cv::Vec3b> ColorConverter::getPresetColors() {
    return {
        cv::Vec3b(0, 0, 255),       // Red
        cv::Vec3b(0, 165, 255),     // Orange
        cv::Vec3b(0, 255, 255),     // Yellow
        cv::Vec3b(0, 255, 0),       // Green
        cv::Vec3b(255, 255, 0),     // Cyan
        cv::Vec3b(255, 0, 0),       // Blue
        cv::Vec3b(255, 0, 255),     // Magenta
        cv::Vec3b(128, 0, 128),     // Purple
        cv::Vec3b(255, 255, 255),   // White
        cv::Vec3b(192, 192, 192),   // Light Gray
        cv::Vec3b(128, 128, 128),   // Gray
        cv::Vec3b(0, 0, 0)          // Black
    };
}

void ColorConverter::drawPresetPalette(cv::Mat& image) {
    cv::putText(image, "Preset Colors (Click to select):", cv::Point(300, 70), 
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    
    std::vector<cv::Vec3b> colors = getPresetColors();
    int colorSize = 35;
    int spacing = 3;
    int startX = 300;
    int startY = 80;
    int colorsPerRow = 12;
    
    for (size_t i = 0; i < colors.size(); i++) {
        int col = i % colorsPerRow;
        int row = i / colorsPerRow;
        int x = startX + col * (colorSize + spacing);
        int y = startY + row * (colorSize + spacing);
        
        cv::rectangle(image, cv::Rect(x, y, colorSize, colorSize), 
                     cv::Scalar(colors[i][0], colors[i][1], colors[i][2]), cv::FILLED);
        cv::rectangle(image, cv::Rect(x, y, colorSize, colorSize), 
                     cv::Scalar(200, 200, 200), 1);
    }
}

void ColorConverter::drawHsvGradient(cv::Mat& image) {
    cv::putText(image, "HSV Color Picker (Click to select):", cv::Point(300, 145), 
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    
    int startX = 300;
    int startY = 155;
    int width = 450;
    int height = 120;
    
    // Draw hue gradient
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            float hue = (x * 360.0f) / width;
            float saturation = 100.0f;
            float value = 100.0f - (y * 100.0f) / height;
            
            cv::Vec3f hsv(hue, saturation, value);
            cv::Vec3b rgb = hsvToRgb(hsv);
            
            image.at<cv::Vec3b>(startY + y, startX + x) = rgb;
        }
    }
    
    // Draw border
    cv::rectangle(image, cv::Rect(startX, startY, width, height), 
                 cv::Scalar(200, 200, 200), 2);
    
    // Add labels
    cv::putText(image, "Hue ->", cv::Point(startX + width/2 - 30, startY + height + 20), 
                cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(200, 200, 200), 1);
}

cv::Vec3b ColorConverter::getColorFromPresetPalette(int x, int y) {
    std::vector<cv::Vec3b> colors = getPresetColors();
    int colorSize = 35;
    int spacing = 3;
    int startX = 300;
    int startY = 80;
    int colorsPerRow = 12;
    
    for (size_t i = 0; i < colors.size(); i++) {
        int col = i % colorsPerRow;
        int row = i / colorsPerRow;
        int rectX = startX + col * (colorSize + spacing);
        int rectY = startY + row * (colorSize + spacing);
        
        if (x >= rectX && x <= rectX + colorSize && 
            y >= rectY && y <= rectY + colorSize) {
            return colors[i];
        }
    }
    
    return cv::Vec3b(0, 0, 0); // Return black if not found
}

cv::Vec3b ColorConverter::getColorFromHsvGradient(int x, int y) {
    int startX = 300;
    int startY = 155;
    int width = 450;
    int height = 120;
    
    if (x >= startX && x < startX + width && y >= startY && y < startY + height) {
        int relX = x - startX;
        int relY = y - startY;
        
        float hue = (relX * 360.0f) / width;
        float saturation = 100.0f;
        float value = 100.0f - (relY * 100.0f) / height;
        
        cv::Vec3f hsv(hue, saturation, value);
        return hsvToRgb(hsv);
    }
    
    return cv::Vec3b(0, 0, 0); // Return black if not found
}
