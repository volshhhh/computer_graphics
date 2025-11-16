#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtkmm.h>
#include <vector>
#include <string>
#include <fstream>

class ImageProcessor {
public:
    ImageProcessor();
    bool loadImage(const std::string& filename);
    void applyLowPassFilter(int kernelSize);
    void applyGaussianFilter(int kernelSize, double sigma);
    std::vector<std::vector<int>> getHistogram();
    void applyHistogramEqualization(int type = 0); // 0 = RGB, 1 = HSV/HLS
    void applyLinearContrast(int min_out, int max_out);
    std::vector<unsigned char> encodeRLE();
    bool decodeRLE(const std::vector<unsigned char>& encoded);
    bool saveRLEToFile(const std::string& filename);
    bool loadRLEFromFile(const std::string& filename);
    void setOriginalFromFiltered();
    Glib::RefPtr<Gdk::Pixbuf> getOriginalPixbuf();
    Glib::RefPtr<Gdk::Pixbuf> getFilteredPixbuf();
    void resetToOriginal();
    bool hasImage() const;

private:
    void applyRGBEqualization();
    void applyBrightnessEqualization();
    
    Glib::RefPtr<Gdk::Pixbuf> originalPixbuf;
    Glib::RefPtr<Gdk::Pixbuf> filteredPixbuf;
    int width, height;
};

class HistogramDrawingArea : public Gtk::DrawingArea {
public:
    HistogramDrawingArea(const std::vector<int>& histogram, const Gdk::RGBA& color);
    
protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    
private:
    std::vector<int> histogram;
    Gdk::RGBA color;
};

class HistogramDialog : public Gtk::Dialog {
public:
    HistogramDialog(Gtk::Window& parent, const std::vector<std::vector<int>>& histogram);
    
private:
    Gtk::Notebook notebook;
    Gtk::Box redBox, greenBox, blueBox;
    HistogramDrawingArea* redDrawingArea;
    HistogramDrawingArea* greenDrawingArea;
    HistogramDrawingArea* blueDrawingArea;
};

class ContrastDialog : public Gtk::Dialog {
public:
    ContrastDialog(Gtk::Window& parent);
    int getMinValue() const;
    int getMaxValue() const;
    
private:
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Box minBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Box maxBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Label minLabel{"Minimum brightness:"};
    Gtk::Label maxLabel{"Maximum brightness:"};
    Gtk::Scale minScale, maxScale;
};

class FilterDialog : public Gtk::Dialog {
public:
    FilterDialog(Gtk::Window& parent);
    int getKernelSize() const;
    int getFilterType() const;
    double getSigma() const;
    
private:
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Box filterTypeBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Box kernelSizeBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Box sigmaBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Label filterTypeLabel, kernelSizeLabel, sigmaLabel;
    Gtk::ComboBoxText filterTypeCombo, kernelSizeCombo;
    Gtk::Scale sigmaScale;
};

class EqualizationDialog : public Gtk::Dialog {
public:
    EqualizationDialog(Gtk::Window& parent);
    int getEqualizationType() const;

private:
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Box typeBox{Gtk::ORIENTATION_HORIZONTAL, 5};
    Gtk::Label typeLabel{"Equalization Type:"};
    Gtk::ComboBoxText typeCombo;
};

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    
private:
    void setupUI();
    void setupLayout();
    void on_open_clicked();
    void on_save_clicked();
    void on_lowpass_clicked();
    void on_equalize_clicked();
    void on_contrast_clicked();
    void on_show_histogram_clicked();
    void on_encode_and_save_rle_clicked();
    void on_decode_and_open_rle_clicked();
    void on_reset_clicked();
    void updateImages();
    
    ImageProcessor processor;
    
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Box contentBox{Gtk::ORIENTATION_HORIZONTAL, 10};
    Gtk::Box imageStackBox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Frame originalFrame, filteredFrame;
    Gtk::ScrolledWindow originalScrolled, filteredScrolled;
    Gtk::Image originalImage, filteredImage;
    Gtk::Box controlsBox{Gtk::ORIENTATION_VERTICAL, 10};
    
    Gtk::Button openButton, saveButton, lowpassButton, equalizeButton;
    Gtk::Button contrastButton, showHistogramButton;
    Gtk::Button encodeAndSaveRLEButton, decodeAndOpenRLEButton, resetButton;
};

#endif // MAIN_WINDOW_H