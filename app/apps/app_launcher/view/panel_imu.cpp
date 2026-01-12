/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-imu";

static constexpr int16_t _wireframe_pos_x = 270;
static constexpr int16_t _wireframe_pos_y = 230;
static constexpr int16_t _wireframe_w     = 180;
static constexpr int16_t _wireframe_h     = 110;
static constexpr uint32_t _wireframe_color = 0x3AD86D;

namespace {

constexpr float kRadToDeg = 57.2957795f;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Edge {
    uint16_t a;
    uint16_t b;
};

static constexpr Vec3 kShipVertices[] = {
    {0.650832f, 0.271315f, -1.000000f},
    {1.240349f, -0.000333f, 1.000000f},
    {0.650832f, 0.315481f, -1.000000f},
    {1.240348f, 0.180615f, 0.560630f},
    {0.805783f, -0.000334f, 4.284117f},
    {0.805782f, 0.180615f, 4.284118f},
    {3.119281f, -0.942804f, 2.322021f},
    {3.119280f, -0.761855f, 1.882651f},
    {0.650832f, 0.315481f, -1.000000f},
    {1.240348f, 0.180615f, 0.560630f},
    {3.119280f, -0.761855f, 1.882651f},
    {0.507604f, 0.512063f, -1.000000f},
    {0.556329f, 0.986010f, 1.000001f},
    {0.218978f, 0.452316f, 3.753458f},
    {1.240349f, -0.000333f, -0.372761f},
    {1.240349f, 0.180615f, -0.372760f},
    {3.119281f, -0.942804f, 0.949260f},
    {3.119281f, -0.761855f, 0.949260f},
    {1.240349f, 0.180615f, -0.372760f},
    {3.119281f, -0.761855f, 0.949260f},
    {0.653542f, 0.986010f, -0.372760f},
    {3.457423f, -0.942803f, 2.322021f},
    {3.457423f, -0.761854f, 1.882651f},
    {3.457423f, -0.942803f, 0.949260f},
    {3.457423f, -0.761854f, 0.949261f},
    {3.119281f, -0.942804f, 2.322021f},
    {3.119280f, -0.761855f, 2.321895f},
    {3.457423f, -0.942803f, 2.322021f},
    {3.457423f, -0.761854f, 2.321895f},
    {3.182715f, -0.908858f, 2.321997f},
    {3.182715f, -0.795800f, 2.321919f},
    {3.393988f, -0.908857f, 2.321997f},
    {3.393988f, -0.795800f, 2.321919f},
    {3.182715f, -0.907922f, 3.673004f},
    {3.182715f, -0.794865f, 3.672926f},
    {3.393988f, -0.907922f, 3.673004f},
    {3.393988f, -0.794865f, 3.672926f},
    {3.242466f, -0.875948f, 3.672982f},
    {3.242466f, -0.826839f, 3.672948f},
    {3.334237f, -0.875948f, 3.672982f},
    {3.334237f, -0.826839f, 3.672948f},
    {3.242466f, -0.875760f, 3.945883f},
    {3.242466f, -0.826651f, 3.945849f},
    {3.334237f, -0.875760f, 3.945883f},
    {3.334237f, -0.826651f, 3.945849f},
    {0.960107f, 0.565249f, 0.770461f},
    {0.525541f, 0.310371f, 4.030690f},
    {0.582431f, 0.409363f, -1.000000f},
    {0.960107f, 0.565249f, -0.372760f},
    {0.845504f, 0.498033f, 1.630241f},
    {0.538940f, 0.845265f, 1.726136f},
    {1.125746f, 0.180615f, 1.542580f},
    {1.125746f, -0.000333f, 1.866080f},
    {-0.650832f, 0.271315f, -1.000000f},
    {-1.240349f, -0.000333f, 1.000000f},
    {0.000000f, -0.000333f, 1.000000f},
    {0.000000f, 0.271315f, -1.000000f},
    {-0.650832f, 0.315481f, -1.000000f},
    {-1.240348f, 0.180615f, 0.560630f},
    {0.000000f, 0.315481f, -1.000000f},
    {-0.805783f, -0.000334f, 4.284117f},
    {0.000000f, -0.000334f, 5.383713f},
    {-0.805782f, 0.180615f, 4.284118f},
    {0.000000f, 0.180615f, 5.383713f},
    {-3.119281f, -0.942804f, 2.322021f},
    {-3.119280f, -0.761855f, 1.882651f},
    {-0.650832f, 0.315481f, -1.000000f},
    {-1.240348f, 0.180615f, 0.560630f},
    {-3.119280f, -0.761855f, 1.882651f},
    {-0.507604f, 0.512063f, -1.000000f},
    {-0.556329f, 0.986010f, 1.000001f},
    {0.000000f, 0.986010f, 1.000000f},
    {0.000000f, 0.512063f, -1.000000f},
    {-0.218978f, 0.452316f, 3.753458f},
    {0.000000f, 0.452316f, 4.853053f},
    {-1.240349f, -0.000333f, -0.372761f},
    {0.000000f, -0.000333f, -0.372761f},
    {-1.240349f, 0.180615f, -0.372760f},
    {-3.119281f, -0.942804f, 0.949260f},
    {-3.119281f, -0.761855f, 0.949260f},
    {-1.240349f, 0.180615f, -0.372760f},
    {-3.119281f, -0.761855f, 0.949260f},
    {-0.653542f, 0.986010f, -0.372760f},
    {0.000000f, 0.986010f, -0.372761f},
    {-3.457423f, -0.942803f, 2.322021f},
    {-3.457423f, -0.761854f, 1.882651f},
    {-3.457423f, -0.942803f, 0.949260f},
    {-3.457423f, -0.761854f, 0.949261f},
    {-3.119281f, -0.942804f, 2.322021f},
    {-3.119280f, -0.761855f, 2.321895f},
    {-3.457423f, -0.942803f, 2.322021f},
    {-3.457423f, -0.761854f, 2.321895f},
    {-3.182715f, -0.908858f, 2.321997f},
    {-3.182715f, -0.795800f, 2.321919f},
    {-3.393988f, -0.908857f, 2.321997f},
    {-3.393988f, -0.795800f, 2.321919f},
    {-3.182715f, -0.907922f, 3.673004f},
    {-3.182715f, -0.794865f, 3.672926f},
    {-3.393988f, -0.907922f, 3.673004f},
    {-3.393988f, -0.794865f, 3.672926f},
    {-3.242466f, -0.875948f, 3.672982f},
    {-3.242466f, -0.826839f, 3.672948f},
    {-3.334237f, -0.875948f, 3.672982f},
    {-3.334237f, -0.826839f, 3.672948f},
    {-3.242466f, -0.875760f, 3.945883f},
    {-3.242466f, -0.826651f, 3.945849f},
    {-3.334237f, -0.875760f, 3.945883f},
    {-3.334237f, -0.826651f, 3.945849f},
    {0.000000f, 0.409363f, -1.000000f},
    {-0.960107f, 0.565249f, 0.770461f},
    {-0.525541f, 0.310371f, 4.030690f},
    {-0.582431f, 0.409363f, -1.000000f},
    {0.000000f, 0.310371f, 5.130285f},
    {-0.960107f, 0.565249f, -0.372760f},
    {-0.845504f, 0.498033f, 1.630241f},
    {-0.538940f, 0.845265f, 1.726136f},
    {0.000000f, 0.948491f, 1.700656f},
    {0.000000f, -0.000333f, 2.156063f},
    {-1.125746f, 0.180615f, 1.542580f},
    {-1.125746f, -0.000333f, 1.866080f},
};

static constexpr Edge kShipEdges[] = {
    {0, 2},
    {0, 14},
    {0, 56},
    {1, 3},
    {1, 6},
    {1, 14},
    {1, 52},
    {1, 55},
    {2, 8},
    {2, 15},
    {2, 47},
    {2, 59},
    {3, 7},
    {3, 9},
    {3, 15},
    {3, 45},
    {3, 51},
    {4, 5},
    {4, 52},
    {4, 61},
    {5, 46},
    {5, 51},
    {5, 63},
    {6, 7},
    {6, 16},
    {6, 21},
    {6, 25},
    {7, 10},
    {7, 17},
    {7, 22},
    {7, 26},
    {8, 18},
    {9, 10},
    {9, 18},
    {10, 19},
    {11, 20},
    {11, 47},
    {11, 72},
    {12, 20},
    {12, 45},
    {12, 50},
    {12, 71},
    {13, 46},
    {13, 50},
    {13, 74},
    {14, 15},
    {14, 16},
    {14, 18},
    {14, 76},
    {15, 18},
    {15, 48},
    {16, 17},
    {16, 19},
    {16, 23},
    {17, 19},
    {17, 24},
    {18, 19},
    {20, 48},
    {20, 83},
    {21, 22},
    {21, 23},
    {21, 27},
    {22, 24},
    {22, 28},
    {23, 24},
    {25, 26},
    {25, 27},
    {25, 29},
    {26, 28},
    {26, 30},
    {27, 28},
    {27, 31},
    {28, 32},
    {29, 30},
    {29, 31},
    {29, 33},
    {30, 32},
    {30, 34},
    {31, 32},
    {31, 35},
    {32, 36},
    {33, 34},
    {33, 35},
    {33, 37},
    {34, 36},
    {34, 38},
    {35, 36},
    {35, 39},
    {36, 40},
    {37, 38},
    {37, 39},
    {37, 41},
    {38, 40},
    {38, 42},
    {39, 40},
    {39, 43},
    {40, 44},
    {41, 42},
    {41, 43},
    {42, 44},
    {43, 44},
    {45, 48},
    {45, 49},
    {46, 49},
    {46, 112},
    {47, 48},
    {47, 108},
    {49, 50},
    {49, 51},
    {50, 116},
    {51, 52},
    {52, 117},
    {53, 56},
    {53, 57},
    {53, 75},
    {54, 55},
    {54, 58},
    {54, 64},
    {54, 75},
    {54, 119},
    {55, 76},
    {55, 117},
    {56, 59},
    {56, 76},
    {57, 59},
    {57, 66},
    {57, 77},
    {57, 111},
    {58, 65},
    {58, 67},
    {58, 77},
    {58, 109},
    {58, 118},
    {59, 108},
    {60, 61},
    {60, 62},
    {60, 119},
    {61, 63},
    {61, 117},
    {62, 63},
    {62, 110},
    {62, 118},
    {63, 112},
    {64, 65},
    {64, 78},
    {64, 84},
    {64, 88},
    {65, 68},
    {65, 79},
    {65, 85},
    {65, 89},
    {66, 80},
    {67, 68},
    {67, 80},
    {68, 81},
    {69, 72},
    {69, 82},
    {69, 111},
    {70, 71},
    {70, 82},
    {70, 109},
    {70, 115},
    {71, 83},
    {71, 116},
    {72, 83},
    {72, 108},
    {73, 74},
    {73, 110},
    {73, 115},
    {74, 112},
    {74, 116},
    {75, 76},
    {75, 77},
    {75, 78},
    {75, 80},
    {77, 80},
    {77, 113},
    {78, 79},
    {78, 81},
    {78, 86},
    {79, 81},
    {79, 87},
    {80, 81},
    {82, 83},
    {82, 113},
    {84, 85},
    {84, 86},
    {84, 90},
    {85, 87},
    {85, 91},
    {86, 87},
    {88, 89},
    {88, 90},
    {88, 92},
    {89, 91},
    {89, 93},
    {90, 91},
    {90, 94},
    {91, 95},
    {92, 93},
    {92, 94},
    {92, 96},
    {93, 95},
    {93, 97},
    {94, 95},
    {94, 98},
    {95, 99},
    {96, 97},
    {96, 98},
    {96, 100},
    {97, 99},
    {97, 101},
    {98, 99},
    {98, 102},
    {99, 103},
    {100, 101},
    {100, 102},
    {100, 104},
    {101, 103},
    {101, 105},
    {102, 103},
    {102, 106},
    {103, 107},
    {104, 105},
    {104, 106},
    {105, 107},
    {106, 107},
    {108, 111},
    {109, 113},
    {109, 114},
    {110, 112},
    {110, 114},
    {111, 113},
    {114, 115},
    {114, 118},
    {115, 116},
    {117, 119},
    {118, 119},
};

struct ModelBounds {
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
};

static ModelBounds ship_bounds()
{
    ModelBounds bounds;
    bounds.min_x = bounds.max_x = kShipVertices[0].x;
    bounds.min_y = bounds.max_y = kShipVertices[0].y;
    bounds.min_z = bounds.max_z = kShipVertices[0].z;
    for (size_t i = 1; i < (sizeof(kShipVertices) / sizeof(kShipVertices[0])); ++i) {
        const auto& v = kShipVertices[i];
        bounds.min_x = std::min(bounds.min_x, v.x);
        bounds.max_x = std::max(bounds.max_x, v.x);
        bounds.min_y = std::min(bounds.min_y, v.y);
        bounds.max_y = std::max(bounds.max_y, v.y);
        bounds.min_z = std::min(bounds.min_z, v.z);
        bounds.max_z = std::max(bounds.max_z, v.z);
    }
    return bounds;
}
} // namespace

