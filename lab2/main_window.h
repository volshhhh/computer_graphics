#pragma once

#include <gtkmm-3.0/gtkmm.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

class ImageProcessor {
public:
    ImageProcessor();
    bool loadImage(const std::string& filename);
    void applyLowPassFilter();
    std::vector<std::vector<int>> getHistogram();
    void applyHistogramEqualization();
    void applyLinearContrast(int min_out = 0, int max_out = 255);
    std::vector<unsigned char> encodeRLE();
    bool decodeRLE(const std::vector<unsigned char>& encoded);
    bool saveRLEToFile(const std::string& filename);
    bool loadRLEFromFile(const std::string& filename);
    void setOriginalFromFiltered();
    Glib::RefPtr<Gdk::Pixbuf> getOriginalPixbuf();
    Glib::RefPtr<Gdk::Pixbuf> getFilteredPixbuf();
    void resetToOriginal();
    bool hasImage() const;
    void applyLowPassFilter(int kernelSize = 3);
void applyGaussianFilter(int kernelSize = 3, double sigma = 1.0);


private:
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
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
    Gtk::Box redBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box greenBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box blueBox{Gtk::ORIENTATION_VERTICAL};
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
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box minBox{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Box maxBox{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Label minLabel{"Min Value (0-254):"};
    Gtk::Label maxLabel{"Max Value (1-255):"};
    Gtk::Scale minScale;
    Gtk::Scale maxScale;
};


class FilterDialog : public Gtk::Dialog {
public:
    FilterDialog(Gtk::Window& parent);
    int getKernelSize() const;
    int getFilterType() const;
    double getSigma() const;

private:
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box filterTypeBox{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Box kernelSizeBox{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Box sigmaBox{Gtk::ORIENTATION_HORIZONTAL};
    
    Gtk::Label filterTypeLabel;
    Gtk::ComboBoxText filterTypeCombo;
    
    Gtk::Label kernelSizeLabel;
    Gtk::ComboBoxText kernelSizeCombo;
    
    Gtk::Label sigmaLabel;
    Gtk::Scale sigmaScale;
};



class MainWindow : public Gtk::Window {
public:
    MainWindow();

private:
    void setupUI();
    void setupMenu();
    void setupLayout();
    
    // Обработчики событий
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

    // Основные виджеты макета
    Gtk::Box mainBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box contentBox{Gtk::ORIENTATION_HORIZONTAL}; // (Images | Controls)
    Gtk::Box imageStackBox{Gtk::ORIENTATION_VERTICAL}; // (Image1 / Image2)
    Gtk::Box controlsBox{Gtk::ORIENTATION_VERTICAL}; // (Sidebar)

    // Области отображения изображений
    Gtk::ScrolledWindow originalScrolled, filteredScrolled;
    Gtk::Frame originalFrame, filteredFrame;
    Gtk::Image originalImage, filteredImage;

    // Меню
    Gtk::MenuBar menuBar;
    Gtk::Menu fileMenu, filterMenu, histogramMenu, compressionMenu;
    Gtk::MenuItem fileMenuItem, filterMenuItem, histogramMenuItem, compressionMenuItem;
    Gtk::MenuItem openMenuItem, saveMenuItem, exitMenuItem;
    Gtk::MenuItem lowpassMenuItem, equalizeMenuItem, contrastMenuItem, showHistogramMenuItem;
    Gtk::MenuItem encodeAndSaveRLEMenuItem, decodeAndOpenRLEMenuItem;

    // Кнопки для боковой панели
    Gtk::Button openButton, saveButton;
    Gtk::Button lowpassButton, equalizeButton, contrastButton, showHistogramButton;
    Gtk::Button encodeAndSaveRLEButton, decodeAndOpenRLEButton;
    Gtk::Button resetButton;

    ImageProcessor processor;
};