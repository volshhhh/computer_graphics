#include <gtkmm.h>
#include <cairomm/context.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <chrono>
#include <functional>
#include <optional>
#include <iomanip> // для std::setprecision

// --- Вспомогательные структуры ---

/**
 * @brief Простая структура для целочисленной точки.
 * Нужен operator< для использования в std::map и std::set.
 */
struct Point {
    int x, y;
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

/**
 * @brief Структура для пикселя с интенсивностью (для AA).
 */
struct Pixel {
    int x, y;
    double intensity;
};

// --- Алгоритмы Растеризации ---

namespace RasterizationAlgorithms {
    using PixelList = std::vector<Point>;
    using WuPixelList = std::vector<Pixel>;

    PixelList step_by_step(int x0, int y0, int x1, int y1) {
        PixelList pixels;
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        if (dx == 0 && dy == 0) {
            return {{x0, y0}};
        }
        int steps = std::max(dx, dy);
        double x_step = static_cast<double>(x1 - x0) / steps;
        double y_step = static_cast<double>(y1 - y0) / steps;
        for (int i = 0; i <= steps; ++i) {
            int x = static_cast<int>(std::round(x0 + i * x_step));
            int y = static_cast<int>(std::round(y0 + i * y_step));
            pixels.push_back({x, y});
        }
        return pixels;
    }

    PixelList dda(int x0, int y0, int x1, int y1) {
        PixelList pixels;
        int dx = x1 - x0;
        int dy = y1 - y0;
        int steps = std::max(std::abs(dx), std::abs(dy));
        if (steps == 0) {
            return {{x0, y0}};
        }
        double x_inc = static_cast<double>(dx) / steps;
        double y_inc = static_cast<double>(dy) / steps;
        double x = x0;
        double y = y0;
        for (int i = 0; i <= steps; ++i) {
            pixels.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
            x += x_inc;
            y += y_inc;
        }
        return pixels;
    }

    PixelList bresenham_line(int x0, int y0, int x1, int y1) {
        PixelList pixels;
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x1 >= x0) ? 1 : -1;
        int sy = (y1 >= y0) ? 1 : -1;

        if (dx >= dy) {
            int err = 2 * dy - dx;
            int x = x0;
            int y = y0;
            for (int i = 0; i <= dx; ++i) {
                pixels.push_back({x, y});
                if (err >= 0) {
                    y += sy;
                    err += 2 * (dy - dx);
                } else {
                    err += 2 * dy;
                }
                x += sx;
            }
        } else {
            int err = 2 * dx - dy;
            int x = x0;
            int y = y0;
            for (int i = 0; i <= dy; ++i) {
                pixels.push_back({x, y});
                if (err >= 0) {
                    x += sx;
                    err += 2 * (dx - dy);
                } else {
                    err += 2 * dx;
                }
                y += sy;
            }
        }
        return pixels;
    }

    PixelList bresenham_circle(int xc, int yc, int r) {
        std::set<Point> pixel_set; // Используем set для автоматической уникальности
        int x = 0;
        int y = r;
        int d = 3 - 2 * r;
        while (x <= y) {
            pixel_set.insert({xc + x, yc + y});
            pixel_set.insert({xc - x, yc + y});
            pixel_set.insert({xc + x, yc - y});
            pixel_set.insert({xc - x, yc - y});
            pixel_set.insert({xc + y, yc + x});
            pixel_set.insert({xc - y, yc + x});
            pixel_set.insert({xc + y, yc - x});
            pixel_set.insert({xc - y, yc - x});
            if (d >= 0) {
                d = d + 4 * (x - y) + 10;
                y -= 1;
            } else {
                d = d + 4 * x + 6;
            }
            x += 1;
        }
        return PixelList(pixel_set.begin(), pixel_set.end());
    }

    // Вспомогательные функции для Wu
    namespace WuUtils {
        inline int ipart(double x) { return static_cast<int>(std::floor(x)); }
        inline int roundi(double x) { return static_cast<int>(std::floor(x + 0.5)); }
        inline double fpart(double x) { return x - std::floor(x); }
        inline double rfpart(double x) { return 1.0 - fpart(x); }
    }

