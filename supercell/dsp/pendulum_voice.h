#ifndef SUPERCELL_DSP_PENDULUM_VOICE_H_
#define SUPERCELL_DSP_PENDULUM_VOICE_H_

#include <cmath>
#include <algorithm>

#include "supercell/dsp/adsr_envelope.h"

namespace clouds {

class PendulumVoice {
 public:
  void Reset() {
    envelope.Reset();
    std::fill(history, history + 2 * kFilterSize, 0.f);
    writeIndex = 0;
    phase = drivePhase = angle = velocity = dc = 0.f;
  }

  void Configure(bool gate, float pitch, float attack, float decay, float sustain,
                 float release, float amount, float shape, float fold, float animate) {
    envelope.Configure(gate, attack, decay, sustain, release);
    const float frequency = 261.625565f * std::exp2(pitch / 12.f);
    phaseStep = kTau * frequency / (2.f * kSampleRate);
    // A bipolar quadratic taper gives fine detune control around noon.
    const float centeredAnimate = 2.f * animate - 1.f;
    const float detuneCents = 100.f * centeredAnimate * std::abs(centeredAnimate);
    drivePhaseStep = phaseStep * std::exp2(detuneCents / 1200.f);
    chaosAmount = amount;
    force = 0.75f + 1.15f * shapeToDrive(shape);
    secondHarmonicMix = 0.65f * shape;
    drive = 1.f + 4.f * fold;
  }

  float Next() {
    const float gain = envelope.Next();
    float oversampled[2];
    for (float& sample : oversampled) {
      const float carrier = Sine(phase);

      // Integrate a damped, driven pendulum with a midpoint step. Time is
      // measured in radians of its own drive so Animate detunes this system
      // without changing the keyed sine or the behavior selected by Shape.
      const float halfStep = 0.5f * drivePhaseStep;
      const float midAngle = angle + halfStep * velocity;
      const float midVelocity = velocity + halfStep * acceleration(angle, velocity, Sine(drivePhase));
      velocity += drivePhaseStep * acceleration(midAngle, midVelocity, Sine(drivePhase + halfStep));
      angle = WrapAngle(angle + drivePhaseStep * midVelocity);
      phase = advancePhase(phase, phaseStep);
      drivePhase = advancePhase(drivePhase, drivePhaseStep);

      // Shape also changes the pendulum's harmonic balance in regions where
      // its motion locks to the carrier and becomes periodic again.
      const float fundamental = Sine(angle);
      const float secondHarmonic = 2.f * fundamental * Sine(angle + kTau / 4.f);
      const float pendulum = fundamental + secondHarmonicMix * (secondHarmonic - fundamental);
      const float signal = carrier + chaosAmount * (pendulum - carrier);
      // Reflect successive peaks back through the range [-1, 1].
      const float driven = signal * drive - 1.f;
      const int period = static_cast<int>((driven + 10.f) * 0.25f);
      sample = 1.f - std::abs(driven + 8.f - 4.f * period);
    }
    const float signal = Decimate(oversampled);
    dc += kDcCoefficient * (signal - dc);
    // Leave headroom for overshoot from the decimation filter.
    return 0.7f * gain * (signal - dc);
  }

 private:
  static constexpr float kTau = 6.28318530718f;
  static constexpr float kSampleRate = 32000.f;
  static constexpr float kDcCoefficient = kTau * 8.f / kSampleRate;

  static constexpr int kFilterSize = 24;

  static float WrapAngle(float angle) {
    if (angle > kTau / 2.f) {
      return angle - kTau;
    }
    if (angle < -kTau / 2.f) {
      return angle + kTau;
    }
    return angle;
  }

  static float Sine(float angle) {
    float x = WrapAngle(angle);
    // Reduce to the central half wave, then evaluate through x^9.
    if (std::abs(x) > kTau / 4.f) {
      x = std::copysign(kTau / 2.f, x) - x;
    }
    const float x2 = x * x;
    return x * (1.f + x2 * (-1.f / 6.f + x2 * (1.f / 120.f
        + x2 * (-1.f / 5040.f + x2 / 362880.f))));
  }

  float Decimate(const float* samples) {
    // Symmetric Blackman-Harris windowed sinc, cutoff 0.225 at 64 kHz.
    // This matches the 24-tap decimator used by the original Rack voice.
    static constexpr float kernel[kFilterSize / 2] = {
      -8.6773804521e-07f, 3.0135042759e-05f, 1.8702691993e-04f,
      -4.8691873629e-04f, -2.5174289526e-03f, 1.5611129545e-03f,
      1.4408544245e-02f, 2.2349021012e-03f, -5.1448334211e-02f,
      -3.7074323729e-02f, 1.6410115707e-01f, 4.0900522109e-01f,
    };
    for (int i = 0; i < 2; ++i) {
      history[writeIndex + i] = history[writeIndex + i + kFilterSize] = samples[i];
    }
    writeIndex += 2;
    if (writeIndex == kFilterSize) {
      writeIndex = 0;
    }
    const float* window = &history[writeIndex];
    float output = 0.f;
    for (int i = 0; i < kFilterSize / 2; ++i) {
      output += kernel[i] * (window[i] + window[kFilterSize - 1 - i]);
    }
    return output;
  }

  static float shapeToDrive(float shape) {
    // Local taper centered between the repeating and irregular regimes.
    // The slope is 0.2 at the center and rejoins the linear curve smoothly.
    constexpr float center = 0.725f;
    constexpr float radius = 0.22f;
    const float distance = shape - center;
    const float x = distance / radius;
    if (std::abs(x) >= 1.f) {
      return shape;
    }
    const float window = 1.f - x * x;
    return shape - 0.8f * distance * window * window;
  }

  static float advancePhase(float phase, float step) {
    phase += step;
    return phase >= kTau ? phase - kTau : phase;
  }

  float acceleration(float position, float speed, float excitation) const {
    return force * excitation - 0.28f * speed - 1.65f * Sine(position);
  }

  AdsrEnvelope envelope;
  float history[2 * kFilterSize];
  int writeIndex;
  float phase = 0.f;
  float drivePhase = 0.f;
  float angle = 0.f;
  float velocity = 0.f;
  float dc = 0.f;
  float phaseStep = 0.f;
  float drivePhaseStep = 0.f;
  float chaosAmount = 0.f;
  float force = 0.f;
  float secondHarmonicMix = 0.f;
  float drive = 1.f;
};

}  // namespace clouds

#endif  // SUPERCELL_DSP_PENDULUM_VOICE_H_