namespace launcher_view {

struct ShipWireframe {
    lv_obj_t* container = nullptr;
    std::vector<lv_obj_t*> lines;
    std::vector<std::array<lv_point_precise_t, 2>> line_points;
    std::vector<lv_point_precise_t> projected_points;
    float center_x = 0.0f;
    float center_y = 0.0f;
    float center_z = 0.0f;
    float scale = 1.0f;
    float depth = 1.0f;
    float origin_x = 0.0f;
    float origin_y = 0.0f;

    void init(lv_obj_t* parent, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y, lv_color_t color, int line_width)
    {
        container = lv_obj_create(parent);
        lv_obj_set_size(container, w, h);
        lv_obj_align(container, LV_ALIGN_LEFT_MID, x, y);
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

        ModelBounds bounds = ship_bounds();
        center_x = (bounds.min_x + bounds.max_x) * 0.5f;
        center_y = (bounds.min_y + bounds.max_y) * 0.5f;
        center_z = (bounds.min_z + bounds.max_z) * 0.5f;

        float model_w = bounds.max_x - bounds.min_x;
        float model_h = bounds.max_y - bounds.min_y;
        float model_z = bounds.max_z - bounds.min_z;
        float scale_x = static_cast<float>(w) / model_w;
        float scale_y = static_cast<float>(h) / model_h;
        scale = 0.82f * std::min(scale_x, scale_y);
        depth = model_z * 2.0f;
        if (depth <= 0.0f) {
            depth = 1.0f;
        }
        origin_x = static_cast<float>(w) * 0.5f;
        origin_y = static_cast<float>(h) * 0.5f;

        size_t edge_count = sizeof(kShipEdges) / sizeof(kShipEdges[0]);
        size_t vertex_count = sizeof(kShipVertices) / sizeof(kShipVertices[0]);
        lines.reserve(edge_count);
        line_points.assign(edge_count, {{{0, 0}, {0, 0}}});
        projected_points.assign(vertex_count, {0, 0});

        for (size_t i = 0; i < edge_count; ++i) {
            lv_obj_t* line = lv_line_create(container);
            lv_line_set_points(line, line_points[i].data(), line_points[i].size());
            lv_obj_set_style_line_color(line, color, 0);
            lv_obj_set_style_line_width(line, line_width, 0);
            lv_obj_set_style_line_rounded(line, true, 0);
            lines.push_back(line);
        }
    }

