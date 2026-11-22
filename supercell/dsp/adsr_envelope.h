#ifndef SUPERCELL_DSP_ADSR_ENVELOPE_H_
#define SUPERCELL_DSP_ADSR_ENVELOPE_H_

#include <algorithm>

namespace clouds {

class AdsrEnvelope {
 public:
  void Reset() {
    stage_ = IDLE;
    gate_ = false;
    level_ = 0.0f;
    release_start_ = 0.0f;
  }

  void Configure(bool gate, float attack, float decay, float sustain, float release) {
    if (gate != gate_) {
      gate_ = gate;
      stage_ = gate ? ATTACK : RELEASE;
      if (!gate) {
        release_start_ = level_;
      }
    }
    sustain_ = sustain;
    attack_step_ = 1.0f / StageSamples(attack);
    decay_step_ = (1.0f - sustain) / StageSamples(decay);
    release_step_ = release_start_ / StageSamples(release);
  }

  float Next() {
    switch (stage_) {
      case ATTACK:
        level_ = std::min(1.0f, level_ + attack_step_);
        if (level_ >= 1.0f) {
          stage_ = DECAY;
        }
        break;
      case DECAY:
        level_ = std::max(sustain_, level_ - decay_step_);
        if (level_ <= sustain_) {
          stage_ = SUSTAIN;
        }
        break;
      case SUSTAIN:
        level_ = sustain_;
        break;
      case RELEASE:
        level_ = std::max(0.0f, level_ - release_step_);
        if (level_ <= 0.0f) {
          stage_ = IDLE;
        }
        break;
      case IDLE:
        break;
    }
    return level_;
  }

 private:
  enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

  static float StageSamples(float control) {
    // The codec runs at 32 kHz regardless of the selected effect quality.
    const float min_samples = 32000.0f * 0.001f;
    const float max_samples = 32000.0f * 10.0f;
    return min_samples + (max_samples - min_samples) * control * control * control;
  }

  Stage stage_;
  bool gate_;
  float level_;
  float release_start_;
  float sustain_;
  float attack_step_;
  float decay_step_;
  float release_step_;
};

}  // namespace clouds

#endif  // SUPERCELL_DSP_ADSR_ENVELOPE_H_
