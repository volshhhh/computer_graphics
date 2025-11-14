#include "main_window.h"
#include <iostream>
#include <cmath>

ImageProcessor::ImageProcessor() : width(0), height(0) {}

bool ImageProcessor::loadImage(const std::string& filename) {
    try {
        pixbuf = Gdk::Pixbuf::create_from_file(filename);
        if (!pixbuf) return false;

        width = pixbuf->get_width();
        height = pixbuf->get_height();

        originalPixbuf = pixbuf;
        filteredPixbuf = pixbuf->copy();

        return true;
    }
    catch (const Glib::Exception& ex) {
        std::cerr << "Error loading image: " << ex.what() << std::endl;
        return false;
    }
}

void ImageProcessor::applyLowPassFilter(int kernelSize) {
    if (!filteredPixbuf) return;

    auto tempPixbuf = filteredPixbuf->copy();

    if (kernelSize % 2 == 0 || kernelSize < 3) {
        kernelSize = 3;
    }

    int radius = kernelSize / 2;
    guint8* src_pixels = tempPixbuf->get_pixels();
    guint8* dst_pixels = filteredPixbuf->get_pixels();
    int rowstride = filteredPixbuf->get_rowstride();
    int n_channels = filteredPixbuf->get_n_channels();

    for (int y = radius; y < height - radius; ++y) {
        for (int x = radius; x < width - radius; ++x) {
            for (int channel = 0; channel < 3; channel++) {
                int sum = 0;
                int count = 0;

                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        guint8* p = src_pixels + (y + ky) * rowstride + (x + kx) * n_channels;
                        sum += p[channel];
                        count++;
                    }
                }

                guint8* dst_p = dst_pixels + y * rowstride + x * n_channels;
                dst_p[channel] = static_cast<guint8>(sum / count);
            }
        }
    }
}

void ImageProcessor::applyGaussianFilter(int kernelSize, double sigma) {
    if (!filteredPixbuf) return;

    auto tempPixbuf = filteredPixbuf->copy();

    if (kernelSize % 2 == 0 || kernelSize < 3) {
        kernelSize = 3;
    }

    int radius = kernelSize / 2;
    guint8* src_pixels = tempPixbuf->get_pixels();
    guint8* dst_pixels = filteredPixbuf->get_pixels();
    int rowstride = filteredPixbuf->get_rowstride();
    int n_channels = filteredPixbuf->get_n_channels();

    std::vector<std::vector<double>> kernel(kernelSize, std::vector<double>(kernelSize));
    double sum_kernel = 0.0;

    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            double value = exp(-(i*i + j*j) / (2 * sigma * sigma));
            kernel[i + radius][j + radius] = value;
            sum_kernel += value;
        }
    }

    for (int i = 0; i < kernelSize; i++) {
        for (int j = 0; j < kernelSize; j++) {
            kernel[i][j] /= sum_kernel;
        }
    }

    for (int y = radius; y < height - radius; ++y) {
        for (int x = radius; x < width - radius; ++x) {
            for (int channel = 0; channel < 3; channel++) {
                double sum = 0.0;

                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        guint8* p = src_pixels + (y + ky) * rowstride + (x + kx) * n_channels;
                        double weight = kernel[ky + radius][kx + radius];
                        sum += p[channel] * weight;
                    }
                }

                guint8* dst_p = dst_pixels + y * rowstride + x * n_channels;
                dst_p[channel] = static_cast<guint8>(std::max(0.0, std::min(255.0, sum)));
            }
        }
    }
}

std::vector<std::vector<int>> ImageProcessor::getHistogram() {
    std::vector<std::vector<int>> histogram(3, std::vector<int>(256, 0));

    if (!originalPixbuf) return histogram;

    guint8* pixels = originalPixbuf->get_pixels();
    int rowstride = originalPixbuf->get_rowstride();
    int n_channels = originalPixbuf->get_n_channels();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            guint8* p = pixels + y * rowstride + x * n_channels;
            histogram[0][p[0]]++;
            histogram[1][p[1]]++;
            histogram[2][p[2]]++;
        }
    }

    return histogram;
}

