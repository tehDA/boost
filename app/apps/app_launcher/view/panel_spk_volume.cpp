#include "view.h"

#include <memory>
#include "esp_log.h"

static const char* TAG = "panel_spk_volume";

namespace {

// -----------------------------------------------------------------------------
// Minimal, extensible "audio tuning" state.
// This is intentionally simple for now; later we can wire these values into the
// audio pipeline (EQ/filters/beamforming/etc) and persist to NVS.
// -----------------------------------------------------------------------------
struct BoostAudioTuning {
    bool enable_processing = true;
    bool mono_mixdown = false;

    bool hpf_enable = false;
    int  hpf_hz = 120;

    bool lpf_enable = false;
    int  lpf_hz = 12000;

    bool beamforming_enable = false;
    int  near_far = 50;          // 0..100 (0=near field, 100=far field)
    int  mic_gain_db = 0;        // placeholder
};

BoostAudioTuning g_tune;

// Helpers for LVGL event target casting (LVGL9 headers sometimes type this as void*)
static inline lv_obj_t* lv_event_target_obj(lv_event_t* e)
{
    return (lv_obj_t*)lv_event_get_target(e);
}

// -----------------------------------------------------------------------------
// Boost tuning window
// -----------------------------------------------------------------------------
class BoostTuneWindow : public ui::Window {
public:
    BoostTuneWindow()
    {
        config().title = "Audio tuning";
        config().keyframes.close_size = LV_COORD_MAX; // will be filled when opening
        config().keyframes.open_size  = { 340, 520 };
        config().keyframes.close_pos  = { 0, 0 };
        config().keyframes.open_pos   = { 10, 30 };
        config().keyframes.control_points[0] = { 0.34f, 1.56f };
        config().keyframes.control_points[1] = { 0.64f, 1.00f };
        config().keyframes.anim_time = 220;
    }

private:
    lv_obj_t* _content = nullptr;

    // Widgets we may want to refresh when window re-opens
    lv_obj_t* _sw_processing = nullptr;
    lv_obj_t* _sw_mono = nullptr;
    lv_obj_t* _sw_hpf = nullptr;
    lv_obj_t* _sl_hpf = nullptr;
    lv_obj_t* _sw_lpf = nullptr;
    lv_obj_t* _sl_lpf = nullptr;
    lv_obj_t* _sw_beam = nullptr;
    lv_obj_t* _sl_near_far = nullptr;

    static lv_obj_t* create_row(lv_obj_t* parent)
    {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);
        return row;
    }

    static lv_obj_t* create_label(lv_obj_t* parent, const char* text)
    {
        lv_obj_t* l = lv_label_create(parent);
        lv_label_set_text(l, text);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
        lv_obj_set_flex_grow(l, 1);
        return l;
    }

