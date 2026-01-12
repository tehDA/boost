#include "view.h"

#include <apps/utils/dsp/boost_settings.h>

#include <esp_log.h>
#include <hal/hal.h>
#include <lvgl.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>

#include <algorithm>
#include <memory>
#include <string>

using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

namespace launcher_view {

static const char* TAG = "boost_tune";

static constexpr int16_t kLabelPosX          = 591;
static constexpr int16_t kBtnUpPosX          = 499;
static constexpr int16_t kBtnDownPosX        = 593;
static constexpr int16_t kLabelToButtonDelta = 72;
static constexpr int16_t kBtnUpPosY          = 312;
static constexpr int16_t kBtnDownPosY        = 312;
static constexpr int16_t kLabelPosY          = kBtnDownPosY - kLabelToButtonDelta;
static constexpr int16_t kLabelHitAreaWidth  = 180;
static constexpr int16_t kLabelHitAreaHeight = 90;

namespace {

static void apply_slider_style(lv_obj_t* slider)
{
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x142737), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x59D9F7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_90, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xC7F4FF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
}

static void update_label_pct(lv_obj_t* label, const char* fmt, int value)
{
    if (label != nullptr) {
        lv_label_set_text_fmt(label, fmt, value);
    }
}

static void on_mono_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "mono mix: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_mono(enabled);
}

static void on_hpf_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int hz = std::clamp((int)lv_slider_get_value(obj), 20, 800);
    update_label_pct(label, "HPF cutoff: %d Hz", hz);
    ESP_LOGI(TAG, "HPF cutoff: %d Hz", hz);
    app::dsp::BoostSettingsStore::instance().set_hpf_hz(static_cast<float>(hz));
}

static void on_lpf_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int hz = std::clamp((int)lv_slider_get_value(obj), 1000, 20000);
    update_label_pct(label, "LPF cutoff: %d Hz", hz);
    ESP_LOGI(TAG, "LPF cutoff: %d Hz", hz);
    app::dsp::BoostSettingsStore::instance().set_lpf_hz(static_cast<float>(hz));
}

static void on_noise_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), 0, 100);
    update_label_pct(label, "Noise reduction: %d%%", v);
    ESP_LOGI(TAG, "noise reduction: %d%%", v);
    app::dsp::BoostSettingsStore::instance().set_noise_reduction(static_cast<float>(v) / 100.0f);
}

static void on_speech_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), 0, 100);
    update_label_pct(label, "Speech boost: %d%%", v);
    ESP_LOGI(TAG, "speech boost: %d%%", v);
    app::dsp::BoostSettingsStore::instance().set_speech_boost(static_cast<float>(v) / 100.0f);
}

} // namespace

static std::string get_vol_str()
{
    return std::to_string(GetHAL()->getSpeakerVolume()) + "%";
}

static void update_vol_str(Label* label_vol)
{
    if (label_vol) {
        label_vol->setText(get_vol_str());
    }
}

static void update_vol(int offset, Label* label_vol)
{
    int vol = GetHAL()->getSpeakerVolume();

    vol += offset;

    vol = std::max(vol, 0);
    vol = std::min(vol, 100);

    GetHAL()->setSpeakerVolume(vol);

    update_vol_str(label_vol);
}