void ImageProcessor::applyHistogramEqualization() {
    if (!originalPixbuf) return;

    filteredPixbuf = originalPixbuf->copy();

    auto histogram = getHistogram();

    std::vector<std::vector<int>> cdf(3, std::vector<int>(256, 0));
    int total_pixels = width * height;

    for (int channel = 0; channel < 3; channel++) {
        cdf[channel][0] = histogram[channel][0];
        for (int i = 1; i < 256; i++) {
            cdf[channel][i] = cdf[channel][i-1] + histogram[channel][i];
        }
    }

    std::vector<int> cdf_min(3, total_pixels);
    for (int channel = 0; channel < 3; channel++) {
        for (int i = 0; i < 256; i++) {
            if (histogram[channel][i] != 0) {
                cdf_min[channel] = std::min(cdf_min[channel], cdf[channel][i]);
            }
        }
    }

    guint8* pixels = filteredPixbuf->get_pixels();
    int rowstride = filteredPixbuf->get_rowstride();
    int n_channels = filteredPixbuf->get_n_channels();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            guint8* p = pixels + y * rowstride + x * n_channels;
            for (int channel = 0; channel < 3; channel++) {
                int old_intensity = p[channel];
                if (cdf[channel][old_intensity] > cdf_min[channel]) {
                    float equalized = (cdf[channel][old_intensity] - cdf_min[channel]) /
                                      static_cast<float>(total_pixels - cdf_min[channel]);
                    p[channel] = static_cast<guint8>(equalized * 255);
                } else {
                    p[channel] = 0;
                }
            }
        }
    }
}

void ImageProcessor::applyLinearContrast(int min_out, int max_out) {
    if (!originalPixbuf) return;

    filteredPixbuf = originalPixbuf->copy();

    int min_brightness = 255;
    int max_brightness = 0;

    guint8* orig_pixels = originalPixbuf->get_pixels();
    int rowstride = originalPixbuf->get_rowstride();
    int n_channels = originalPixbuf->get_n_channels();

    // Находим минимальную и максимальную яркость по всему изображению
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            guint8* p = orig_pixels + y * rowstride + x * n_channels;
            
            // Вычисляем яркость по формуле Y = 0.299R + 0.587G + 0.114B
            float brightness = 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2];
            int bright_int = static_cast<int>(brightness);
            
            min_brightness = std::min(min_brightness, bright_int);
            max_brightness = std::max(max_brightness, bright_int);
        }
    }

    // Если все пиксели одинаковой яркости, избегаем деления на ноль
    if (max_brightness == min_brightness) {
        return;
    }

    guint8* pixels = filteredPixbuf->get_pixels();

    // Применяем контрастирование ко всем каналам с одинаковым коэффициентом
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            guint8* p = pixels + y * rowstride + x * n_channels;
            
            // Вычисляем яркость текущего пикселя
            float brightness = 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2];
            int bright_int = static_cast<int>(brightness);
            
            // Нормализуем яркость
            float normalized = static_cast<float>(bright_int - min_brightness) / 
                              (max_brightness - min_brightness);
            
            // Вычисляем новый уровень яркости
            int new_brightness = static_cast<int>(min_out + normalized * (max_out - min_out));
            new_brightness = std::max(0, std::min(255, new_brightness));
            
            // Вычисляем коэффициент масштабирования для каждого канала
            float scale_factor = (bright_int == 0) ? 1.0f : static_cast<float>(new_brightness) / bright_int;
            
            // Применяем одинаковое преобразование ко всем каналам
            for (int channel = 0; channel < 3; channel++) {
                int new_value = static_cast<int>(p[channel] * scale_factor);
                p[channel] = static_cast<guint8>(std::max(0, std::min(255, new_value)));
            }
        }
    }
}

std::vector<unsigned char> ImageProcessor::encodeRLE() {
    std::vector<unsigned char> encoded;
    if (!filteredPixbuf) return encoded;

    guint8* pixels = filteredPixbuf->get_pixels();
    int rowstride = filteredPixbuf->get_rowstride();
    int n_channels = filteredPixbuf->get_n_channels();

    encoded.push_back((width >> 8) & 0xFF);
    encoded.push_back(width & 0xFF);
    encoded.push_back((height >> 8) & 0xFF);
    encoded.push_back(height & 0xFF);

    for (int y = 0; y < height; ++y) {
        for (int channel = 0; channel < 3; channel++) {
            int count = 1;
            unsigned char current = pixels[y * rowstride + channel];

            for (int x = 1; x < width; ++x) {
                unsigned char next = pixels[y * rowstride + x * n_channels + channel];
                if (next == current && count < 255) {
                    count++;
                } else {
                    encoded.push_back(count);
                    encoded.push_back(current);
                    current = next;
                    count = 1;
                }
            }
            encoded.push_back(count);
            encoded.push_back(current);
        }
    }

    return encoded;
}