    void update(float roll_deg, float pitch_deg)
    {
        if (!container) {
            return;
        }

        float roll = pitch_deg / kRadToDeg;
        float pitch = roll_deg / kRadToDeg;
        float cos_r = std::cos(roll);
        float sin_r = std::sin(roll);
        float cos_p = std::cos(pitch);
        float sin_p = std::sin(pitch);

        size_t vertex_count = projected_points.size();
        for (size_t i = 0; i < vertex_count; ++i) {
            const auto& v = kShipVertices[i];
            float x = v.x - center_x;
            float y = v.y - center_y;
            float z = v.z - center_z;

            float y1 = (y * cos_p) - (z * sin_p);
            float z1 = (y * sin_p) + (z * cos_p);
            float x1 = x;

            float x2 = (x1 * cos_r) - (y1 * sin_r);
            float y2 = (x1 * sin_r) + (y1 * cos_r);
            float z2 = z1;

            float perspective = depth / (depth + z2 + depth * 0.15f);
            float px = origin_x + (x2 * scale * perspective);
            float py = origin_y - (y2 * scale * perspective);

            projected_points[i].x = static_cast<lv_coord_t>(px);
            projected_points[i].y = static_cast<lv_coord_t>(py);
        }

        for (size_t i = 0; i < line_points.size(); ++i) {
            const auto& edge = kShipEdges[i];
            line_points[i][0] = projected_points[edge.a];
            line_points[i][1] = projected_points[edge.b];
            lv_line_set_points(lines[i], line_points[i].data(), line_points[i].size());
        }
    }
};

void ShipWireframeDeleter::operator()(ShipWireframe* wireframe) const
{
    delete wireframe;
}

} // namespace launcher_view

