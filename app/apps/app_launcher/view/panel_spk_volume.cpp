#include "view.h"

#include <apps/utils/ui/window.h>
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

/**
 * This window is a *UI placeholder* for future real-time audio tuning.
 *
 * Goal (future): make EQ, HPF/LPF, mono/stereo mix, beamforming, near/far field, etc.
 * adjustable on-the-fly without reworking the UI plumbing.
 *
 * For now, this just exposes a few controls and logs changes.
 */
class BoostTuneWindow : public ui::Window
{
public:
    BoostTuneWindow()
    {
        config.title = "Audio tuning";

        // Conservative defaults that fit TAB5 portrait.
        config.kfClosed = { .x = 10, .y = 30, .w = 0,   .h = 0,   .opa = 0   };
        config.kfOpened = { .x = 10, .y = 30, .w = 340, .h = 520, .opa = 255 };
    }

protected:
    void onOpen() override
    {
        lv_obj_clean(_window->get());

        lv_obj_t* root = _window->get();
        lv_obj_set_style_bg_color(root, lv_color_hex(0x0C1F2E), 0);
        lv_obj_set_style_bg_opa(root, LV_OPA_90, 0);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_all(root, 14, 0);
        lv_obj_set_style_pad_row(root, 10, 0);

        auto settings = app::dsp::BoostSettingsStore::instance().get();
        _state.mono = settings.mono_mix;
        _state.spk_vol_pct = GetHAL()->getSpeakerVolume();
        _state.hpf_hz = static_cast<int>(settings.hpf_hz);
        _state.lpf_hz = static_cast<int>(settings.lpf_hz);
        _state.noise_reduction = static_cast<int>(settings.noise_reduction * 100.0f);
        _state.speech_boost = static_cast<int>(settings.speech_boost * 100.0f);

        // Header
        lv_obj_t* hint = lv_label_create(root);
        lv_label_set_text(hint, "LCARS AUDIO TUNING");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x89E6FF), 0);
        lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);

        // Mono mix toggle
        _cb_mono = lv_checkbox_create(root);
        lv_checkbox_set_text(_cb_mono, "Mono mix output");
        if (_state.mono) {
            lv_obj_add_state(_cb_mono, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(_cb_mono, &BoostTuneWindow::on_mono_changed, LV_EVENT_VALUE_CHANGED, this);

        // Speaker volume (0-100)
        lv_obj_t* row_vol = lv_obj_create(root);
        lv_obj_set_width(row_vol, LV_PCT(100));
        lv_obj_set_height(row_vol, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row_vol, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(row_vol, 0, 0);
        lv_obj_clear_flag(row_vol, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_vol = lv_label_create(row_vol);
        lv_label_set_text(lbl_vol, "Speaker volume (%)");

        _sl_vol = lv_slider_create(row_vol);
        lv_slider_set_range(_sl_vol, 0, 100);
        lv_slider_set_value(_sl_vol, _state.spk_vol_pct, LV_ANIM_OFF);
        lv_obj_set_width(_sl_vol, LV_PCT(100));
        lv_obj_add_event_cb(_sl_vol, &BoostTuneWindow::on_vol_changed, LV_EVENT_VALUE_CHANGED, this);
        apply_slider_style(_sl_vol);

        // HPF cutoff
        lv_obj_t* row_hpf = lv_obj_create(root);
        lv_obj_set_width(row_hpf, LV_PCT(100));
        lv_obj_set_height(row_hpf, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row_hpf, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(row_hpf, 0, 0);
        lv_obj_clear_flag(row_hpf, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_hpf = lv_label_create(row_hpf);
        lv_label_set_text_fmt(lbl_hpf, "HPF cutoff (Hz): %d", _state.hpf_hz);

        _sl_hpf = lv_slider_create(row_hpf);
        lv_slider_set_range(_sl_hpf, 20, 800);
        lv_slider_set_value(_sl_hpf, _state.hpf_hz, LV_ANIM_OFF);
        lv_obj_set_width(_sl_hpf, LV_PCT(100));
        lv_obj_add_event_cb(_sl_hpf, &BoostTuneWindow::on_hpf_changed, LV_EVENT_VALUE_CHANGED, this);
        apply_slider_style(_sl_hpf);

        // LPF cutoff
        lv_obj_t* row_lpf = lv_obj_create(root);
        lv_obj_set_width(row_lpf, LV_PCT(100));
        lv_obj_set_height(row_lpf, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row_lpf, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(row_lpf, 0, 0);
        lv_obj_clear_flag(row_lpf, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_lpf = lv_label_create(row_lpf);
        lv_label_set_text_fmt(lbl_lpf, "LPF cutoff (Hz): %d", _state.lpf_hz);

        _sl_lpf = lv_slider_create(row_lpf);
        lv_slider_set_range(_sl_lpf, 1000, 20000);
        lv_slider_set_value(_sl_lpf, _state.lpf_hz, LV_ANIM_OFF);
        lv_obj_set_width(_sl_lpf, LV_PCT(100));
        lv_obj_add_event_cb(_sl_lpf, &BoostTuneWindow::on_lpf_changed, LV_EVENT_VALUE_CHANGED, this);
        apply_slider_style(_sl_lpf);

        // Noise reduction
        lv_obj_t* row_nr = lv_obj_create(root);
        lv_obj_set_width(row_nr, LV_PCT(100));
        lv_obj_set_height(row_nr, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row_nr, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(row_nr, 0, 0);
        lv_obj_clear_flag(row_nr, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_nr = lv_label_create(row_nr);
        lv_label_set_text_fmt(lbl_nr, "Noise reduction: %d%%", _state.noise_reduction);

        _sl_noise = lv_slider_create(row_nr);
        lv_slider_set_range(_sl_noise, 0, 100);
        lv_slider_set_value(_sl_noise, _state.noise_reduction, LV_ANIM_OFF);
        lv_obj_set_width(_sl_noise, LV_PCT(100));
        lv_obj_add_event_cb(_sl_noise, &BoostTuneWindow::on_noise_changed, LV_EVENT_VALUE_CHANGED, this);
        apply_slider_style(_sl_noise);

        // Speech boost
        lv_obj_t* row_sb = lv_obj_create(root);
        lv_obj_set_width(row_sb, LV_PCT(100));
        lv_obj_set_height(row_sb, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row_sb, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(row_sb, 0, 0);
        lv_obj_clear_flag(row_sb, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_sb = lv_label_create(row_sb);
        lv_label_set_text_fmt(lbl_sb, "Speech boost: %d%%", _state.speech_boost);

        _sl_speech = lv_slider_create(row_sb);
        lv_slider_set_range(_sl_speech, 0, 100);
        lv_slider_set_value(_sl_speech, _state.speech_boost, LV_ANIM_OFF);
        lv_obj_set_width(_sl_speech, LV_PCT(100));
        lv_obj_add_event_cb(_sl_speech, &BoostTuneWindow::on_speech_changed, LV_EVENT_VALUE_CHANGED, this);
        apply_slider_style(_sl_speech);

        // Close button
        lv_obj_t* btn_close = lv_btn_create(root);
        lv_obj_set_width(btn_close, LV_PCT(100));
        lv_obj_t* btn_lbl = lv_label_create(btn_close);
        lv_label_set_text(btn_lbl, "Close");
        lv_obj_center(btn_lbl);

        lv_obj_add_event_cb(btn_close, &BoostTuneWindow::on_close_pressed, LV_EVENT_CLICKED, this);

        // Animated scanline for sci-fi vibe
        lv_obj_t* scanline = lv_obj_create(root);
        lv_obj_set_size(scanline, LV_PCT(100), 3);
        lv_obj_set_style_bg_color(scanline, lv_color_hex(0x33D6FF), 0);
        lv_obj_set_style_bg_opa(scanline, LV_OPA_60, 0);
        lv_obj_set_style_border_width(scanline, 0, 0);
        lv_obj_clear_flag(scanline, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(scanline, LV_OBJ_FLAG_FLOATING);
        lv_obj_move_background(scanline);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, scanline);
        lv_anim_set_values(&anim, 24, 470);
        lv_anim_set_time(&anim, 2400);
        lv_anim_set_playback_time(&anim, 2400);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
            lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
        });
        lv_anim_start(&anim);
    }

private:
    struct State
    {
        bool mono = false;
        int spk_vol_pct = 60;
        int hpf_hz = 100;
        int lpf_hz = 12000;
        int noise_reduction = 35;
        int speech_boost = 35;
    };

    static void apply_slider_style(lv_obj_t* slider)
    {
        lv_obj_set_style_bg_color(slider, lv_color_hex(0x18344A), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slider, LV_OPA_60, LV_PART_MAIN);
        lv_obj_set_style_bg_color(slider, lv_color_hex(0x56D6FF), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(slider, LV_OPA_90, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(slider, lv_color_hex(0xA5F1FF), LV_PART_KNOB);
        lv_obj_set_style_bg_opa(slider, LV_OPA_100, LV_PART_KNOB);
    }

    static void on_close_pressed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self != nullptr) {
            self->close();
        }
    }

    static void on_mono_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        self->_state.mono = lv_obj_has_state(obj, LV_STATE_CHECKED);

        ESP_LOGI(TAG, "mono mix: %s", self->_state.mono ? "ON" : "OFF");
        app::dsp::BoostSettingsStore::instance().set_mono(self->_state.mono);
    }

    static void on_vol_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        int v = (int)lv_slider_get_value(obj);
        v = std::clamp(v, 0, 100);
        self->_state.spk_vol_pct = v;

        ESP_LOGI(TAG, "speaker volume: %d%%", v);
        GetHAL()->setSpeakerVolume((uint8_t)v);
    }

    static void on_hpf_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        int hz = (int)lv_slider_get_value(obj);
        hz = std::clamp(hz, 20, 800);
        self->_state.hpf_hz = hz;

        // Update the label above (parent's first child).
        lv_obj_t* parent = lv_obj_get_parent(obj);
        lv_obj_t* lbl = lv_obj_get_child(parent, 0);
        lv_label_set_text_fmt(lbl, "HPF cutoff (Hz): %d", hz);

        ESP_LOGI(TAG, "HPF cutoff: %d Hz", hz);
        app::dsp::BoostSettingsStore::instance().set_hpf_hz(static_cast<float>(hz));
    }

    static void on_lpf_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        int hz = (int)lv_slider_get_value(obj);
        hz = std::clamp(hz, 1000, 20000);
        self->_state.lpf_hz = hz;

        lv_obj_t* parent = lv_obj_get_parent(obj);
        lv_obj_t* lbl = lv_obj_get_child(parent, 0);
        lv_label_set_text_fmt(lbl, "LPF cutoff (Hz): %d", hz);

        ESP_LOGI(TAG, "LPF cutoff: %d Hz", hz);
        app::dsp::BoostSettingsStore::instance().set_lpf_hz(static_cast<float>(hz));
    }

    static void on_noise_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        int v = (int)lv_slider_get_value(obj);
        v = std::clamp(v, 0, 100);
        self->_state.noise_reduction = v;

        lv_obj_t* parent = lv_obj_get_parent(obj);
        lv_obj_t* lbl = lv_obj_get_child(parent, 0);
        lv_label_set_text_fmt(lbl, "Noise reduction: %d%%", v);

        ESP_LOGI(TAG, "noise reduction: %d%%", v);
        app::dsp::BoostSettingsStore::instance().set_noise_reduction(static_cast<float>(v) / 100.0f);
    }

    static void on_speech_changed(lv_event_t* e)
    {
        auto* self = static_cast<BoostTuneWindow*>(lv_event_get_user_data(e));
        if (self == nullptr) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        int v = (int)lv_slider_get_value(obj);
        v = std::clamp(v, 0, 100);
        self->_state.speech_boost = v;

        lv_obj_t* parent = lv_obj_get_parent(obj);
        lv_obj_t* lbl = lv_obj_get_child(parent, 0);
        lv_label_set_text_fmt(lbl, "Speech boost: %d%%", v);

        ESP_LOGI(TAG, "speech boost: %d%%", v);
        app::dsp::BoostSettingsStore::instance().set_speech_boost(static_cast<float>(v) / 100.0f);
    }

private:
    State _state{};

    lv_obj_t* _cb_mono = nullptr;
    lv_obj_t* _sl_vol  = nullptr;
    lv_obj_t* _sl_hpf  = nullptr;
    lv_obj_t* _sl_lpf  = nullptr;
    lv_obj_t* _sl_noise = nullptr;
    lv_obj_t* _sl_speech = nullptr;
};

static std::unique_ptr<BoostTuneWindow> s_boost_tune_window;

static void open_boost_tune_window(lv_obj_t* parent)
{
    if (!parent) {
        parent = lv_screen_active();
    }

    if (!s_boost_tune_window) {
        s_boost_tune_window = std::make_unique<BoostTuneWindow>();
        s_boost_tune_window->init(parent);
    }

    const auto st = s_boost_tune_window->getState();
    if (st == ui::Window::Opened || st == ui::Window::Opening) {
        return;
    }

    s_boost_tune_window->open();
}

static void on_volume_label_long_pressed(lv_event_t* e)
{
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* scr = (target != nullptr) ? lv_obj_get_screen(target) : lv_screen_active();
    open_boost_tune_window(scr);
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
    _label_volume_container = std::make_unique<Container>(lv_screen_active());
    _label_volume_container->align(LV_ALIGN_CENTER, kLabelPosX, kLabelPosY);
    _label_volume_container->setSize(kLabelHitAreaWidth, kLabelHitAreaHeight);
    lv_obj_set_style_bg_opa(_label_volume_container->get(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_label_volume_container->get(), 0, 0);
    _label_volume_container->addFlag(LV_OBJ_FLAG_CLICKABLE);
    _label_volume_container->addEventCb(&on_volume_label_long_pressed, LV_EVENT_LONG_PRESSED);

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

    _label_y_anim.easingOptions().duration = 0.2;
    _label_y_anim.teleport(kLabelPosY);
}

void PanelSpeakerVolume::openBoostTune(lv_obj_t* parent)
{
    open_boost_tune_window(parent);
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

} // namespace launcher_view