bool ImageProcessor::decodeRLE(const std::vector<unsigned char>& encoded) {
    if (encoded.size() < 4) return false;

    int decoded_width = (encoded[0] << 8) | encoded[1];
    int decoded_height = (encoded[2] << 8) | encoded[3];

    filteredPixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, decoded_width, decoded_height);
    width = decoded_width;
    height = decoded_height;

    guint8* pixels = filteredPixbuf->get_pixels();
    int rowstride = filteredPixbuf->get_rowstride();
    int n_channels = filteredPixbuf->get_n_channels();

    size_t pos = 4;

    for (int y = 0; y < height && pos < encoded.size(); ++y) {
        for (int channel = 0; channel < 3 && pos < encoded.size(); channel++) {
            int x = 0;
            while (x < width && pos + 1 < encoded.size()) {
                unsigned char count = encoded[pos++];
                unsigned char value = encoded[pos++];

                for (int i = 0; i < count && x < width; ++i) {
                    pixels[y * rowstride + x * n_channels + channel] = value;
                    x++;
                }
            }
        }
    }

    return true;
}

bool ImageProcessor::saveRLEToFile(const std::string& filename) {
    auto encoded = encodeRLE();
    if (encoded.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    file.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    return file.good();
}

bool ImageProcessor::loadRLEFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> encoded(size);
    file.read(reinterpret_cast<char*>(encoded.data()), size);

    return decodeRLE(encoded);
}

void ImageProcessor::setOriginalFromFiltered() {
    if (filteredPixbuf) {
        originalPixbuf = filteredPixbuf->copy();
    }
}

Glib::RefPtr<Gdk::Pixbuf> ImageProcessor::getOriginalPixbuf() { return originalPixbuf; }
Glib::RefPtr<Gdk::Pixbuf> ImageProcessor::getFilteredPixbuf() { return filteredPixbuf; }

void ImageProcessor::resetToOriginal() {
    if (originalPixbuf) {
        filteredPixbuf = originalPixbuf->copy();
    }
}

bool ImageProcessor::hasImage() const { return (bool)originalPixbuf; }

HistogramDrawingArea::HistogramDrawingArea(const std::vector<int>& histogram, const Gdk::RGBA& color)
        : histogram(histogram), color(color) {
    set_size_request(550, 300);
}

bool HistogramDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    int max_count = *std::max_element(histogram.begin(), histogram.end());
    if (max_count == 0) max_count = 1;

    const int margin = 50;
    const int graph_width = width - 2 * margin;
    const int graph_height = height - 2 * margin;

    cr->set_source_rgb(0, 0, 0);
    cr->set_line_width(2);
    cr->move_to(margin, margin);
    cr->line_to(margin, height - margin);
    cr->move_to(margin, height - margin);
    cr->line_to(width - margin, height - margin);
    cr->stroke();

    cr->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), 0.7);

    double bar_width = static_cast<double>(graph_width) / 256;
    for (int i = 0; i < 256; i++) {
        if (histogram[i] > 0) {
            double bar_height = (histogram[i] / static_cast<double>(max_count)) * graph_height;
            cr->rectangle(margin + i * bar_width,
                          height - margin - bar_height,
                          bar_width - 1,
                          bar_height);
            cr->fill();
        }
    }

    cr->set_source_rgb(0, 0, 0);
    cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
    cr->set_font_size(12);

    cr->save();
    cr->move_to(10, height / 2);
    cr->rotate(-M_PI / 2);
    cr->show_text("Frequency");
    cr->restore();

    cr->move_to(width / 2 - 30, height - 10);
    cr->show_text("Pixel Intensity");

    cr->set_font_size(14);
    cr->move_to(width / 2 - 40, 20);
    if (color.get_red() > 0.5) cr->show_text("Red Channel");
    else if (color.get_green() > 0.5) cr->show_text("Green Channel");
    else cr->show_text("Blue Channel");

    return true;
}

