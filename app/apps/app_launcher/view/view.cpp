/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>

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

    // Base screen: dark, high-contrast LCARS-style canvas
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x05070B), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_size(scr, 1680, 720);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(scr, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_ACTIVE);

    lv_obj_t* lcars_left_rail = lv_obj_create(scr);
    lv_obj_set_size(lcars_left_rail, 12, 720);
    lv_obj_set_pos(lcars_left_rail, 0, 0);
    lv_obj_set_style_bg_color(lcars_left_rail, lv_color_hex(0x1A2A38), 0);
    lv_obj_set_style_bg_opa(lcars_left_rail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lcars_left_rail, 0, 0);
    lv_obj_clear_flag(lcars_left_rail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_right_rail = lv_obj_create(scr);
    lv_obj_set_size(lcars_right_rail, 12, 720);
    lv_obj_set_pos(lcars_right_rail, 1668, 0);
    lv_obj_set_style_bg_color(lcars_right_rail, lv_color_hex(0x1A2A38), 0);
    lv_obj_set_style_bg_opa(lcars_right_rail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lcars_right_rail, 0, 0);
    lv_obj_clear_flag(lcars_right_rail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_footer = lv_obj_create(scr);
    lv_obj_set_size(lcars_footer, 1680, 8);
    lv_obj_set_pos(lcars_footer, 0, 712);
    lv_obj_set_style_bg_color(lcars_footer, lv_color_hex(0x0E202E), 0);
    lv_obj_set_style_bg_opa(lcars_footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lcars_footer, 0, 0);
    lv_obj_clear_flag(lcars_footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_header = lv_obj_create(scr);
    lv_obj_set_size(lcars_header, 640, 36);
    lv_obj_align(lcars_header, LV_ALIGN_TOP_LEFT, 18, 18);
    lv_obj_set_style_bg_color(lcars_header, lv_color_hex(0x203A4F), 0);
    lv_obj_set_style_bg_opa(lcars_header, LV_OPA_90, 0);
    lv_obj_set_style_border_width(lcars_header, 0, 0);
    lv_obj_clear_flag(lcars_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_title = lv_label_create(lcars_header);
    lv_label_set_text(lcars_title, "BOOST LCARS SYSTEMS");
    lv_obj_set_style_text_color(lcars_title, lv_color_hex(0xA7F0FF), 0);
    lv_obj_set_style_text_font(lcars_title, &lv_font_montserrat_18, 0);
    lv_obj_align(lcars_title, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t* lcars_pill = lv_obj_create(scr);
    lv_obj_set_size(lcars_pill, 220, 18);
    lv_obj_align(lcars_pill, LV_ALIGN_TOP_RIGHT, -22, 26);
    lv_obj_set_style_bg_color(lcars_pill, lv_color_hex(0x133044), 0);
    lv_obj_set_style_bg_opa(lcars_pill, LV_OPA_90, 0);
    lv_obj_set_style_border_width(lcars_pill, 0, 0);
    lv_obj_clear_flag(lcars_pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lcars_accent = lv_obj_create(scr);
    lv_obj_set_size(lcars_accent, 90, 6);
    lv_obj_align(lcars_accent, LV_ALIGN_TOP_LEFT, 18, 60);
    lv_obj_set_style_bg_color(lcars_accent, lv_color_hex(0xE58C6B), 0);
    lv_obj_set_style_bg_opa(lcars_accent, LV_OPA_90, 0);
    lv_obj_set_style_border_width(lcars_accent, 0, 0);
    lv_obj_clear_flag(lcars_accent, LV_OBJ_FLAG_SCROLLABLE);

    // Install panels
    _panels.push_back(std::make_unique<PanelRtc>());
    _panels.push_back(std::make_unique<PanelImu>());
    _panels.push_back(std::make_unique<PanelSpeakerVolume>());

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