    WuPixelList wu_line(double x0, double y0, double x1, double y1) {
        using namespace WuUtils;
        std::map<Point, double> pixel_map; // Для слияния пикселей

        bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        double dx = x1 - x0;
        double dy = y1 - y0;
        double gradient = (dx != 0.0) ? dy / dx : 0.0;

        auto add_pixel = [&](int x, int y, double intensity) {
            Point p = steep ? Point{y, x} : Point{x, y};
            pixel_map[p] = std::max(pixel_map[p], intensity);
        };

        // Ручка первого эндпоинта
        int xend = roundi(x0);
        double yend = y0 + gradient * (xend - x0);
        double xgap = rfpart(x0 + 0.5);
        int xpxl1 = xend;
        int ypxl1 = ipart(yend);
        add_pixel(xpxl1, ypxl1, rfpart(yend) * xgap);
        add_pixel(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
        double intery = yend + gradient;

        // Ручка второго эндпоинта
        xend = roundi(x1);
        yend = y1 + gradient * (xend - x1);
        xgap = fpart(x1 + 0.5);
        int xpxl2 = xend;
        int ypxl2 = ipart(yend);
        add_pixel(xpxl2, ypxl2, rfpart(yend) * xgap);
        add_pixel(xpxl2, ypxl2 + 1, fpart(yend) * xgap);

        // Основной цикл
        for (int x = xpxl1 + 1; x < xpxl2; ++x) {
            add_pixel(x, ipart(intery), rfpart(intery));
            add_pixel(x, ipart(intery) + 1, fpart(intery));
            intery += gradient;
        }

        // Конвертация map в vector
        WuPixelList pixels;
        for (const auto& pair : pixel_map) {
            pixels.push_back({pair.first.x, pair.first.y, pair.second});
        }
        return pixels;
    }

    WuPixelList castle_pitteway(int x0, int y0, int x1, int y1) {
        std::map<Point, double> merged;

        int dx = x1 - x0;
        int dy = y1 - y0;
        if (dx == 0 && dy == 0) {
            return {{x0, y0, 1.0}};
        }
        bool steep = std::abs(dy) > std::abs(dx);
        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
            std::swap(dx, dy);
        }
        int sx = (dx >= 0) ? 1 : -1;
        int sy = (dy >= 0) ? 1 : -1;
        dx = std::abs(dx);
        dy = std::abs(dy);

        auto add_pixel = [&](int x, int y, double intensity) {
            Point p = steep ? Point{y, x} : Point{x, y};
            merged[p] = std::max(merged[p], intensity);
        };

        if (dx == 0) {
            for (int i = 0; i <= dy; ++i) {
                add_pixel(x0, y0 + i * sy, 1.0);
            }
        } else {
            double gradient = static_cast<double>(dy) / dx;
            for (int i = 0; i <= dx; ++i) {
                int x = x0 + i * sx;
                double ideal_y = y0 + gradient * (x - x0) * sy; // Учитываем sy для градиента
                int y_near = static_cast<int>(std::round(ideal_y));
                double dist = std::abs(ideal_y - y_near);
                double intensity_near = std::max(0.0, 1.0 - dist);
                double intensity_far = std::max(0.0, dist);

                add_pixel(x, y_near, intensity_near);
                if (intensity_far > 1e-6) {
                    int neighbor = y_near + ((ideal_y - y_near) > 0 ? 1 : -1);
                    add_pixel(x, neighbor, intensity_far);
                }
            }
        }
        
        WuPixelList pixels;
        for (const auto& pair : merged) {
            pixels.push_back({pair.first.x, pair.first.y, pair.second});
        }
        return pixels;
    }
} // namespace RasterizationAlgorithms

// --- Тестер Производительности ---