HistogramDialog::HistogramDialog(Gtk::Window& parent, const std::vector<std::vector<int>>& histogram)
        : Gtk::Dialog("Image Histogram", parent, true) {

    set_default_size(600, 400);
    set_border_width(10);

    Gtk::Box* contentBox = get_content_area();

    notebook.append_page(redBox, "Red Channel");
    notebook.append_page(greenBox, "Green Channel");
    notebook.append_page(blueBox, "Blue Channel");

    redDrawingArea = Gtk::manage(new HistogramDrawingArea(histogram[0], Gdk::RGBA("red")));
    greenDrawingArea = Gtk::manage(new HistogramDrawingArea(histogram[1], Gdk::RGBA("green")));
    blueDrawingArea = Gtk::manage(new HistogramDrawingArea(histogram[2], Gdk::RGBA("blue")));

    redBox.pack_start(*redDrawingArea, true, true, 0);
    greenBox.pack_start(*greenDrawingArea, true, true, 0);
    blueBox.pack_start(*blueDrawingArea, true, true, 0);

    contentBox->pack_start(notebook, true, true, 0);

    add_button("_Close", Gtk::RESPONSE_CLOSE);

    show_all_children();

    redDrawingArea->queue_draw();
    greenDrawingArea->queue_draw();
    blueDrawingArea->queue_draw();
}

ContrastDialog::ContrastDialog(Gtk::Window& parent)
        : Gtk::Dialog("Linear Contrast Settings", parent, true) {
    
    set_default_size(300, 150);
    set_border_width(10);
    
    Gtk::Box* contentBox = get_content_area();
    
    minScale.set_range(0, 254);
    minScale.set_value(0);
    minScale.set_increments(1, 10);
    minScale.set_digits(0);
    
    maxScale.set_range(1, 255);
    maxScale.set_value(255);
    maxScale.set_increments(1, 10);
    maxScale.set_digits(0);
    
    minBox.pack_start(minLabel, false, false, 5);
    minBox.pack_start(minScale, true, true, 5);
    
    maxBox.pack_start(maxLabel, false, false, 5);
    maxBox.pack_start(maxScale, true, true, 5);
    
    mainBox.pack_start(minBox, true, true, 5);
    mainBox.pack_start(maxBox, true, true, 5);
    
    contentBox->pack_start(mainBox, true, true, 0);
    
    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("_Apply", Gtk::RESPONSE_OK);
    
    show_all_children();
}

int ContrastDialog::getMinValue() const {
    return static_cast<int>(minScale.get_value());
}

int ContrastDialog::getMaxValue() const {
    return static_cast<int>(maxScale.get_value());
}

FilterDialog::FilterDialog(Gtk::Window& parent)
        : Gtk::Dialog("Low-Pass Filter Settings", parent, true) {
    
    set_default_size(300, 200);
    set_border_width(10);
    
    Gtk::Box* contentBox = get_content_area();
    
    filterTypeLabel.set_label("Filter Type:");
    filterTypeCombo.append("Average Filter");
    filterTypeCombo.append("Gaussian Filter");
    filterTypeCombo.set_active(0);
    
    kernelSizeLabel.set_label("Kernel Size:");
    kernelSizeCombo.append("3x3");
    kernelSizeCombo.append("5x5");
    kernelSizeCombo.append("7x7");
    kernelSizeCombo.append("9x9");
    kernelSizeCombo.append("11x11");
    kernelSizeCombo.set_active(0);
    
    sigmaLabel.set_label("Sigma (for Gaussian):");
    sigmaScale.set_range(0.5, 5.0);
    sigmaScale.set_value(1.0);
    sigmaScale.set_increments(0.1, 0.5);
    sigmaScale.set_digits(1);
    
    filterTypeBox.pack_start(filterTypeLabel, false, false, 5);
    filterTypeBox.pack_start(filterTypeCombo, true, true, 5);
    
    kernelSizeBox.pack_start(kernelSizeLabel, false, false, 5);
    kernelSizeBox.pack_start(kernelSizeCombo, true, true, 5);
    
    sigmaBox.pack_start(sigmaLabel, false, false, 5);
    sigmaBox.pack_start(sigmaScale, true, true, 5);
    
    mainBox.pack_start(filterTypeBox, true, true, 5);
    mainBox.pack_start(kernelSizeBox, true, true, 5);
    mainBox.pack_start(sigmaBox, true, true, 5);
    
    contentBox->pack_start(mainBox, true, true, 0);
    
    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("_Apply", Gtk::RESPONSE_OK);
    
    show_all_children();
}

