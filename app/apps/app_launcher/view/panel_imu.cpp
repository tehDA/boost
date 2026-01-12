/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
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

struct WireframeLine {
    std::vector<lv_point_t> base_points;
    std::vector<lv_point_t> points;
    lv_obj_t* line = nullptr;
};

struct Wireframe {
    lv_obj_t* container = nullptr;
    std::vector<WireframeLine> lines;
    lv_coord_t width = 0;
    lv_coord_t height = 0;

    void init(lv_obj_t* parent, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y, lv_color_t color, int line_width)
    {
        width = w;
        height = h;
        container = lv_obj_create(parent);
        lv_obj_set_size(container, w, h);
        lv_obj_align(container, LV_ALIGN_LEFT_MID, x, y);
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

        auto add_line = [&](std::initializer_list<lv_point_t> pts) {
            WireframeLine wf_line;
            wf_line.base_points.assign(pts.begin(), pts.end());
            wf_line.points = wf_line.base_points;
            wf_line.line = lv_line_create(container);
            lv_line_set_points(wf_line.line, wf_line.points.data(), wf_line.points.size());
            lv_obj_set_style_line_color(wf_line.line, color, 0);
            lv_obj_set_style_line_width(wf_line.line, line_width, 0);
            lv_obj_set_style_line_rounded(wf_line.line, true, 0);
            lines.push_back(std::move(wf_line));
        };

        add_line({{40, 34}, {140, 34}});
        add_line({{30, 44}, {40, 34}});
        add_line({{140, 34}, {150, 44}});
        add_line({{40, 54}, {140, 54}});
        add_line({{80, 54}, {80, 74}});
        add_line({{22, 64}, {55, 64}});
        add_line({{125, 64}, {158, 64}});
        add_line({{55, 58}, {38, 64}});
        add_line({{105, 58}, {122, 64}});
    }

    void update(float roll_deg, float pitch_deg)
    {
        if (!container) {
            return;
        }

        float roll = roll_deg / kRadToDeg;
        float cos_r = std::cos(roll);
        float sin_r = std::sin(roll);

        float pitch_norm = std::clamp(pitch_deg / 45.0f, -1.0f, 1.0f);
        float scale_y = 1.0f - (pitch_norm * 0.12f);
        float shift_y = pitch_norm * 8.0f;

        float cx = width * 0.5f;
        float cy = height * 0.5f;

        for (auto& line : lines) {
            for (size_t i = 0; i < line.base_points.size(); ++i) {
                float dx = line.base_points[i].x - cx;
                float dy = (line.base_points[i].y - cy) * scale_y;

                float rx = dx * cos_r - dy * sin_r;
                float ry = dx * sin_r + dy * cos_r;

                line.points[i].x = static_cast<lv_coord_t>(cx + rx);
                line.points[i].y = static_cast<lv_coord_t>(cy + ry + shift_y);
            }
            lv_line_set_points(line.line, line.points.data(), line.points.size());
        }
    }
};

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

        _wireframe.init(_window->get(), 360, 220, 36, -10, lv_color_hex(_wireframe_color), 3);

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

        _wireframe.update(roll_deg, pitch_deg);
    }

private:
    Wireframe _wireframe;
    std::unique_ptr<Label> _label_roll;
    std::unique_ptr<Label> _label_pitch;
    std::unique_ptr<Label> _label_accel_x;
    std::unique_ptr<Label> _label_accel_y;
    std::unique_ptr<Label> _label_accel_z;
    std::unique_ptr<Label> _label_hint;
};

} // namespace

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

    lv_obj_t* wire_container = lv_obj_create(_wireframe->get());
    lv_obj_set_size(wire_container, 160, 90);
    lv_obj_align(wire_container, LV_ALIGN_TOP_LEFT, 8, 20);
    lv_obj_set_style_bg_opa(wire_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wire_container, 0, 0);
    lv_obj_clear_flag(wire_container, LV_OBJ_FLAG_SCROLLABLE);

    auto add_line = [&](std::initializer_list<lv_point_t> pts) {
        _wire_base_points.emplace_back(pts.begin(), pts.end());
        _wire_points.emplace_back(_wire_base_points.back());
        lv_obj_t* line = lv_line_create(wire_container);
        lv_line_set_points(line, _wire_points.back().data(), _wire_points.back().size());
        lv_obj_set_style_line_color(line, lv_color_hex(_wireframe_color), 0);
        lv_obj_set_style_line_width(line, 2, 0);
        lv_obj_set_style_line_rounded(line, true, 0);
        _wire_lines.push_back(line);
    };

    add_line({{40, 34}, {140, 34}});
    add_line({{30, 44}, {40, 34}});
    add_line({{140, 34}, {150, 44}});
    add_line({{40, 54}, {140, 54}});
    add_line({{80, 54}, {80, 74}});
    add_line({{22, 64}, {55, 64}});
    add_line({{125, 64}, {158, 64}});
    add_line({{55, 58}, {38, 64}});
    add_line({{105, 58}, {122, 64}});

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

    float roll = roll_deg / kRadToDeg;
    float cos_r = std::cos(roll);
    float sin_r = std::sin(roll);

    float pitch_norm = std::clamp(pitch_deg / 45.0f, -1.0f, 1.0f);
    float scale_y = 1.0f - (pitch_norm * 0.12f);
    float shift_y = pitch_norm * 6.0f;

    float cx = 80.0f;
    float cy = 45.0f;

    for (size_t idx = 0; idx < _wire_lines.size(); ++idx) {
        for (size_t i = 0; i < _wire_base_points[idx].size(); ++i) {
            float dx = _wire_base_points[idx][i].x - cx;
            float dy = (_wire_base_points[idx][i].y - cy) * scale_y;

            float rx = dx * cos_r - dy * sin_r;
            float ry = dx * sin_r + dy * cos_r;

            _wire_points[idx][i].x = static_cast<lv_coord_t>(cx + rx);
            _wire_points[idx][i].y = static_cast<lv_coord_t>(cy + ry + shift_y);
        }
        lv_line_set_points(_wire_lines[idx], _wire_points[idx].data(), _wire_points[idx].size());
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