namespace PerformanceTester {
    /**
     * @brief Измеряет время выполнения функции в микросекундах.
     */
    double measure_time(std::function<void()> func, int iterations = 200) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> us = (end - start);
        return us.count() / iterations;
    }

    struct BenchmarkResult {
        std::string name;
        double time_us;
        size_t pixel_count;
    };

    std::vector<BenchmarkResult> benchmark_all() {
        std::vector<BenchmarkResult> results;
        int x0=0, y0=0, x1=150, y1=100, r=80;

        auto test_step = [&](){ RasterizationAlgorithms::step_by_step(x0, y0, x1, y1); };
        results.push_back({"Step-by-step", measure_time(test_step, 300), RasterizationAlgorithms::step_by_step(x0, y0, x1, y1).size()});

        auto test_dda = [&](){ RasterizationAlgorithms::dda(x0, y0, x1, y1); };
        results.push_back({"DDA", measure_time(test_dda, 300), RasterizationAlgorithms::dda(x0, y0, x1, y1).size()});

        auto test_bres = [&](){ RasterizationAlgorithms::bresenham_line(x0, y0, x1, y1); };
        results.push_back({"Bresenham", measure_time(test_bres, 300), RasterizationAlgorithms::bresenham_line(x0, y0, x1, y1).size()});

        auto test_wu = [&](){ RasterizationAlgorithms::wu_line(x0, y0, x1, y1); };
        results.push_back({"Wu", measure_time(test_wu, 300), RasterizationAlgorithms::wu_line(x0, y0, x1, y1).size()});
        
        auto test_cp = [&](){ RasterizationAlgorithms::castle_pitteway(x0, y0, x1, y1); };
        results.push_back({"Castle-Pitteway", measure_time(test_cp, 300), RasterizationAlgorithms::castle_pitteway(x0, y0, x1, y1).size()});

        auto test_circle = [&](){ RasterizationAlgorithms::bresenham_circle(x0, y0, r); };
        results.push_back({"Bresenham circle", measure_time(test_circle, 300), RasterizationAlgorithms::bresenham_circle(x0, y0, r).size()});

        return results;
    }
} // namespace PerformanceTester

// --- Вспомогательные функции для цвета ---

namespace ColorHelpers {
    /**
     * @brief Конвертирует hex-строку (#RRGGBB) в Gdk::RGBA (0.0-1.0).
     */
    Gdk::RGBA hex_to_rgba(const std::string& hex) {
        Gdk::RGBA color;
        color.set_alpha(1.0);
        if (hex.length() == 7 && hex[0] == '#') {
            unsigned int r, g, b;
            sscanf(hex.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
            color.set_red(r / 255.0);
            color.set_green(g / 255.0);
            color.set_blue(b / 255.0);
        }
        return color;
    }

    /**
     * @brief Смешивает базовый цвет с белым. weight=1.0 - чистый цвет, weight=0.0 - белый.
     */
    Gdk::RGBA blend_with_white(const Gdk::RGBA& base_color, double weight) {
        Gdk::RGBA blended;
        blended.set_red(1.0 * (1.0 - weight) + base_color.get_red() * weight);
        blended.set_green(1.0 * (1.0 - weight) + base_color.get_green() * weight);
        blended.set_blue(1.0 * (1.0 - weight) + base_color.get_blue() * weight);
        blended.set_alpha(1.0);
        return blended;
    }
} // namespace ColorHelpers

// --- Основное приложение Gtkmm ---

class RasterizationApp : public Gtk::Window {
public:
    RasterizationApp();
    virtual ~RasterizationApp() {}

private:
    // Структура для хранения нарисованной фигуры
    struct Shape {
        std::vector<Pixel> pixels;
        Gdk::RGBA color;
        std::string algo_name;
    };

    // --- Обработчики событий ---
    bool on_drawing_area_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    bool on_drawing_area_button_press(GdkEventButton* event);
    void on_draw_from_entries();
    void on_apply_scale();
    void on_clear_all();
    void on_benchmark();

    // --- Вспомогательные методы ---
    void log_info(const std::string& msg);
    void draw_legend();
    void draw_line(Point p0, Point p1);
    void draw_grid_and_axes(const Cairo::RefPtr<Cairo::Context>& cr);
    void draw_all_shapes(const Cairo::RefPtr<Cairo::Context>& cr);
    void draw_pixel(const Cairo::RefPtr<Cairo::Context>& cr, const Pixel& p, const Gdk::RGBA& base_color);
    