int FilterDialog::getKernelSize() const {
    int index = kernelSizeCombo.get_active_row_number();
    switch (index) {
        case 0: return 3;
        case 1: return 5;
        case 2: return 7;
        case 3: return 9;
        case 4: return 11;
        default: return 3;
    }
}

int FilterDialog::getFilterType() const {
    return filterTypeCombo.get_active_row_number();
}

double FilterDialog::getSigma() const {
    return sigmaScale.get_value();
}

MainWindow::MainWindow() {
    set_title("Image Processing Application");
    set_default_size(1200, 800);
    set_border_width(10);
    
    setupUI();
}

void MainWindow::setupUI() {
    add(mainBox);
    setupLayout();
    show_all_children();
}

void MainWindow::setupLayout() {
    mainBox.pack_start(contentBox, true, true, 0);
    contentBox.set_spacing(10);

    imageStackBox.set_spacing(10);
    contentBox.pack_start(imageStackBox, true, true, 0);

    originalFrame.set_label("Original Image");
    originalScrolled.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    originalScrolled.add(originalImage);
    originalFrame.add(originalScrolled);
    imageStackBox.pack_start(originalFrame, true, true, 0);

    filteredFrame.set_label("Processed Image");
    filteredScrolled.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    filteredScrolled.add(filteredImage);
    filteredFrame.add(filteredScrolled);
    imageStackBox.pack_start(filteredFrame, true, true, 0);

    controlsBox.set_border_width(10);
    controlsBox.set_spacing(8);
    controlsBox.set_size_request(250, -1);
    contentBox.pack_start(controlsBox, false, false, 0);

    auto fileLabel = Gtk::manage(new Gtk::Label("<b>File</b>"));
    fileLabel->set_use_markup(true);
    fileLabel->set_xalign(0.0);
    controlsBox.pack_start(*fileLabel, Gtk::PACK_SHRINK, 5);

    auto openIcon = Gtk::manage(new Gtk::Image("document-open-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto saveIcon = Gtk::manage(new Gtk::Image("document-save-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto lowpassIcon = Gtk::manage(new Gtk::Image("view-grid-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto equalizeIcon = Gtk::manage(new Gtk::Image("color-balance-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto contrastIcon = Gtk::manage(new Gtk::Image("display-brightness-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto histogramIcon = Gtk::manage(new Gtk::Image("view-histogram-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto encodeIcon = Gtk::manage(new Gtk::Image("archive-insert-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto decodeIcon = Gtk::manage(new Gtk::Image("archive-extract-symbolic", Gtk::ICON_SIZE_BUTTON));
    auto resetIcon = Gtk::manage(new Gtk::Image("edit-undo-symbolic", Gtk::ICON_SIZE_BUTTON));

    openButton.set_label("Open Image");
    openButton.set_image(*openIcon);
    openButton.set_always_show_image(true);
    openButton.signal_clicked().connect([this]() { on_open_clicked(); });
    controlsBox.pack_start(openButton, Gtk::PACK_SHRINK);

    saveButton.set_label("Save Result");
    saveButton.set_image(*saveIcon);
    saveButton.set_always_show_image(true);
    saveButton.signal_clicked().connect([this]() { on_save_clicked(); });
    controlsBox.pack_start(saveButton, Gtk::PACK_SHRINK);

    controlsBox.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), Gtk::PACK_SHRINK, 10);

    auto filterLabel = Gtk::manage(new Gtk::Label("<b>Filters</b>"));
    filterLabel->set_use_markup(true);
    filterLabel->set_xalign(0.0);
    controlsBox.pack_start(*filterLabel, Gtk::PACK_SHRINK, 5);

    lowpassButton.set_label("Apply Low-Pass Filter");
    lowpassButton.set_image(*lowpassIcon);
    lowpassButton.set_always_show_image(true);
    lowpassButton.signal_clicked().connect([this]() { on_lowpass_clicked(); });
    controlsBox.pack_start(lowpassButton, Gtk::PACK_SHRINK);

    controlsBox.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), Gtk::PACK_SHRINK, 10);

    auto histLabel = Gtk::manage(new Gtk::Label("<b>Histogram</b>"));
    histLabel->set_use_markup(true);
    histLabel->set_xalign(0.0);
    controlsBox.pack_start(*histLabel, Gtk::PACK_SHRINK, 5);

    equalizeButton.set_label("Equalize Histogram");
    equalizeButton.set_image(*equalizeIcon);
    equalizeButton.set_always_show_image(true);
    equalizeButton.signal_clicked().connect([this]() { on_equalize_clicked(); });
    controlsBox.pack_start(equalizeButton, Gtk::PACK_SHRINK);

    contrastButton.set_label("Apply Linear Contrast");
    contrastButton.set_image(*contrastIcon);
    contrastButton.set_always_show_image(true);
    contrastButton.signal_clicked().connect([this]() { on_contrast_clicked(); });
    controlsBox.pack_start(contrastButton, Gtk::PACK_SHRINK);

    showHistogramButton.set_label("Show Histogram");
    showHistogramButton.set_image(*histogramIcon);
    showHistogramButton.set_always_show_image(true);
    showHistogramButton.signal_clicked().connect([this]() { on_show_histogram_clicked(); });
    controlsBox.pack_start(showHistogramButton, Gtk::PACK_SHRINK);

    controlsBox.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), Gtk::PACK_SHRINK, 10);

    auto rleLabel = Gtk::manage(new Gtk::Label("<b>Compression (RLE)</b>"));
    rleLabel->set_use_markup(true);
    rleLabel->set_xalign(0.0);
    controlsBox.pack_start(*rleLabel, Gtk::PACK_SHRINK, 5);

    encodeAndSaveRLEButton.set_label("Encode and Save RLE");
    encodeAndSaveRLEButton.set_image(*encodeIcon);
    encodeAndSaveRLEButton.set_always_show_image(true);
    encodeAndSaveRLEButton.signal_clicked().connect([this]() { on_encode_and_save_rle_clicked(); });
    controlsBox.pack_start(encodeAndSaveRLEButton, Gtk::PACK_SHRINK);

    decodeAndOpenRLEButton.set_label("Decode and Open RLE");
    decodeAndOpenRLEButton.set_image(*decodeIcon);
    decodeAndOpenRLEButton.set_always_show_image(true);
    decodeAndOpenRLEButton.signal_clicked().connect([this]() { on_decode_and_open_rle_clicked(); });
    controlsBox.pack_start(decodeAndOpenRLEButton, Gtk::PACK_SHRINK);

    controlsBox.pack_end(resetButton, Gtk::PACK_SHRINK, 10);
    resetButton.set_label("Reset to Original");
    resetButton.set_image(*resetIcon);
    resetButton.set_always_show_image(true);
    resetButton.signal_clicked().connect([this]() { on_reset_clicked(); });
}

void MainWindow::on_open_clicked() {
    Gtk::FileChooserDialog dialog("Choose an image", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    auto filter_image = Gtk::FileFilter::create();
    filter_image->set_name("Image files");
    filter_image->add_mime_type("image/jpeg");
    filter_image->add_mime_type("image/png");
    filter_image->add_mime_type("image/bmp");
    filter_image->add_pattern("*.jpg");
    filter_image->add_pattern("*.jpeg");
    filter_image->add_pattern("*.png");
    filter_image->add_pattern("*.bmp");
    dialog.add_filter(filter_image);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        if (processor.loadImage(filename)) {
            updateImages();
        } else {
            Gtk::MessageDialog error(*this, "Failed to load image", false, Gtk::MESSAGE_ERROR);
            error.run();
        }
    }
}

void MainWindow::on_save_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image to save", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }

    Gtk::FileChooserDialog dialog("Save image", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);

    auto filter_png = Gtk::FileFilter::create();
    filter_png->set_name("PNG files");
    filter_png->add_mime_type("image/png");
    filter_png->add_pattern("*.png");
    dialog.add_filter(filter_png);

    auto filter_jpeg = Gtk::FileFilter::create();
    filter_jpeg->set_name("JPEG files");
    filter_jpeg->add_mime_type("image/jpeg");
    filter_jpeg->add_pattern("*.jpg");
    filter_jpeg->add_pattern("*.jpeg");
    dialog.add_filter(filter_jpeg);

    dialog.set_current_name("processed_image.png");

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        if (!filename.empty()) {
            try {
                std::string extension = filename.substr(filename.find_last_of(".") + 1);
                std::string file_type = "png";
                
                if (extension == "jpg" || extension == "jpeg") {
                    file_type = "jpeg";
                } else if (extension == "png") {
                    file_type = "png";
                } else if (extension == "bmp") {
                    file_type = "bmp";
                }
                
                processor.getFilteredPixbuf()->save(filename, file_type);
                
                Gtk::MessageDialog success(*this, "Image saved successfully", false, Gtk::MESSAGE_INFO);
                success.run();
            } catch (const Glib::Exception& ex) {
                Gtk::MessageDialog error(*this, "Failed to save image", false, Gtk::MESSAGE_ERROR);
                error.run();
            }
        }
    }
}

