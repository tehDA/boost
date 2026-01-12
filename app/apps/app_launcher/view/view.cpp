/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "launcher-view";

void LauncherView::init()
{
    mclog::tagInfo(_tag, "init");

    ui::signal_window_opened().clear();
    ui::signal_window_opened().connect([&](bool opened) { _is_stacked = opened; });

    LvglLockGuard lock;

    // Base screen
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    // Background image
    _img_bg = std::make_unique<Image>(lv_screen_active());
    _img_bg->setAlign(LV_ALIGN_CENTER);
    _img_bg->setSrc(&launcher_bg);
    _img_bg->setOpa(160);

    // Starship-style overlay to calm background noise
    _panel_overlay = std::make_unique<Container>(lv_screen_active());
    _panel_overlay->align(LV_ALIGN_CENTER, 0, 0);
    _panel_overlay->setSize(1280, 720);
    lv_obj_set_style_bg_color(_panel_overlay->get(), lv_color_hex(0x061623), 0);
    lv_obj_set_style_bg_opa(_panel_overlay->get(), LV_OPA_40, 0);
    lv_obj_set_style_border_width(_panel_overlay->get(), 0, 0);
    lv_obj_clear_flag(_panel_overlay->get(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_panel_overlay->get(), LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* lcars_left = lv_obj_create(_panel_overlay->get());
    lv_obj_set_size(lcars_left, 140, 28);
    lv_obj_align(lcars_left, LV_ALIGN_TOP_LEFT, 18, 18);
    lv_obj_set_style_bg_color(lcars_left, lv_color_hex(0x1E5B6B), 0);
    lv_obj_set_style_bg_opa(lcars_left, LV_OPA_70, 0);
    lv_obj_set_style_border_width(lcars_left, 0, 0);
    lv_obj_clear_flag(lcars_left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_right = lv_obj_create(_panel_overlay->get());
    lv_obj_set_size(lcars_right, 220, 18);
    lv_obj_align(lcars_right, LV_ALIGN_TOP_RIGHT, -18, 26);
    lv_obj_set_style_bg_color(lcars_right, lv_color_hex(0x12384B), 0);
    lv_obj_set_style_bg_opa(lcars_right, LV_OPA_70, 0);
    lv_obj_set_style_border_width(lcars_right, 0, 0);
    lv_obj_clear_flag(lcars_right, LV_OBJ_FLAG_SCROLLABLE);

    // ESP32P4 chip hotspot for audio tuning menu
    _btn_esp32p4 = std::make_unique<Container>(lv_screen_active());
    _btn_esp32p4->align(LV_ALIGN_CENTER, -10, 140);
    _btn_esp32p4->setSize(240, 170);
    _btn_esp32p4->setOpa(0);
    _btn_esp32p4->addFlag(LV_OBJ_FLAG_CLICKABLE);
    _btn_esp32p4->onClick().connect([&]() {
        PanelSpeakerVolume::openBoostTune(lv_screen_active());
    });

    // Install panels
    _panels.push_back(std::make_unique<PanelRtc>());
    _panels.push_back(std::make_unique<PanelLcdBacklight>());
    _panels.push_back(std::make_unique<PanelSpeakerVolume>());
    _panels.push_back(std::make_unique<PanelPowerMonitor>());
    _panels.push_back(std::make_unique<PanelImu>());
    _panels.push_back(std::make_unique<PanelSwitches>());
    _panels.push_back(std::make_unique<PanelPower>());
    _panels.push_back(std::make_unique<PanelCamera>());
    _panels.push_back(std::make_unique<PanelDualMic>());
    _panels.push_back(std::make_unique<PanelHeadphone>());
    _panels.push_back(std::make_unique<PanelSdCard>());
    _panels.push_back(std::make_unique<PanelI2cScan>());
    _panels.push_back(std::make_unique<PanelGpioTest>());
    _panels.push_back(std::make_unique<PanelMusic>());
    _panels.push_back(std::make_unique<PanelComMonitor>());

    for (auto& panel : _panels) {
        panel->init();
    }
}

void LauncherView::update()
{
    LvglLockGuard lock;

    for (auto& panel : _panels) {
        panel->update(_is_stacked);
    }
}