    // --- Трансформации координат ---
    Point screen_to_grid(double sx, double sy);
    std::pair<double, double> grid_to_screen(int gx, int gy);

    // --- Состояние приложения ---
    int m_pixel_size = 6;
    int m_canvas_w = 900;
    int m_canvas_h = 650;
    std::map<std::string, Gdk::RGBA> m_colors;
    std::vector<Shape> m_shapes; // Хранилище всех нарисованных фигур
    std::optional<Point> m_start_pt;

    // --- Виджеты ---
    Gtk::Box m_main_box{Gtk::ORIENTATION_VERTICAL, 5};
    Gtk::Box m_control_box{Gtk::ORIENTATION_HORIZONTAL, 6};
    
    Gtk::Label m_algo_label{"Algorithm:"};
    Gtk::ComboBoxText m_algo_combo;
    
    Gtk::Label m_start_label{"Start x,y:"};
    Gtk::Entry m_start_x_entry, m_start_y_entry;
    
    Gtk::Label m_end_label{"End x,y:"};
    Gtk::Entry m_end_x_entry, m_end_y_entry;
    
    Gtk::Button m_draw_button{"Draw coords"};
    
    Gtk::Label m_scale_label{"Scale(px/cell):"};
    Gtk::Entry m_scale_entry;
    Gtk::Button m_apply_scale_button{"Apply"};
    
    Gtk::Button m_clear_all_button{"Clear All"};
    Gtk::Button m_benchmark_button{"Benchmark"};
    
    Gtk::DrawingArea m_drawing_area;
    