void MainWindow::on_lowpass_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }

    FilterDialog dialog(*this);
    if (dialog.run() == Gtk::RESPONSE_OK) {
        int kernelSize = dialog.getKernelSize();
        int filterType = dialog.getFilterType();
        double sigma = dialog.getSigma();

        if (filterType == 0) {
            processor.applyLowPassFilter(kernelSize);
            Gtk::MessageDialog info(*this, 
                "Applied average filter with kernel size " + std::to_string(kernelSize) + "x" + std::to_string(kernelSize), 
                false, Gtk::MESSAGE_INFO);
            info.run();
        } else {
            processor.applyGaussianFilter(kernelSize, sigma);
            Gtk::MessageDialog info(*this, 
                "Applied Gaussian filter with kernel size " + std::to_string(kernelSize) + "x" + std::to_string(kernelSize) + 
                " and sigma=" + std::to_string(sigma), 
                false, Gtk::MESSAGE_INFO);
            info.run();
        }
        
        updateImages();
    }
}

void MainWindow::on_equalize_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }
    processor.applyHistogramEqualization();
    updateImages();
}

void MainWindow::on_contrast_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }

    ContrastDialog dialog(*this);
    if (dialog.run() == Gtk::RESPONSE_OK) {
        int min_out = dialog.getMinValue();
        int max_out = dialog.getMaxValue();

        if (min_out >= max_out) {
            Gtk::MessageDialog error(*this, "Min value must be less than max value", false, Gtk::MESSAGE_ERROR);
            error.run();
            return;
        }

        processor.applyLinearContrast(min_out, max_out);
        updateImages();
    }
}

