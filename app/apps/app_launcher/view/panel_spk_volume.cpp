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
static constexpr int16_t kLabelTagOffsetY    = -52;

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

static void update_label_db(lv_obj_t* label, const char* fmt, int value)
{
    if (label != nullptr) {
        lv_label_set_text_fmt(label, fmt, value);
    }
}

static void on_enabled_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "audio path: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_enabled(enabled);
}

static void on_near_field_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "near field: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_near_field(enabled);
}

static void on_beamform_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "beamforming: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_beamform_enable(enabled);
}

static void on_beam_width_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), 0, 100);
    update_label_pct(label, "Beam width: %d%%", v);
    ESP_LOGI(TAG, "beam width: %d%%", v);
    app::dsp::BoostSettingsStore::instance().set_beam_width(static_cast<float>(v) / 100.0f);
}

static void on_pre_gain_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), -12, 12);
    update_label_db(label, "Pre gain: %+d dB", v);
    ESP_LOGI(TAG, "pre gain: %d dB", v);
    app::dsp::BoostSettingsStore::instance().set_pre_gain_db(static_cast<float>(v));
}

static void on_post_gain_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), -12, 12);
    update_label_db(label, "Post gain: %+d dB", v);
    ESP_LOGI(TAG, "post gain: %d dB", v);
    app::dsp::BoostSettingsStore::instance().set_post_gain_db(static_cast<float>(v));
}

static void on_dereverb_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), 0, 100);
    update_label_pct(label, "Dereverb: %d%%", v);
    ESP_LOGI(TAG, "dereverb: %d%%", v);
    app::dsp::BoostSettingsStore::instance().set_dereverb(static_cast<float>(v) / 100.0f);
}

static void on_eq_low_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), -12, 12);
    update_label_db(label, "EQ bass: %+d dB", v);
    ESP_LOGI(TAG, "eq bass: %d dB", v);
    app::dsp::BoostSettingsStore::instance().set_eq_low_db(static_cast<float>(v));
}

static void on_eq_mid_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), -12, 12);
    update_label_db(label, "EQ mid: %+d dB", v);
    ESP_LOGI(TAG, "eq mid: %d dB", v);
    app::dsp::BoostSettingsStore::instance().set_eq_mid_db(static_cast<float>(v));
}

static void on_eq_high_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    int v = std::clamp((int)lv_slider_get_value(obj), -12, 12);
    update_label_db(label, "EQ treble: %+d dB", v);
    ESP_LOGI(TAG, "eq treble: %d dB", v);
    app::dsp::BoostSettingsStore::instance().set_eq_high_db(static_cast<float>(v));
}

static void on_aes_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "aes: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_aes_enable(enabled);
}

static void on_headset_mic_only_changed(lv_event_t* e)
{
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "headset mic only: %s", enabled ? "ON" : "OFF");
    app::dsp::BoostSettingsStore::instance().set_headset_mic_only(enabled);
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