    Gtk::ScrolledWindow m_info_scrolled_window;
    Gtk::TextView m_info_text;
    Glib::RefPtr<Gtk::TextBuffer> m_info_buffer;
};

RasterizationApp::RasterizationApp() {
    set_title("Rasterization with C++ / Gtkmm");
    set_default_size(m_canvas_w, m_canvas_h + 200); // +200 для UI и лога
    set_position(Gtk::WIN_POS_CENTER);

    // --- Инициализация состояния ---
    m_colors = {
        {"Step-by-step", ColorHelpers::hex_to_rgba("#d62728")},
        {"DDA", ColorHelpers::hex_to_rgba("#2ca02c")},
        {"Bresenham", ColorHelpers::hex_to_rgba("#1f77b4")},
        {"Wu", ColorHelpers::hex_to_rgba("#9467bd")},
        {"Castle-Pitteway", ColorHelpers::hex_to_rgba("#ff7f0e")},
        {"Circle", ColorHelpers::hex_to_rgba("#17becf")}
    };

    // --- Сборка UI ---
    add(m_main_box);

    // 1. Панель управления
    m_main_box.pack_start(m_control_box, Gtk::PACK_SHRINK, 5);
    
    m_control_box.pack_start(m_algo_label, Gtk::PACK_SHRINK);
    m_algo_combo.append("Step-by-step");
    m_algo_combo.append("DDA");
    m_algo_combo.append("Bresenham");
    m_algo_combo.append("Wu");
    m_algo_combo.append("Castle-Pitteway");
    m_algo_combo.append("Circle");
    m_algo_combo.set_active_text("Bresenham");
    m_control_box.pack_start(m_algo_combo, Gtk::PACK_SHRINK);

    m_control_box.pack_start(m_start_label, Gtk::PACK_SHRINK);
    m_start_x_entry.set_width_chars(4); m_start_x_entry.set_text("0");
    m_start_y_entry.set_width_chars(4); m_start_y_entry.set_text("0");
    m_control_box.pack_start(m_start_x_entry, Gtk::PACK_SHRINK);
    m_control_box.pack_start(m_start_y_entry, Gtk::PACK_SHRINK);

    m_control_box.pack_start(m_end_label, Gtk::PACK_SHRINK);
    m_end_x_entry.set_width_chars(4); m_end_x_entry.set_text("20");
    m_end_y_entry.set_width_chars(4); m_end_y_entry.set_text("10");
    m_control_box.pack_start(m_end_x_entry, Gtk::PACK_SHRINK);
    m_control_box.pack_start(m_end_y_entry, Gtk::PACK_SHRINK);

    m_control_box.pack_start(m_draw_button, Gtk::PACK_SHRINK);

    m_control_box.pack_start(m_scale_label, Gtk::PACK_SHRINK);
    m_scale_entry.set_width_chars(4);
    m_scale_entry.set_text(std::to_string(m_pixel_size));
    m_control_box.pack_start(m_scale_entry, Gtk::PACK_SHRINK);
    m_control_box.pack_start(m_apply_scale_button, Gtk::PACK_SHRINK);
    
    m_control_box.pack_start(m_clear_all_button, Gtk::PACK_SHRINK);
    m_control_box.pack_start(m_benchmark_button, Gtk::PACK_SHRINK);

    // 2. Область рисования
    m_drawing_area.set_size_request(m_canvas_w, m_canvas_h);
    m_main_box.pack_start(m_drawing_area, Gtk::PACK_EXPAND_WIDGET);
    
    // 3. Окно лога
    m_info_buffer = Gtk::TextBuffer::create();
    m_info_text.set_buffer(m_info_buffer);
    m_info_text.set_editable(false);
    m_info_scrolled_window.add(m_info_text);
    m_info_scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_info_scrolled_window.set_min_content_height(100);
    m_main_box.pack_start(m_info_scrolled_window, Gtk::PACK_SHRINK);
    
    // --- Подключение сигналов ---
    
    // Рисование
    m_drawing_area.signal_draw().connect(sigc::mem_fun(*this, &RasterizationApp::on_drawing_area_draw));
    
    // Клик мыши
    m_drawing_area.add_events(Gdk::BUTTON_PRESS_MASK);
    m_drawing_area.signal_button_press_event().connect(sigc::mem_fun(*this, &RasterizationApp::on_drawing_area_button_press));

    // Кнопки
    m_draw_button.signal_clicked().connect(sigc::mem_fun(*this, &RasterizationApp::on_draw_from_entries));
    m_apply_scale_button.signal_clicked().connect(sigc::mem_fun(*this, &RasterizationApp::on_apply_scale));
    m_clear_all_button.signal_clicked().connect(sigc::mem_fun(*this, &RasterizationApp::on_clear_all));
    m_benchmark_button.signal_clicked().connect(sigc::mem_fun(*this, &RasterizationApp::on_benchmark));

    // --- Запуск ---
    draw_legend();
    show_all_children();
}

void RasterizationApp::log_info(const std::string& msg) {
    m_info_buffer->insert(m_info_buffer->end(), msg + "\n");
    // Автопрокрутка вниз
    auto adjustment = m_info_scrolled_window.get_vadjustment();
    adjustment->set_value(adjustment->get_upper());
}

void RasterizationApp::draw_legend() {
    m_info_buffer->set_text("");
    log_info("Algorithm colors:");
    for (const auto& pair : m_colors) {
        log_info("  " + pair.first); // Gtkmm TextBuffer не поддерживает цвета так просто, как Tkinter
    }
    log_info("Click on canvas to pick start and end (first click = start, second = end).");
    log_info("Or input coordinates in fields and press Draw coords.");
    log_info("Use 'Clear All' to remove all drawn shapes.");
}

// --- Трансформации Координат ---

Point RasterizationApp::screen_to_grid(double sx, double sy) {
    // Получаем актуальные размеры области рисования
    int w = m_drawing_area.get_allocated_width();
    int h = m_drawing_area.get_allocated_height();
    double ox = w / 2.0;
    double oy = h / 2.0;
    
    int gx = static_cast<int>(std::round((sx - ox) / m_pixel_size));
    int gy = static_cast<int>(std::round((oy - sy) / m_pixel_size));
    return {gx, gy};
}

std::pair<double, double> RasterizationApp::grid_to_screen(int gx, int gy) {
    int w = m_drawing_area.get_allocated_width();
    int h = m_drawing_area.get_allocated_height();
    double ox = w / 2.0;
    double oy = h / 2.0;

    double sx = ox + gx * m_pixel_size;
    double sy = oy - gy * m_pixel_size; // Y инвертирован
    return {sx, sy};
}


// --- Логика Рисования (Cairo) ---

bool RasterizationApp::on_drawing_area_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    // 1. Очистить фон (белый)
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->paint();