void MainWindow::on_show_histogram_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }

    auto histogram = processor.getHistogram();
    HistogramDialog dialog(*this, histogram);
    dialog.run();
}

void MainWindow::on_encode_and_save_rle_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }

    Gtk::FileChooserDialog dialog("Save RLE", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);

    auto filter_rle = Gtk::FileFilter::create();
    filter_rle->set_name("RLE files");
    filter_rle->add_pattern("*.rle");
    dialog.add_filter(filter_rle);

    dialog.set_current_name("image.rle");

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        if (!filename.empty()) {
            if (processor.saveRLEToFile(filename)) {
                Gtk::MessageDialog success(*this, "RLE saved successfully", false, Gtk::MESSAGE_INFO);
                success.run();
            } else {
                Gtk::MessageDialog error(*this, "Failed to save RLE", false, Gtk::MESSAGE_ERROR);
                error.run();
            }
        }
    }
}

void MainWindow::on_decode_and_open_rle_clicked() {
    Gtk::FileChooserDialog dialog("Load RLE", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    auto filter_rle = Gtk::FileFilter::create();
    filter_rle->set_name("RLE files");
    filter_rle->add_pattern("*.rle");
    dialog.add_filter(filter_rle);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        if (processor.loadRLEFromFile(filename)) {
            processor.setOriginalFromFiltered();
            updateImages();
            Gtk::MessageDialog success(*this, "RLE loaded as original image", false, Gtk::MESSAGE_INFO);
            success.run();
        } else {
            Gtk::MessageDialog error(*this, "Failed to load RLE file", false, Gtk::MESSAGE_ERROR);
            error.run();
        }
    }
}

void MainWindow::on_reset_clicked() {
    if (!processor.hasImage()) {
        Gtk::MessageDialog error(*this, "No image loaded", false, Gtk::MESSAGE_WARNING);
        error.run();
        return;
    }
    processor.resetToOriginal();
    updateImages();
}

void MainWindow::updateImages() {
    if (processor.hasImage()) {
        auto original = processor.getOriginalPixbuf();
        auto filtered = processor.getFilteredPixbuf();

        if (original) {
            originalImage.set(original);
        }

        if (filtered) {
            filteredImage.set(filtered);
        }
    }
}