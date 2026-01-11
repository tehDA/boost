#include "view.h"

#include <apps/utils/ui/window.h>

#include <esp_log.h>
#include <lvgl.h>

#include <algorithm>
#include <memory>

using namespace smooth_ui_toolkit::lvgl_cpp;

namespace launcher_view {

static const char* TAG = "boost_tune";

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
        _window->clearChildren();

        lv_obj_t* root = _window->get();
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_all(root, 14, 0);
        lv_obj_set_style_pad_row(root, 10, 0);

        // Header
        lv_obj_t* hint = lv_label_create(root);
        lv_label_set_text(hint, "Preview controls (no DSP wired yet)");
        lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);

        // Mono mix toggle
        _cb_mono = lv_checkbox_create(root);
        lv_checkbox_set_text(_cb_mono, "Mono mix output");
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

        // Close button
        lv_obj_t* btn_close = lv_btn_create(root);
        lv_obj_set_width(btn_close, LV_PCT(100));
        lv_obj_t* btn_lbl = lv_label_create(btn_close);
        lv_label_set_text(btn_lbl, "Close");
        lv_obj_center(btn_lbl);

        lv_obj_add_event_cb(btn_close, &BoostTuneWindow::on_close_pressed, LV_EVENT_CLICKED, this);
    }

private:
    struct State
    {
        bool mono = false;
        int spk_vol_pct = 60;
        int hpf_hz = 100;
        int lpf_hz = 12000;
    };

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
        // TODO: wire to DSP/audio graph when it exists.
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
        // TODO: wire to DSP/audio graph when it exists.
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
        // TODO: wire to DSP/audio graph when it exists.
    }

private:
    State _state{};

    lv_obj_t* _cb_mono = nullptr;
    lv_obj_t* _sl_vol  = nullptr;
    lv_obj_t* _sl_hpf  = nullptr;
    lv_obj_t* _sl_lpf  = nullptr;
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
    if (st == ui::Window::State::Opened || st == ui::Window::State::Opening) {
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
    return fmt::format("{}%", GetHAL()->getSpeakerVolume());
}

static void update_vol_str(std::shared_ptr<Label> label_vol)
{
    if (label_vol) label_vol->setText(get_vol_str());
}

static void update_vol(int offset, std::shared_ptr<Label> label_vol)
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
    _container->addFlag(LV_OBJ_FLAG_CLICKABLE);
    _container->setFlexFlow(LV_FLEX_FLOW_ROW);
    _container->setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    _btn_minus = std::make_shared<Button>(_container->get());
    _btn_minus->setSize(50, 50);
    _btn_minus->setStyleRadius(LV_RADIUS_CIRCLE, 0);
    _btn_minus->setStyleBgColor(lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    _btn_minus->setStyleBgOpa(LV_OPA_COVER, 0);
    _btn_minus->addEventCb(
        [](lv_event_t*) {
            update_vol(-10, _label_volume);
        },
        LV_EVENT_CLICKED);

    auto label_minus = std::make_shared<Label>(_btn_minus->get());
    label_minus->setText("-");
    label_minus->setStyleTextFont(&lv_font_montserrat_24, 0);
    label_minus->center();

    _label_volume = std::make_shared<Label>(_container->get());
    _label_volume->setText(get_vol_str());
    _label_volume->setStyleTextFont(&lv_font_montserrat_24, 0);

    // Long-press the volume label to open the audio tuning window.
    _label_volume->addEventCb(&on_volume_label_long_pressed, LV_EVENT_LONG_PRESSED);

    _btn_plus = std::make_shared<Button>(_container->get());
    _btn_plus->setSize(50, 50);
    _btn_plus->setStyleRadius(LV_RADIUS_CIRCLE, 0);
    _btn_plus->setStyleBgColor(lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    _btn_plus->setStyleBgOpa(LV_OPA_COVER, 0);
    _btn_plus->addEventCb(
        [](lv_event_t*) {
            update_vol(10, _label_volume);
        },
        LV_EVENT_CLICKED);

    auto label_plus = std::make_shared<Label>(_btn_plus->get());
    label_plus->setText("+");
    label_plus->setStyleTextFont(&lv_font_montserrat_24, 0);
    label_plus->center();
}

void PanelSpeakerVolume::update()
{
    update_vol_str(_label_volume);
}

} // namespace launcher_view
