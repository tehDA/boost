/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
#include <lvgl.h>
#include <hal/hal.h>
#include <memory>
#include <cstdio>

#include "apps/utils/dsp/boost_settings.h"
#include "apps/utils/ui/window.h"
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-spk-vol";

static constexpr int16_t _label_pos_x    = 605;
static constexpr int16_t _label_pos_y    = 240;
static constexpr int16_t _btn_up_pos_x   = 499;
static constexpr int16_t _btn_up_pos_y   = 312;
static constexpr int16_t _btn_down_pos_x = 593;
static constexpr int16_t _btn_down_pos_y = 312;

static constexpr int16_t _midi_up   = 64 + 24;
static constexpr int16_t _midi_down = 60 + 24;
static constexpr int16_t _midi_top  = 64 + 8 + 24;
namespace {

static lv_obj_t* make_row(lv_obj_t* parent, const char* label_txt)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_style_pad_gap(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, label_txt);
    lv_obj_set_width(label, lv_pct(40));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    return row;
}

static void set_label_value(lv_obj_t* label, const char* fmt, float v)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), fmt, v);
    lv_label_set_text(label, buf);
}

class BoostTuneWindow : public ui::Window
{
public:
    explicit BoostTuneWindow(lv_disp_t* disp) : ui::Window(disp) {}

    void init() override
    {
        ui::Window::init("Audio tuning");

        lv_obj_t* root = _window->get();   // lvgl_cpp::Container -> lv_obj_t*
        lv_obj_set_style_pad_all(root, 16, 0);
        lv_obj_set_style_pad_gap(root, 14, 0);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        const auto settings = app::dsp::BoostSettingsStore::instance().get();

        // Enable
        {
            lv_obj_t* row = make_row(root, "Enable");
            lv_obj_t* sw = lv_switch_create(row);
            if (settings.enabled) lv_obj_add_state(sw, LV_STATE_CHECKED);

            lv_obj_add_event_cb(sw, [](lv_event_t* e) {
                bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
                app::dsp::BoostSettingsStore::instance().set_enabled(on);
            }, LV_EVENT_VALUE_CHANGED, nullptr);
        }

        // Mono mix
        {
            lv_obj_t* row = make_row(root, "Mono mix");
            lv_obj_t* sw = lv_switch_create(row);
            if (settings.mono_mix) lv_obj_add_state(sw, LV_STATE_CHECKED);

            lv_obj_add_event_cb(sw, [](lv_event_t* e) {
                bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
                app::dsp::BoostSettingsStore::instance().set_mono(on);
            }, LV_EVENT_VALUE_CHANGED, nullptr);
        }

        // Pre gain
        {
            lv_obj_t* row = make_row(root, "Pre gain (dB)");
            lv_obj_t* slider = lv_slider_create(row);
            lv_slider_set_range(slider, -24, 24);
            lv_slider_set_value(slider, (int)settings.pre_gain_db, LV_ANIM_OFF);
            lv_obj_set_width(slider, lv_pct(45));

            lv_obj_t* val = lv_label_create(row);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            set_label_value(val, "%.0f", (float)lv_slider_get_value(slider));

            lv_obj_add_event_cb(slider, [](lv_event_t* e) {
                auto* s = lv_event_get_target(e);
                float db = (float)lv_slider_get_value(s);
                app::dsp::BoostSettingsStore::instance().set_pre_gain_db(db);
                set_label_value((lv_obj_t*)lv_event_get_user_data(e), "%.0f", db);
            }, LV_EVENT_VALUE_CHANGED, val);
        }

        // HPF
        {
            lv_obj_t* row = make_row(root, "HPF (Hz)");
            lv_obj_t* slider = lv_slider_create(row);
            lv_slider_set_range(slider, 0, 500);
            lv_slider_set_value(slider, (int)settings.hpf_hz, LV_ANIM_OFF);
            lv_obj_set_width(slider, lv_pct(45));

            lv_obj_t* val = lv_label_create(row);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            set_label_value(val, "%.0f", (float)lv_slider_get_value(slider));

            lv_obj_add_event_cb(slider, [](lv_event_t* e) {
                auto* s = lv_event_get_target(e);
                float hz = (float)lv_slider_get_value(s);
                app::dsp::BoostSettingsStore::instance().set_hpf_hz(hz);
                set_label_value((lv_obj_t*)lv_event_get_user_data(e), "%.0f", hz);
            }, LV_EVENT_VALUE_CHANGED, val);
        }

        // LPF
        {
            lv_obj_t* row = make_row(root, "LPF (Hz)");
            lv_obj_t* slider = lv_slider_create(row);
            lv_slider_set_range(slider, 1000, 8000);
            lv_slider_set_value(slider, (int)settings.lpf_hz, LV_ANIM_OFF);
            lv_obj_set_width(slider, lv_pct(45));

            lv_obj_t* val = lv_label_create(row);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            set_label_value(val, "%.0f", (float)lv_slider_get_value(slider));

            lv_obj_add_event_cb(slider, [](lv_event_t* e) {
                auto* s = lv_event_get_target(e);
                float hz = (float)lv_slider_get_value(s);
                app::dsp::BoostSettingsStore::instance().set_lpf_hz(hz);
                set_label_value((lv_obj_t*)lv_event_get_user_data(e), "%.0f", hz);
            }, LV_EVENT_VALUE_CHANGED, val);
        }

        // Noise reduction
        {
            lv_obj_t* row = make_row(root, "Noise reduct.");
            lv_obj_t* slider = lv_slider_create(row);
            lv_slider_set_range(slider, 0, 100);
            lv_slider_set_value(slider, (int)(settings.noise_reduction * 100.0f), LV_ANIM_OFF);
            lv_obj_set_width(slider, lv_pct(45));

            lv_obj_t* val = lv_label_create(row);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            set_label_value(val, "%.0f%%", (float)lv_slider_get_value(slider));

            lv_obj_add_event_cb(slider, [](lv_event_t* e) {
                auto* s = lv_event_get_target(e);
                float v01 = (float)lv_slider_get_value(s) / 100.0f;
                app::dsp::BoostSettingsStore::instance().set_noise_reduction(v01);
                set_label_value((lv_obj_t*)lv_event_get_user_data(e), "%.0f%%", v01 * 100.0f);
            }, LV_EVENT_VALUE_CHANGED, val);
        }

        // Speech boost
        {
            lv_obj_t* row = make_row(root, "Speech boost");
            lv_obj_t* slider = lv_slider_create(row);
            lv_slider_set_range(slider, 0, 100);
            lv_slider_set_value(slider, (int)(settings.speech_boost * 100.0f), LV_ANIM_OFF);
            lv_obj_set_width(slider, lv_pct(45));

            lv_obj_t* val = lv_label_create(row);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            set_label_value(val, "%.0f%%", (float)lv_slider_get_value(slider));

            lv_obj_add_event_cb(slider, [](lv_event_t* e) {
                auto* s = lv_event_get_target(e);
                float v01 = (float)lv_slider_get_value(s) / 100.0f;
                app::dsp::BoostSettingsStore::instance().set_speech_boost(v01);
                set_label_value((lv_obj_t*)lv_event_get_user_data(e), "%.0f%%", v01 * 100.0f);
            }, LV_EVENT_VALUE_CHANGED, val);
        }
    }
};