    // 2. Нарисовать сетку и оси
    draw_grid_and_axes(cr);

    // 3. Нарисовать все сохраненные фигуры
    draw_all_shapes(cr);

    return true; // Cигнал обработан
}

void RasterizationApp::draw_grid_and_axes(const Cairo::RefPtr<Cairo::Context>& cr) {
    int w = m_drawing_area.get_allocated_width();
    int h = m_drawing_area.get_allocated_height();
    double ox = w / 2.0;
    double oy = h / 2.0;
    int ps = m_pixel_size;

    int max_x = w / ps;
    int max_y = h / ps;
    
    cr->set_line_width(1.0);
    
    // Вертикальные линии
    for (int i = -max_x; i <= max_x; ++i) {
        double x = ox + i * ps;
        if (i % 10 == 0) cr->set_source_rgb(0.6, 0.6, 0.6); // #999
        else if (i % 5 == 0) cr->set_source_rgb(0.8, 0.8, 0.8); // #ccc
        else cr->set_source_rgb(0.93, 0.93, 0.93); // #eee
        
        cr->move_to(x, 0);
        cr->line_to(x, h);
        cr->stroke();
    }

    // Горизонтальные линии
    for (int j = -max_y; j <= max_y; ++j) {
        double y = oy - j * ps;
        if (j % 10 == 0) cr->set_source_rgb(0.6, 0.6, 0.6);
        else if (j % 5 == 0) cr->set_source_rgb(0.8, 0.8, 0.8);
        else cr->set_source_rgb(0.93, 0.93, 0.93);
        
        cr->move_to(0, y);
        cr->line_to(w, y);
        cr->stroke();
    }

    // Оси
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->set_line_width(2.0);
    cr->move_to(0, oy); cr->line_to(w, oy); // X-ось
    cr->move_to(ox, 0); cr->line_to(ox, h); // Y-ось
    cr->stroke();
    
    // Подписи осей (просто)
    cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
    cr->set_font_size(10);
    cr->move_to(ox + 10, oy - 10); cr->show_text("+X");
    cr->move_to(ox + 10, oy + 20); cr->show_text("+Y"); // Инвертировано!
}

void RasterizationApp::draw_all_shapes(const Cairo::RefPtr<Cairo::Context>& cr) {
    for (const auto& shape : m_shapes) {
        for (const auto& pixel : shape.pixels) {
            draw_pixel(cr, pixel, shape.color);
        }
    }
}

void RasterizationApp::draw_pixel(const Cairo::RefPtr<Cairo::Context>& cr, const Pixel& p, const Gdk::RGBA& base_color) {
    auto [sx, sy] = grid_to_screen(p.x, p.y);
    
    // Применить интенсивность (anti-aliasing)
    Gdk::RGBA final_color = ColorHelpers::blend_with_white(base_color, p.intensity);
    cr->set_source_rgba(final_color.get_red(), final_color.get_green(), final_color.get_blue(), final_color.get_alpha());
    
    // Рисуем квадрат (пиксель)
    // (sx, sy) - это ВЕРХНИЙ ЛЕВЫЙ угол ячейки (по соглашению Python)
    cr->rectangle(sx, sy, m_pixel_size, m_pixel_size);
    cr->fill();
}


// --- Обработчики Действий ---

