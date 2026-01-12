#include "boost_settings.h"
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace app::dsp {

static float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }

BoostSettingsStore& BoostSettingsStore::instance()
{
    static BoostSettingsStore inst;
    if (inst._mutex == nullptr) {
        inst._mutex = xSemaphoreCreateMutex();
    }
    return inst;
}

BoostSettings BoostSettingsStore::get() const
{
    const auto m = static_cast<SemaphoreHandle_t>(_mutex);
    if (m) xSemaphoreTake(m, portMAX_DELAY);
    BoostSettings out = _s;
    if (m) xSemaphoreGive(m);
    return out;
}

void BoostSettingsStore::set(const BoostSettings& s)
{
    const auto m = static_cast<SemaphoreHandle_t>(_mutex);
    if (m) xSemaphoreTake(m, portMAX_DELAY);
    _s = s;
    if (m) xSemaphoreGive(m);
}

void BoostSettingsStore::set_enabled(bool v) { auto s = get(); s.enabled = v; set(s); }
void BoostSettingsStore::set_mono(bool v) { auto s = get(); s.mono_mix = v; set(s); }
void BoostSettingsStore::set_near_field(bool v) { auto s = get(); s.near_field = v; set(s); }

void BoostSettingsStore::set_pre_gain_db(float db) { auto s = get(); s.pre_gain_db = db; set(s); }
void BoostSettingsStore::set_post_gain_db(float db) { auto s = get(); s.post_gain_db = db; set(s); }
void BoostSettingsStore::set_hpf_hz(float hz) { auto s = get(); s.hpf_hz = std::max(0.0f, hz); set(s); }
void BoostSettingsStore::set_lpf_hz(float hz) { auto s = get(); s.lpf_hz = std::max(0.0f, hz); set(s); }

void BoostSettingsStore::set_noise_reduction(float v01) { auto s = get(); s.noise_reduction = clamp01(v01); set(s); }
void BoostSettingsStore::set_speech_boost(float v01) { auto s = get(); s.speech_boost = clamp01(v01); set(s); }
void BoostSettingsStore::set_dereverb(float v01) { auto s = get(); s.dereverb = clamp01(v01); set(s); }

void BoostSettingsStore::set_beamform_enable(bool v) { auto s = get(); s.beamform_enable = v; set(s); }
void BoostSettingsStore::set_beam_width(float v01) { auto s = get(); s.beam_width = clamp01(v01); set(s); }

void BoostSettingsStore::set_eq_low_db(float db) { auto s = get(); s.eq_low_db = db; set(s); }
void BoostSettingsStore::set_eq_mid_db(float db) { auto s = get(); s.eq_mid_db = db; set(s); }
void BoostSettingsStore::set_eq_high_db(float db) { auto s = get(); s.eq_high_db = db; set(s); }

void BoostSettingsStore::set_aes_enable(bool v) { auto s = get(); s.aes_enable = v; set(s); }
void BoostSettingsStore::set_headset_mic_only(bool v) { auto s = get(); s.headset_mic_only = v; set(s); }

} // namespace app::dsp