    static lv_obj_t* create_switch(lv_obj_t* parent, bool initial, lv_event_cb_t cb, void* user)
    {
        lv_obj_t* sw = lv_switch_create(parent);
        if (initial) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, user);
        return sw;
    }

    static lv_obj_t* create_slider(lv_obj_t* parent, int min, int max, int initial, lv_event_cb_t cb, void* user)
    {
        lv_obj_t* s = lv_slider_create(parent);
        lv_slider_set_range(s, min, max);
        lv_slider_set_value(s, initial, LV_ANIM_OFF);
        lv_obj_set_width(s, 160);
        lv_obj_add_event_cb(s, cb, LV_EVENT_VALUE_CHANGED, user);
        return s;
    }

    // ----- callbacks -----
    static void on_sw_processing(lv_event_t* e)
    {
        lv_obj_t* sw = lv_event_target_obj(e);
        g_tune.enable_processing = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "tune: processing=%d", (int)g_tune.enable_processing);
    }

    static void on_sw_mono(lv_event_t* e)
    {
        lv_obj_t* sw = lv_event_target_obj(e);
        g_tune.mono_mixdown = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "tune: mono_mixdown=%d", (int)g_tune.mono_mixdown);
    }

    static void on_sw_hpf(lv_event_t* e)
    {
        lv_obj_t* sw = lv_event_target_obj(e);
        g_tune.hpf_enable = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "tune: hpf_enable=%d", (int)g_tune.hpf_enable);
    }

    static void on_sl_hpf(lv_event_t* e)
    {
        lv_obj_t* s = lv_event_target_obj(e);
        g_tune.hpf_hz = (int)lv_slider_get_value(s);
        ESP_LOGI(TAG, "tune: hpf_hz=%d", g_tune.hpf_hz);
    }

    static void on_sw_lpf(lv_event_t* e)
    {
        lv_obj_t* sw = lv_event_target_obj(e);
        g_tune.lpf_enable = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "tune: lpf_enable=%d", (int)g_tune.lpf_enable);
    }

    static void on_sl_lpf(lv_event_t* e)
    {
        lv_obj_t* s = lv_event_target_obj(e);
        g_tune.lpf_hz = (int)lv_slider_get_value(s);
        ESP_LOGI(TAG, "tune: lpf_hz=%d", g_tune.lpf_hz);
    }

    static void on_sw_beam(lv_event_t* e)
    {
        lv_obj_t* sw = lv_event_target_obj(e);
        g_tune.beamforming_enable = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "tune: beamforming=%d", (int)g_tune.beamforming_enable);
    }

    static void on_sl_near_far(lv_event_t* e)
    {
        lv_obj_t* s = lv_event_target_obj(e);
        g_tune.near_far = (int)lv_slider_get_value(s);
        ESP_LOGI(TAG, "tune: near_far=%d", g_tune.near_far);
    }

    void onInit() override
    {
        // Content container
        _content = lv_obj_create(_window->get());
        lv_obj_set_size(_content, lv_pct(100), lv_pct(100));
        lv_obj_set_style_border_width(_content, 0, 0);
        lv_obj_set_style_pad_all(_content, 10, 0);
        lv_obj_set_style_pad_row(_content, 10, 0);
        lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(_content, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(_content, LV_SCROLLBAR_MODE_AUTO);

        // Processing enable
        {
            lv_obj_t* row = create_row(_content);
            create_label(row, "Processing");
            _sw_processing = create_switch(row, g_tune.enable_processing, on_sw_processing, nullptr);
        }

        // Mono mixdown
        {
            lv_obj_t* row = create_row(_content);
            create_label(row, "Mono output");
            _sw_mono = create_switch(row, g_tune.mono_mixdown, on_sw_mono, nullptr);
        }

        // HPF enable + cutoff
        {
            lv_obj_t* row = create_row(_content);
            create_label(row, "HPF");
            _sw_hpf = create_switch(row, g_tune.hpf_enable, on_sw_hpf, nullptr);
            _sl_hpf = create_slider(row, 20, 2000, g_tune.hpf_hz, on_sl_hpf, nullptr);
        }

        // LPF enable + cutoff
        {
            lv_obj_t* row = create_row(_content);
            create_label(row, "LPF");
            _sw_lpf = create_switch(row, g_tune.lpf_enable, on_sw_lpf, nullptr);
            _sl_lpf = create_slider(row, 2000, 20000, g_tune.lpf_hz, on_sl_lpf, nullptr);
        }

        // Beamforming + near/far
        {
            lv_obj_t* row = create_row(_content);
            create_label(row, "Beamforming");
            _sw_beam = create_switch(row, g_tune.beamforming_enable, on_sw_beam, nullptr);
            _sl_near_far = create_slider(row, 0, 100, g_tune.near_far, on_sl_near_far, nullptr);
        }

        // Note / placeholder
        {
            lv_obj_t* note = lv_label_create(_content);
            lv_label_set_text(note,
                              "Tip: long-press the speaker % label to open this panel.\n"
                              "Next: hook these values into the audio DSP pipeline and persist to NVS.");
            lv_obj_set_style_text_opa(note, LV_OPA_70, 0);
            lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
            lv_obj_set_width(note, lv_pct(100));
        }
    }
};

static std::unique_ptr<BoostTuneWindow> s_boost_window;