static std::unique_ptr<BoostTuneWindow> s_boost_window;

static void open_boost_tune_window()
{
    if (!s_boost_window) {
        s_boost_window = std::make_unique<BoostTuneWindow>(GetHAL()->getDisplay());
        s_boost_window->init();
    }
    s_boost_window->open();
}

} // namespace



void PanelSpeakerVolume::init()
{
    _label_volume = std::make_unique<Label>(lv_screen_active());
    _label_volume->align(LV_ALIGN_CENTER, _label_pos_x, _label_pos_y);
    _label_volume->setTextFont(&lv_font_montserrat_24);
    // Long-press the volume number to open the granular audio tuning UI
    _label_volume->addEventCb(LV_EVENT_LONG_PRESSED, [](lv_event_t*) {
        open_boost_tune_window();
    });
    _label_volume->setTextColor(lv_color_hex(0xFEFEFE));
    _label_volume->setText(fmt::format("{}", GetHAL()->getSpeakerVolume()));

    _btn_up = std::make_unique<Container>(lv_screen_active());
    _btn_up->align(LV_ALIGN_CENTER, _btn_up_pos_x, _btn_up_pos_y);
    _btn_up->setSize(81, 85);
    _btn_up->setOpa(0);
    _btn_up->onClick().connect([&]() {
        // Animation
        _label_y_anim.teleport(_label_pos_y - 8);
        _label_y_anim = _label_pos_y;

        // SFX
        if (GetHAL()->getSpeakerVolume() >= 100) {
            audio::play_tone_from_midi(_midi_top);
            return;
        }

        // Update volume
        int target = GetHAL()->getSpeakerVolume();
        target     = std::clamp(target + 20, 0, 100);
        GetHAL()->setSpeakerVolume(target);
        _label_volume->setText(fmt::format("{}", GetHAL()->getSpeakerVolume()));

        audio::play_tone_from_midi(_midi_up);
    });

    _btn_down = std::make_unique<Container>(lv_screen_active());
    _btn_down->align(LV_ALIGN_CENTER, _btn_down_pos_x, _btn_down_pos_y);
    _btn_down->setSize(81, 85);
    _btn_down->setOpa(0);
    _btn_down->onClick().connect([&]() {
        // Animation
        _label_y_anim.teleport(_label_pos_y + 8);
        _label_y_anim = _label_pos_y;

        // SFX
        if (GetHAL()->getSpeakerVolume() <= 0) {
            return;
        }
        audio::play_tone_from_midi(_midi_down);

        // Update volume
        int target = GetHAL()->getSpeakerVolume();
        target     = std::clamp(target - 20, 0, 100);
        GetHAL()->setSpeakerVolume(target);
        _label_volume->setText(fmt::format("{}", GetHAL()->getSpeakerVolume()));
    });

    _label_y_anim.easingOptions().duration = 0.2;
    _label_y_anim.teleport(_label_pos_y);
}

void PanelSpeakerVolume::update(bool isStacked)
{
    if (!_label_y_anim.done()) {
        _label_volume->setY(_label_y_anim);
    }
}
