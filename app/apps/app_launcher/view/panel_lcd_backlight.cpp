/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-lcd-bl";

static constexpr int16_t _label_pos_x    = 591;
static constexpr int16_t _label_pos_y    = 94;
static constexpr int16_t _btn_up_pos_x   = 499;
static constexpr int16_t _btn_up_pos_y   = 166;
static constexpr int16_t _btn_down_pos_x = 593;
static constexpr int16_t _btn_down_pos_y = 166;

static constexpr int16_t _midi_up     = 64 + 24;
static constexpr int16_t _midi_down   = 60 + 24;
static constexpr int16_t _midi_top    = 64 + 8 + 24;
static constexpr int16_t _midi_bottom = 60 - 8 + 24;

namespace {

static void stylize_trek_button(lv_obj_t* obj, const char* glyph, lv_color_t accent)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0E2333), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1E465C), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, accent, 0);
    lv_obj_set_style_radius(obj, 18, 0);
    lv_obj_set_style_shadow_width(obj, 12, 0);
    lv_obj_set_style_shadow_color(obj, accent, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_40, 0);

    lv_obj_t* accent_bar = lv_obj_create(obj);
    lv_obj_set_size(accent_bar, 44, 6);
    lv_obj_align(accent_bar, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_bg_color(accent_bar, accent, 0);
    lv_obj_set_style_bg_opa(accent_bar, LV_OPA_90, 0);
    lv_obj_set_style_radius(accent_bar, 3, 0);
    lv_obj_set_style_border_width(accent_bar, 0, 0);

    lv_obj_t* label = lv_label_create(obj);
    lv_label_set_text(label, glyph);
    lv_obj_set_style_text_color(label, accent, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 6);
}

} // namespace

void PanelLcdBacklight::init()
{
    _label_brightness = std::make_unique<Label>(lv_screen_active());
    _label_brightness->align(LV_ALIGN_CENTER, _label_pos_x, _label_pos_y);
    _label_brightness->setTextFont(&lv_font_montserrat_36);
    _label_brightness->setTextColor(lv_color_hex(0xFEFEFE));
    _label_brightness->setText(fmt::format("{}", GetHAL()->getDisplayBrightness()));

    _btn_up = std::make_unique<Container>(lv_screen_active());
    _btn_up->align(LV_ALIGN_CENTER, _btn_up_pos_x, _btn_up_pos_y);
    _btn_up->setSize(81, 85);
    stylize_trek_button(_btn_up->get(), "+", lv_color_hex(0x65DDFD));
    _btn_up->onClick().connect([&]() {
        // Animation
        _label_y_anim.teleport(_label_pos_y - 8);
        _label_y_anim = _label_pos_y;

        // SFX
        if (GetHAL()->getDisplayBrightness() >= 100) {
            audio::play_tone_from_midi(_midi_top);
            return;
        }
        audio::play_tone_from_midi(_midi_up);

        // Update brightness
        int target = GetHAL()->getDisplayBrightness();
        target     = std::clamp(target + 20, 0, 100);
        GetHAL()->setDisplayBrightness(target);
        _label_brightness->setText(fmt::format("{}", GetHAL()->getDisplayBrightness()));
    });

    _btn_down = std::make_unique<Container>(lv_screen_active());
    _btn_down->align(LV_ALIGN_CENTER, _btn_down_pos_x, _btn_down_pos_y);
    _btn_down->setSize(81, 85);
    stylize_trek_button(_btn_down->get(), "-", lv_color_hex(0x5BDAC9));
    _btn_down->onClick().connect([&]() {
        // Animation
        _label_y_anim.teleport(_label_pos_y + 8);
        _label_y_anim = _label_pos_y;

        // SFX
        if (GetHAL()->getDisplayBrightness() <= 20) {
            audio::play_tone_from_midi(_midi_bottom);
            return;
        }
        audio::play_tone_from_midi(_midi_down);

        // Update brightness
        int target = GetHAL()->getDisplayBrightness();
        target     = std::clamp(target - 20, 20, 100);
        GetHAL()->setDisplayBrightness(target);
        _label_brightness->setText(fmt::format("{}", GetHAL()->getDisplayBrightness()));
    });

    _label_y_anim.easingOptions().duration = 0.2;
    _label_y_anim.teleport(_label_pos_y);
}

void PanelLcdBacklight::update(bool isStacked)
{
    if (!_label_y_anim.done()) {
        _label_brightness->setY(_label_y_anim);
    }
}