namespace {

class ImuDetailWindow : public ui::Window {
public:
    ImuDetailWindow()
    {
        config.title    = "IMU Attitude";
        config.kfClosed = {720, -260, 64, 64, 0};
        config.kfOpened = {220, 90, 740, 520, 255};
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _ship_wireframe.init(_window->get(), 360, 220, 36, -10, lv_color_hex(_wireframe_color), 3);

        _label_roll = std::make_unique<Label>(_window->get());
        _label_roll->align(LV_ALIGN_TOP_RIGHT, -40, 60);
        _label_roll->setTextFont(&lv_font_montserrat_24);
        _label_roll->setTextColor(lv_color_hex(0xB6F6CC));

        _label_pitch = std::make_unique<Label>(_window->get());
        _label_pitch->align(LV_ALIGN_TOP_RIGHT, -40, 92);
        _label_pitch->setTextFont(&lv_font_montserrat_24);
        _label_pitch->setTextColor(lv_color_hex(0xB6F6CC));

        _label_accel_x = std::make_unique<Label>(_window->get());
        _label_accel_x->align(LV_ALIGN_TOP_RIGHT, -40, 150);
        _label_accel_x->setTextFont(&lv_font_montserrat_22);
        _label_accel_x->setTextColor(lv_color_hex(0xD7E7F0));

        _label_accel_y = std::make_unique<Label>(_window->get());
        _label_accel_y->align(LV_ALIGN_TOP_RIGHT, -40, 182);
        _label_accel_y->setTextFont(&lv_font_montserrat_22);
        _label_accel_y->setTextColor(lv_color_hex(0xD7E7F0));

        _label_accel_z = std::make_unique<Label>(_window->get());
        _label_accel_z->align(LV_ALIGN_TOP_RIGHT, -40, 214);
        _label_accel_z->setTextFont(&lv_font_montserrat_22);
        _label_accel_z->setTextColor(lv_color_hex(0xD7E7F0));

        _label_hint = std::make_unique<Label>(_window->get());
        _label_hint->align(LV_ALIGN_BOTTOM_RIGHT, -40, -40);
        _label_hint->setTextFont(&lv_font_montserrat_16);
        _label_hint->setTextColor(lv_color_hex(0x5C7A6A));
        _label_hint->setText("Tap outside to close");
    }