void RasterizationApp::draw_line(Point p0, Point p1) {
    std::string algo = m_algo_combo.get_active_text();
    Gdk::RGBA color = m_colors[algo];
    
    Shape new_shape;
    new_shape.algo_name = algo;
    new_shape.color = color;
    
    if (algo == "Step-by-step") {
        for(auto& p : RasterizationAlgorithms::step_by_step(p0.x, p0.y, p1.x, p1.y))
            new_shape.pixels.push_back({p.x, p.y, 1.0});
    } else if (algo == "DDA") {
        for(auto& p : RasterizationAlgorithms::dda(p0.x, p0.y, p1.x, p1.y))
            new_shape.pixels.push_back({p.x, p.y, 1.0});
    } else if (algo == "Bresenham") {
        for(auto& p : RasterizationAlgorithms::bresenham_line(p0.x, p0.y, p1.x, p1.y))
            new_shape.pixels.push_back({p.x, p.y, 1.0});
    } else if (algo == "Wu") {
        new_shape.pixels = RasterizationAlgorithms::wu_line(p0.x, p0.y, p1.x, p1.y);
    } else if (algo == "Castle-Pitteway") {
        new_shape.pixels = RasterizationAlgorithms::castle_pitteway(p0.x, p0.y, p1.x, p1.y);
    } else if (algo == "Circle") {
        int r = static_cast<int>(std::round(std::hypot(p1.x - p0.x, p1.y - p0.y)));
        for(auto& p : RasterizationAlgorithms::bresenham_circle(p0.x, p0.y, r))
            new_shape.pixels.push_back({p.x, p.y, 1.0});
    }
    
    m_shapes.push_back(new_shape);
    
    // Запросить перерисовку
    m_drawing_area.queue_draw();
    
    log_info("Appended " + std::to_string(new_shape.pixels.size()) + " pixels using " + algo);
}

void RasterizationApp::on_draw_from_entries() {
    try {
        int x0 = std::stoi(m_start_x_entry.get_text());
        int y0 = std::stoi(m_start_y_entry.get_text());
        int x1 = std::stoi(m_end_x_entry.get_text());
        int y1 = std::stoi(m_end_y_entry.get_text());
        
        draw_line({x0, y0}, {x1, y1});
        
        // Сбросить точки клика
        m_start_pt.reset();

    } catch (const std::exception& e) {
        log_info("Invalid integer coordinates!");
    }
}

void RasterizationApp::on_apply_scale() {
    try {
        int s = std::stoi(m_scale_entry.get_text());
        if (s <= 0) throw std::invalid_argument("Scale must be positive");
        m_pixel_size = s;
        // Запросить полную перерисовку, т.к. изменилась сетка
        m_drawing_area.queue_draw();
        log_info("Scale set to " + std::to_string(s));
    } catch (const std::exception& e) {
        log_info("Invalid scale value!");
    }
}

void RasterizationApp::on_clear_all() {
    m_shapes.clear();
    m_drawing_area.queue_draw();
    log_info("Cleared all shapes.");
    // Очистим и легенду
    draw_legend();
}

bool RasterizationApp::on_drawing_area_button_press(GdkEventButton* event) {
    if (event->button == 1) { // Левая кнопка
        Point grid_pt = screen_to_grid(event->x, event->y);
        
        if (!m_start_pt.has_value()) {
            // Это первый клик
            m_start_pt = grid_pt;
            m_start_x_entry.set_text(std::to_string(grid_pt.x));
            m_start_y_entry.set_text(std::to_string(grid_pt.y));
            log_info("Start set to (" + std::to_string(grid_pt.x) + ", " + std::to_string(grid_pt.y) + ")");
        } else {
            // Это второй клик
            Point end_pt = grid_pt;
            m_end_x_entry.set_text(std::to_string(end_pt.x));
            m_end_y_entry.set_text(std::to_string(end_pt.y));
            log_info("End set to (" + std::to_string(end_pt.x) + ", " + std::to_string(end_pt.y) + ") — drawing...");
            
            draw_line(m_start_pt.value(), end_pt);
            
            // Сброс для следующей линии
            m_start_pt.reset();
        }
        return true; // Событие обработано
    }
    return false;
}

void RasterizationApp::on_benchmark() {
    log_info("Running benchmark...");
    
    // Запускаем в отдельном потоке, чтобы не блокировать GUI (хотя в данном случае это быстро)
    auto results = PerformanceTester::benchmark_all();
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2); // Форматирование вывода
    for (const auto& res : results) {
        ss.str(""); // Очистить stringstream
        ss << "  " << res.name << ": " << res.time_us << " us, pixels=" << res.pixel_count;
        log_info(ss.str());
    }
    log_info("Benchmark complete.");
}


// --- Точка входа ---

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.example.rasterization");

    RasterizationApp window;

    return app->run(window);
}