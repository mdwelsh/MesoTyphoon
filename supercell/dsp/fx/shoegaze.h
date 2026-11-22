#ifndef SUPERCELL_DSP_FX_SHOEGAZE_H_
#define SUPERCELL_DSP_FX_SHOEGAZE_H_

#include <algorithm>
#include <cmath>

#include "supercell/dsp/frame.h"
#include "supercell/dsp/fx/reverb.h"

namespace clouds {

class Shoegaze {
 public:
  void Init(uint16_t* pre_buffer, uint16_t* post_buffer) {
    pre_.Init(pre_buffer);
    post_.Init(post_buffer, 3.0f);
    pre_.set_input_gain(0.25f);
    pre_.set_diffusion(0.7f);
    post_.set_input_gain(0.25f);
    post_.set_diffusion(0.7f);
    post_.set_amount(1.0f);
    tone_l_ = tone_r_ = 0.0f;
    feedback_dc_ = FloatFrame{};
    std::fill_n(feedback_tail_, kMaxBlockSize, FloatFrame{});
    feedback_index_ = 0;
  }

  void Process(FloatFrame* frames, size_t size,
      float pre_mix, float pre_decay, float fuzz, float tone,
      float post_mix, float post_decay, float feedback, float modulation) {
    pre_.set_amount(pre_mix);
    pre_.set_time(DecayFeedback(pre_decay));
    pre_.Process(frames, size);

    const float fuzz_squared = fuzz * fuzz;
    const float drive = 1.0f + fuzz_squared * (47.0f + 80.0f * fuzz_squared * fuzz_squared);
    const float cutoff = 500.0f * std::exp2(4.6f * tone);
    const float coefficient = 1.0f - std::exp(-6.2831853f * cutoff / 32000.0f);
    const float regeneration = 0.85f * feedback * feedback * feedback;
    FloatFrame wet[kMaxBlockSize];
    for (size_t i = 0; i < size; ++i) {
      FloatFrame& frame = frames[i];
      const FloatFrame& echo = feedback_tail_[(feedback_index_ + i) % kMaxBlockSize];
      frame.l += regeneration * echo.l;
      frame.r += regeneration * echo.r;
      tone_l_ += coefficient * (Saturate(frame.l * drive) - tone_l_);
      tone_r_ += coefficient * (Saturate(frame.r * drive) - tone_r_);
      frame.l += fuzz * (tone_l_ - frame.l);
      frame.r += fuzz * (tone_r_ - frame.r);
      wet[i] = frame;
    }

    post_.set_time(DecayFeedback(post_decay));
    post_.set_modulation_depth(modulation);
    post_.Process(wet, size);

    for (size_t i = 0; i < size; ++i) {
      // Regenerate only the wet tail, with a DC blocker and bounded return.
      feedback_dc_.l += 0.002f * (wet[i].l - feedback_dc_.l);
      feedback_dc_.r += 0.002f * (wet[i].r - feedback_dc_.r);
      feedback_tail_[feedback_index_] = {
          Saturate(wet[i].l - feedback_dc_.l), Saturate(wet[i].r - feedback_dc_.r)};
      feedback_index_ = (feedback_index_ + 1) % kMaxBlockSize;
      frames[i].l += post_mix * (wet[i].l - frames[i].l);
      frames[i].r += post_mix * (wet[i].r - frames[i].r);
    }
  }

 private:
  static float DecayFeedback(float control) {
    const float seconds = 0.3f + 19.7f * control * control;
    // Each feedback pass averages about 240 ms. Target -60 dB at the
    // chosen decay time before accounting for damping and quantization.
    return std::exp2(-2.4f / seconds);
  }

  static float Saturate(float sample) {
    const float x = std::max(-3.0f, std::min(3.0f, sample));
    const float square = x * x;
    return x * (27.0f + square) / (27.0f + 9.0f * square);
  }

  Reverb pre_;
  Reverb post_;
  FloatFrame feedback_tail_[kMaxBlockSize];
  FloatFrame feedback_dc_;
  size_t feedback_index_;
  float tone_l_;
  float tone_r_;
};

}  // namespace clouds

#endif  // SUPERCELL_DSP_FX_SHOEGAZE_H_