static void stylize_trek_tag(lv_obj_t* obj, const char* text, lv_color_t accent)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0E2333), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1E465C), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_90, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, accent, 0);
    lv_obj_set_style_radius(obj, 14, 0);

    lv_obj_t* accent_bar = lv_obj_create(obj);
    lv_obj_set_size(accent_bar, 48, 6);
    lv_obj_align(accent_bar, LV_ALIGN_TOP_LEFT, 12, 6);
    lv_obj_set_style_bg_color(accent_bar, accent, 0);
    lv_obj_set_style_bg_opa(accent_bar, LV_OPA_90, 0);
    lv_obj_set_style_radius(accent_bar, 3, 0);
    lv_obj_set_style_border_width(accent_bar, 0, 0);

    lv_obj_t* label = lv_label_create(obj);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, accent, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 70, 2);
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

    auto* volume_tag = lv_obj_create(lv_screen_active());
    lv_obj_set_size(volume_tag, 160, 40);
    lv_obj_align(volume_tag, LV_ALIGN_CENTER, kLabelPosX, kLabelPosY + kLabelTagOffsetY);
    stylize_trek_tag(volume_tag, "AUDIO VOLUME", lv_color_hex(0x75F0FF));

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
    stylize_trek_button(_btn_up->get(), "+", lv_color_hex(0x65DDFD));
    _btn_up->onClick().connect([&]() {
        _label_y_anim.teleport(kLabelPosY - 8);
        _label_y_anim = kLabelPosY;

        update_vol(10, _label_volume.get());
    });

    _btn_down = std::make_unique<Container>(lv_screen_active());
    _btn_down->align(LV_ALIGN_CENTER, kBtnDownPosX, kBtnDownPosY);
    _btn_down->setSize(81, 85);
    stylize_trek_button(_btn_down->get(), "-", lv_color_hex(0x5BDAC9));
    _btn_down->onClick().connect([&]() {
        _label_y_anim.teleport(kLabelPosY + 8);
        _label_y_anim = kLabelPosY;

        update_vol(-10, _label_volume.get());
    });

    _panel_tuning = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_panel_tuning, 300, 360);
    lv_obj_align(_panel_tuning, LV_ALIGN_TOP_RIGHT, -24, 70);
    lv_obj_set_style_bg_color(_panel_tuning, lv_color_hex(0x0C1A26), 0);
    lv_obj_set_style_bg_opa(_panel_tuning, LV_OPA_90, 0);
    lv_obj_set_style_border_color(_panel_tuning, lv_color_hex(0x2E5166), 0);
    lv_obj_set_style_border_width(_panel_tuning, 2, 0);
    lv_obj_set_style_text_color(_panel_tuning, lv_color_hex(0xD6F7FF), LV_PART_MAIN);
    lv_obj_set_style_pad_all(_panel_tuning, 12, 0);
    lv_obj_set_style_pad_row(_panel_tuning, 10, 0);
    lv_obj_set_flex_flow(_panel_tuning, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(_panel_tuning, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t* title = lv_label_create(_panel_tuning);
    lv_label_set_text(title, "AUDIO TUNING");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8FE9FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    lv_obj_t* cb_enabled = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(cb_enabled, "Audio path enabled");
    if (settings.enabled) {
        lv_obj_add_state(cb_enabled, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(cb_enabled, &on_enabled_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* cb_near_field = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(cb_near_field, "Near-field tuning");
    if (settings.near_field) {
        lv_obj_add_state(cb_near_field, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(cb_near_field, &on_near_field_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    _cb_mono = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(_cb_mono, "Mono mix");
    if (settings.mono_mix) {
        lv_obj_add_state(_cb_mono, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(_cb_mono, &on_mono_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* cb_headset_only = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(cb_headset_only, "Headset mic only");
    if (settings.headset_mic_only) {
        lv_obj_add_state(cb_headset_only, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(cb_headset_only, &on_headset_mic_only_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* cb_aes = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(cb_aes, "AES processing");
    if (settings.aes_enable) {
        lv_obj_add_state(cb_aes, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(cb_aes, &on_aes_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* cb_beamform = lv_checkbox_create(_panel_tuning);
    lv_checkbox_set_text(cb_beamform, "Beamforming");
    if (settings.beamform_enable) {
        lv_obj_add_state(cb_beamform, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(cb_beamform, &on_beamform_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* label_beam_width = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_beam_width, "Beam width: %d%%", (int)(settings.beam_width * 100.0f));
    lv_obj_t* sl_beam_width = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_beam_width, 0, 100);
    lv_slider_set_value(sl_beam_width, (int)(settings.beam_width * 100.0f), LV_ANIM_OFF);
    lv_obj_set_width(sl_beam_width, LV_PCT(100));
    lv_obj_add_event_cb(sl_beam_width, &on_beam_width_changed, LV_EVENT_VALUE_CHANGED, label_beam_width);
    apply_slider_style(sl_beam_width);

    lv_obj_t* label_pre_gain = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_pre_gain, "Pre gain: %+d dB", (int)settings.pre_gain_db);
    lv_obj_t* sl_pre_gain = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_pre_gain, -12, 12);
    lv_slider_set_value(sl_pre_gain, (int)settings.pre_gain_db, LV_ANIM_OFF);
    lv_obj_set_width(sl_pre_gain, LV_PCT(100));
    lv_obj_add_event_cb(sl_pre_gain, &on_pre_gain_changed, LV_EVENT_VALUE_CHANGED, label_pre_gain);
    apply_slider_style(sl_pre_gain);

    lv_obj_t* label_post_gain = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_post_gain, "Post gain: %+d dB", (int)settings.post_gain_db);
    lv_obj_t* sl_post_gain = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_post_gain, -12, 12);
    lv_slider_set_value(sl_post_gain, (int)settings.post_gain_db, LV_ANIM_OFF);
    lv_obj_set_width(sl_post_gain, LV_PCT(100));
    lv_obj_add_event_cb(sl_post_gain, &on_post_gain_changed, LV_EVENT_VALUE_CHANGED, label_post_gain);
    apply_slider_style(sl_post_gain);

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

    lv_obj_t* label_dereverb = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_dereverb, "Dereverb: %d%%", (int)(settings.dereverb * 100.0f));
    lv_obj_t* sl_dereverb = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_dereverb, 0, 100);
    lv_slider_set_value(sl_dereverb, (int)(settings.dereverb * 100.0f), LV_ANIM_OFF);
    lv_obj_set_width(sl_dereverb, LV_PCT(100));
    lv_obj_add_event_cb(sl_dereverb, &on_dereverb_changed, LV_EVENT_VALUE_CHANGED, label_dereverb);
    apply_slider_style(sl_dereverb);

    lv_obj_t* label_eq_low = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_eq_low, "EQ bass: %+d dB", (int)settings.eq_low_db);
    lv_obj_t* sl_eq_low = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_eq_low, -12, 12);
    lv_slider_set_value(sl_eq_low, (int)settings.eq_low_db, LV_ANIM_OFF);
    lv_obj_set_width(sl_eq_low, LV_PCT(100));
    lv_obj_add_event_cb(sl_eq_low, &on_eq_low_changed, LV_EVENT_VALUE_CHANGED, label_eq_low);
    apply_slider_style(sl_eq_low);

    lv_obj_t* label_eq_mid = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_eq_mid, "EQ mid: %+d dB", (int)settings.eq_mid_db);
    lv_obj_t* sl_eq_mid = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_eq_mid, -12, 12);
    lv_slider_set_value(sl_eq_mid, (int)settings.eq_mid_db, LV_ANIM_OFF);
    lv_obj_set_width(sl_eq_mid, LV_PCT(100));
    lv_obj_add_event_cb(sl_eq_mid, &on_eq_mid_changed, LV_EVENT_VALUE_CHANGED, label_eq_mid);
    apply_slider_style(sl_eq_mid);

    lv_obj_t* label_eq_high = lv_label_create(_panel_tuning);
    lv_label_set_text_fmt(label_eq_high, "EQ treble: %+d dB", (int)settings.eq_high_db);
    lv_obj_t* sl_eq_high = lv_slider_create(_panel_tuning);
    lv_slider_set_range(sl_eq_high, -12, 12);
    lv_slider_set_value(sl_eq_high, (int)settings.eq_high_db, LV_ANIM_OFF);
    lv_obj_set_width(sl_eq_high, LV_PCT(100));
    lv_obj_add_event_cb(sl_eq_high, &on_eq_high_changed, LV_EVENT_VALUE_CHANGED, label_eq_high);
    apply_slider_style(sl_eq_high);

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
    (void)parent;
}

} // namespace launcher_view