static void open_boost_tune_window(lv_obj_t* anchor_obj)
{
    if (s_boost_window && s_boost_window->opened()) {
        return;
    }

    s_boost_window = std::make_unique<BoostTuneWindow>();
    s_boost_window->init(lv_screen_active());

    // If we have an anchor, animate from it (nice UX, avoids needing HAL display getters)
    if (anchor_obj) {
        lv_area_t coords;
        lv_obj_get_coords(anchor_obj, &coords);

        ui::Window::WindowKeyFrames& kf = s_boost_window->config().keyframes;
        kf.close_pos.x  = coords.x1;
        kf.close_pos.y  = coords.y1;
        kf.close_size.x = coords.x2 - coords.x1;
        kf.close_size.y = coords.y2 - coords.y1;
    }

    s_boost_window->open();
}

static void label_long_pressed_cb(lv_event_t* e)
{
    open_boost_tune_window(lv_event_target_obj(e));
}

} // namespace

namespace launcher_view {

PanelSpeakerVolume::PanelSpeakerVolume(std::shared_ptr<launcher_view::View> launcher_view)
    : _launcher_view(std::move(launcher_view))
{
}

void PanelSpeakerVolume::init()
{
    // Speaker volume icon
    _btn_volume = std::make_unique<lvgl::Container>(lv_screen_active());
    _btn_volume->setSize(52, 52);
    _btn_volume->align(LV_ALIGN_TOP_LEFT, 10, 54);
    _btn_volume->setStyleRadius(100, LV_PART_MAIN);
    _btn_volume->setStyleBgColor(lv_color_hex(0xffffff), LV_PART_MAIN);
    _btn_volume->setStyleBgOpa(50, LV_PART_MAIN);
    _btn_volume->setStyleBorderWidth(0, LV_PART_MAIN);
    _btn_volume->onPressing([this](lv_event_t*) { _btn_volume->setStyleBgOpa(100, LV_PART_MAIN); });
    _btn_volume->onClick([this](lv_event_t*) {
        _btn_volume->setStyleBgOpa(50, LV_PART_MAIN);
        _launcher_view->onNoticeMessage("SPK", 1000);
        GetHAL()->audioSetSpeakerMute(false);
        if (GetHAL()->audioGetSpeakerVolume() > 0) {
            GetHAL()->audioSetSpeakerVolume(0);
        } else {
            GetHAL()->audioSetSpeakerVolume(0.6f);
        }
    });
    _btn_volume->onReleased([this](lv_event_t*) { _btn_volume->setStyleBgOpa(50, LV_PART_MAIN); });

    _img_volume = std::make_unique<lvgl::Image>(_btn_volume->get());
    _img_volume->setSize(32, 32);
    _img_volume->align(LV_ALIGN_CENTER, 0, 0);
    _img_volume->setSrc("S:/app/assets/icon/volume_black_32.png");

    // Volume label
    _label_volume = std::make_unique<lvgl::Label>(lv_screen_active());
    _label_volume->align(LV_ALIGN_TOP_LEFT, 18, 108);
    _label_volume->setText("  0%");
    _label_volume->setStyleTextFont(&lv_font_montserrat_24, LV_PART_MAIN);
    _label_volume->setStyleTextColor(lv_color_hex(0x000000), LV_PART_MAIN);
    _label_volume->setStyleTextOpa(60, LV_PART_MAIN);

    // Ensure label is clickable for long-press (LVGL labels are not always clickable by default)
    lv_obj_add_flag(_label_volume->get(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_label_volume->get(), label_long_pressed_cb, LV_EVENT_LONG_PRESSED, nullptr);

    _label_volume->onClick([this](lv_event_t*) {
        GetHAL()->audioSetSpeakerMute(!GetHAL()->audioGetSpeakerMute());
        if (GetHAL()->audioGetSpeakerMute()) {
            _launcher_view->onNoticeMessage("SPK mute", 1000);
        } else {
            _launcher_view->onNoticeMessage("SPK on", 1000);
        }
    });

    // Headphone icon
    _btn_headphone = std::make_unique<lvgl::Container>(lv_screen_active());
    _btn_headphone->setSize(52, 52);
    _btn_headphone->align(LV_ALIGN_TOP_RIGHT, -10, 54);
    _btn_headphone->setStyleRadius(100, LV_PART_MAIN);
    _btn_headphone->setStyleBgColor(lv_color_hex(0xffffff), LV_PART_MAIN);
    _btn_headphone->setStyleBgOpa(50, LV_PART_MAIN);
    _btn_headphone->setStyleBorderWidth(0, LV_PART_MAIN);
    _btn_headphone->onPressing([this](lv_event_t*) { _btn_headphone->setStyleBgOpa(100, LV_PART_MAIN); });
    _btn_headphone->onClick([this](lv_event_t*) {
        _btn_headphone->setStyleBgOpa(50, LV_PART_MAIN);
        _launcher_view->openApp("headphone_test");
    });
    _btn_headphone->onReleased([this](lv_event_t*) { _btn_headphone->setStyleBgOpa(50, LV_PART_MAIN); });

    _img_headphone = std::make_unique<lvgl::Image>(_btn_headphone->get());
    _img_headphone->setSize(32, 32);
    _img_headphone->align(LV_ALIGN_CENTER, 0, 0);
    _img_headphone->setSrc("S:/app/assets/icon/headphone_black_32.png");

    // LED indicator on the headphone button
    _cont_headphone_led = std::make_unique<lvgl::Container>(_btn_headphone->get());
    _cont_headphone_led->setSize(10, 10);
    _cont_headphone_led->align(LV_ALIGN_CENTER, 10, -13);
    _cont_headphone_led->setStyleRadius(100, LV_PART_MAIN);
    _cont_headphone_led->setStyleBorderWidth(0, LV_PART_MAIN);
    _cont_headphone_led->setStyleBgOpa(100, LV_PART_MAIN);
    _cont_headphone_led->setStyleBgColor(lv_color_hex(0xff0000), LV_PART_MAIN);

    _cont_headphone_led_off = std::make_unique<lvgl::Container>(_btn_headphone->get());
    _cont_headphone_led_off->setSize(10, 10);
    _cont_headphone_led_off->align(LV_ALIGN_CENTER, 10, -13);
    _cont_headphone_led_off->setStyleRadius(100, LV_PART_MAIN);
    _cont_headphone_led_off->setStyleBorderWidth(0, LV_PART_MAIN);
    _cont_headphone_led_off->setStyleBgOpa(10, LV_PART_MAIN);
    _cont_headphone_led_off->setStyleBgColor(lv_color_hex(0x000000), LV_PART_MAIN);
}

void PanelSpeakerVolume::update()
{
    // Keep tune window alive & updated if opened
    if (s_boost_window && s_boost_window->opened()) {
        s_boost_window->update();
    }

    if (GetHAL()->audioGetSpeakerVolume() > 0) {
        _img_volume->setSrc("S:/app/assets/icon/volume_black_32.png");
    } else {
        _img_volume->setSrc("S:/app/assets/icon/volume_mute_black_32.png");
    }

    if (GetHAL()->audioGetSpeakerMute()) {
        _img_volume->setStyleImgRecolor(lv_color_hex(0xff0000), LV_PART_MAIN);
        _img_volume->setStyleImgRecolorOpa(100, LV_PART_MAIN);
    } else {
        _img_volume->setStyleImgRecolorOpa(0, LV_PART_MAIN);
    }

    float vol = GetHAL()->audioGetSpeakerVolume() * 100;
    if (GetHAL()->audioGetSpeakerMute()) {
        vol = 0;
    }
    static float vol_last = -1;
    if (vol != vol_last) {
        vol_last = vol;
        char buf[8];
        snprintf(buf, sizeof(buf), "%3.0f%%", vol);
        _label_volume->setText(buf);
    }

    if (GetHAL()->audioHeadphoneState()) {
        _cont_headphone_led->show();
        _cont_headphone_led_off->hide();
    } else {
        _cont_headphone_led->hide();
        _cont_headphone_led_off->show();
    }
}

} // namespace launcher_view