void PanelSpeakerVolume::init()
{
    auto settings = app::dsp::BoostSettingsStore::instance().get();

    _label_volume_container = std::make_unique<Container>(lv_screen_active());
    _label_volume_container->align(LV_ALIGN_CENTER, kLabelPosX, kLabelPosY);
    _label_volume_container->setSize(kLabelHitAreaWidth, kLabelHitAreaHeight);
    lv_obj_set_style_bg_opa(_label_volume_container->get(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_label_volume_container->get(), 0, 0);

    _label_volume = std::make_unique<Label>(_label_volume_container->get());
    _label_volume->align(LV_ALIGN_CENTER, 0, 0);
    _label_volume->setTextFont(&lv_font_montserrat_24);
    _label_volume->setTextColor(lv_color_hex(0xFEFEFE));
    _label_volume->setText(get_vol_str());

    _btn_up = std::make_unique<Container>(lv_screen_active());
    _btn_up->align(LV_ALIGN_CENTER, kBtnUpPosX, kBtnUpPosY);
    _btn_up->setSize(81, 85);
    _btn_up->setOpa(0);
    _btn_up->onClick().connect([&]() {
        _label_y_anim.teleport(kLabelPosY - 8);
        _label_y_anim = kLabelPosY;

        update_vol(10, _label_volume.get());
    });

    _btn_down = std::make_unique<Container>(lv_screen_active());
    _btn_down->align(LV_ALIGN_CENTER, kBtnDownPosX, kBtnDownPosY);
    _btn_down->setSize(81, 85);
    _btn_down->setOpa(0);
    _btn_down->onClick().connect([&]() {
        _label_y_anim.teleport(kLabelPosY + 8);
        _label_y_anim = kLabelPosY;

        update_vol(-10, _label_volume.get());
    });

    _panel_tuning = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_panel_tuning, 320, 600);
    lv_obj_set_pos(_panel_tuning, 1320, 80);
    lv_obj_set_style_bg_color(_panel_tuning, lv_color_hex(0x0C1A26), 0);
    lv_obj_set_style_bg_opa(_panel_tuning, LV_OPA_90, 0);
    lv_obj_set_style_border_color(_panel_tuning, lv_color_hex(0x2E5166), 0);
    lv_obj_set_style_border_width(_panel_tuning, 2, 0);
    lv_obj_set_style_text_color(_panel_tuning, lv_color_hex(0xD6F7FF), LV_PART_MAIN);
    lv_obj_set_style_pad_all(_panel_tuning, 12, 0);
    lv_obj_set_style_pad_row(_panel_tuning, 10, 0);
    lv_obj_set_flex_flow(_panel_tuning, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(_panel_tuning, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_panel_tuning);
    lv_label_set_text(title, "AUDIO TUNING");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8FE9FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    _cb_mono = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(_cb_mono, "Mono mix");
    if (settings.mono_mix) {
        lv_obj_add_state(_cb_mono, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(_cb_mono, &on_mono_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    _label_hpf = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(_label_hpf, "HPF cutoff: %d Hz", (int)settings.hpf_hz);
    _sl_hpf = lv_slider_create(_panel_tuning);
    lv_slider_set_range(_sl_hpf, 20, 800);
    lv_slider_set_value(_sl_hpf, (int)settings.hpf_hz, LV_ANIM_OFF);
    lv_obj_set_width(_sl_hpf, LV_PCT(100));
    lv_obj_add_event_cb(_sl_hpf, &on_hpf_changed, LV_EVENT_VALUE_CHANGED, _label_hpf);
    apply_slider_style(_sl_hpf);

    _label_lpf = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(_label_lpf, "LPF cutoff: %d Hz", (int)settings.lpf_hz);
    _sl_lpf = lv_slider_create(_panel_tuning);
    lv_slider_set_range(_sl_lpf, 1000, 20000);
    lv_slider_set_value(_sl_lpf, (int)settings.lpf_hz, LV_ANIM_OFF);
    lv_obj_set_width(_sl_lpf, LV_PCT(100));
    lv_obj_add_event_cb(_sl_lpf, &on_lpf_changed, LV_EVENT_VALUE_CHANGED, _label_lpf);
    apply_slider_style(_sl_lpf);

    _label_noise = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(_label_noise, "Noise reduction: %d%%", (int)(settings.noise_reduction * 100.0f));
    _sl_noise = lv_slider_create(_panel_tuning);
    lv_slider_set_range(_sl_noise, 0, 100);
    lv_slider_set_value(_sl_noise, (int)(settings.noise_reduction * 100.0f), LV_ANIM_OFF);
    lv_obj_set_width(_sl_noise, LV_PCT(100));
    lv_obj_add_event_cb(_sl_noise, &on_noise_changed, LV_EVENT_VALUE_CHANGED, _label_noise);
    apply_slider_style(_sl_noise);

    _label_speech = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(_label_speech, "Speech boost: %d%%", (int)(settings.speech_boost * 100.0f));
    _sl_speech = lv_slider_create(_panel_tuning);
    lv_slider_set_range(_sl_speech, 0, 100);
    lv_slider_set_value(_sl_speech, (int)(settings.speech_boost * 100.0f), LV_ANIM_OFF);
    lv_obj_set_width(_sl_speech, LV_PCT(100));
    lv_obj_add_event_cb(_sl_speech, &on_speech_changed, LV_EVENT_VALUE_CHANGED, _label_speech);
    apply_slider_style(_sl_speech);

    _label_y_anim.easingOptions().duration = 0.2;
    _label_y_anim.teleport(kLabelPosY);
}

void PanelSpeakerVolume::update(bool isStacked)
{
    if (_label_volume) {
        update_vol_str(_label_volume.get());
    }

    if (!_label_y_anim.done()) {
        if (_label_volume_container) {
            _label_volume_container->setY(_label_y_anim);
        }
    }
}

void PanelSpeakerVolume::openBoostTune(lv_obj_t* parent)
{
    if (!parent) {
        parent = lv_screen_active();
    }
    lv_obj_scroll_to_x(parent, 1200, LV_ANIM_ON);
}

} // namespace launcher_view