    void setImuData(float accel_x, float accel_y, float accel_z, float pitch_deg, float roll_deg)
    {
        if (_label_roll) {
            _label_roll->setText(fmt::format("Roll: {:+.2f}°", roll_deg));
        }
        if (_label_pitch) {
            _label_pitch->setText(fmt::format("Pitch: {:+.2f}°", pitch_deg));
        }
        if (_label_accel_x) {
            _label_accel_x->setText(fmt::format("Accel X: {:+.3f} g", accel_x));
        }
        if (_label_accel_y) {
            _label_accel_y->setText(fmt::format("Accel Y: {:+.3f} g", accel_y));
        }
        if (_label_accel_z) {
            _label_accel_z->setText(fmt::format("Accel Z: {:+.3f} g", accel_z));
        }

        _ship_wireframe.update(roll_deg, pitch_deg);
    }

private:
    ShipWireframe _ship_wireframe;
    std::unique_ptr<Label> _label_roll;
    std::unique_ptr<Label> _label_pitch;
    std::unique_ptr<Label> _label_accel_x;
    std::unique_ptr<Label> _label_accel_y;
    std::unique_ptr<Label> _label_accel_z;
    std::unique_ptr<Label> _label_hint;
};

} // namespace

launcher_view::PanelImu::~PanelImu() = default;

