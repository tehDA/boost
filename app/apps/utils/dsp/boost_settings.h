#pragma once
#include <stdint.h>

namespace app::dsp {

/**
 * Central, runtime-tunable audio processing settings.
 *
 * NOTE: This is intentionally processing-agnostic. It is meant to be a stable
 * API surface for the UI and for future audio pipeline backends.
 */
struct BoostSettings
{
    // Global
    bool enabled = true;                 // Master enable/disable (bypass when false)
    bool mono_mix = false;               // Combine to mono (and duplicate to L/R when playing stereo)
    bool near_field = true;              // Near-field vs far-field tuning hint

    // Gain / dynamics
    float pre_gain_db = 0.0f;            // Input gain before processing
    float post_gain_db = 0.0f;           // Output gain after processing
    float limiter_threshold_dbfs = -2.0f;
    float limiter_release_ms = 120.0f;

    // Filters
    float hpf_hz = 120.0f;               // High-pass cutoff
    float lpf_hz = 7000.0f;              // Low-pass cutoff

    // "How much" controls (0..1)
    float noise_reduction = 0.35f;
    float speech_boost = 0.35f;
    float dereverb = 0.0f;

    // Beamforming (future)
    bool beamform_enable = false;
    float beam_width = 0.6f;             // 0..1 (narrow->wide)

    // EQ (future)
    float eq_low_db = 0.0f;
    float eq_mid_db = 0.0f;
    float eq_high_db = 0.0f;

    // I/O routing (future)
    bool aes_enable = false;
    bool headset_mic_only = false;       // prefer headset mic, suppress local playback
};

/**
 * Thread-safe access to the runtime settings.
 *
 * This avoids coupling the UI directly to the processing implementation.
 * The backend can poll or subscribe to changes later.
 */
class BoostSettingsStore
{
public:
    static BoostSettingsStore& instance();

    BoostSettings get() const;
    void set(const BoostSettings& s);

    // Convenience helpers
    void set_enabled(bool v);
    void set_mono(bool v);
    void set_near_field(bool v);

    void set_pre_gain_db(float db);
    void set_post_gain_db(float db);
    void set_hpf_hz(float hz);
    void set_lpf_hz(float hz);

    void set_noise_reduction(float v01);
    void set_speech_boost(float v01);
    void set_dereverb(float v01);

    void set_beamform_enable(bool v);
    void set_beam_width(float v01);

    void set_eq_low_db(float db);
    void set_eq_mid_db(float db);
    void set_eq_high_db(float db);

    void set_aes_enable(bool v);
    void set_headset_mic_only(bool v);

private:
    BoostSettingsStore() = default;
    BoostSettingsStore(const BoostSettingsStore&) = delete;
    BoostSettingsStore& operator=(const BoostSettingsStore&) = delete;

    BoostSettings _s{};
    mutable void* _mutex = nullptr; // created lazily to keep this header IDF-agnostic
};

} // namespace app::dsp