void PanelImu::init()
{
    _wireframe = std::make_unique<Container>(lv_screen_active());
    _wireframe->setSize(_wireframe_w, _wireframe_h);
    _wireframe->align(LV_ALIGN_LEFT_MID, _wireframe_pos_x, _wireframe_pos_y);
    _wireframe->setBgColor(lv_color_hex(0x071218));
    _wireframe->setBorderWidth(1);
    _wireframe->setBorderColor(lv_color_hex(0x1E3C2A));
    _wireframe->setRadius(10);
    _wireframe->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _label_attitude = std::make_unique<Label>(_wireframe->get());
    _label_attitude->align(LV_ALIGN_TOP_LEFT, 10, 6);
    _label_attitude->setTextFont(&lv_font_montserrat_14);
    _label_attitude->setTextColor(lv_color_hex(0x4FD88E));
    _label_attitude->setText("ATTITUDE");

    _ship_wireframe = std::unique_ptr<ShipWireframe, ShipWireframeDeleter>(new ShipWireframe());
    _ship_wireframe->init(_wireframe->get(), 170, 96, 5, 16, lv_color_hex(_wireframe_color), 2);

    _wireframe->onClick().connect([&]() {
        if (_window) {
            return;
        }

        audio::play_next_tone_progression();
        _window = std::make_unique<ImuDetailWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelImu::update(bool isStacked)
{
    if (GetHAL()->millis() - _time_count < 100) {
        return;
    }

    GetHAL()->updateImuData();

    float accel_x = GetHAL()->imuData.accelX;
    float accel_y = GetHAL()->imuData.accelY;
    float accel_z = GetHAL()->imuData.accelZ;

    float roll_deg = std::atan2(accel_y, accel_z) * kRadToDeg;
    float pitch_deg = std::atan2(-accel_x, std::sqrt(accel_y * accel_y + accel_z * accel_z)) * kRadToDeg;

    if (_ship_wireframe) {
        _ship_wireframe->update(roll_deg, pitch_deg);
    }

    if (_window) {
        _window->update();
        auto* imu_window = static_cast<ImuDetailWindow*>(_window.get());
        imu_window->setImuData(accel_x, accel_y, accel_z, pitch_deg, roll_deg);
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }

    _time_count = GetHAL()->millis();
}
