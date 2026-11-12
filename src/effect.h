#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: effect.h
 *
 *  Noise gate generic effect for the NTS-3 kaoss pad kit.
 *
 *  The gate is opened/closed by one of several sources (see GateType):
 *    - a steady beat at 1/2, 1/4, 1/8 or 1/16 notes of the current tempo
 *      (open for the first half of each division, closed for the second half),
 *    - MIDI CC (open while the CC_GATE parameter is >= 50%; assign CC_GATE to Pad X/Y or
 *      Depth on the device and send that control's CC, e.g. CC 12/13/14; value >= 64 opens),
 *    - the XY pad (open while touched, closed on release).
 *
 *  When the gate opens, gain ramps up to unity over ATTACK ms; when it closes,
 *  gain ramps down to the REDUCE level over RELEASE ms.
 */
#include <math.h>
#include "processor.h"
#include "unit_genericfx.h"

class Effect : public Processor
{
public:
  // A gate does not need a delay line or any SDRAM.
  uint32_t getBufferSize() const override final { return 0; }

  // audio parameters (order must match header.c)
  enum
  {
    ATTACK = 0U,
    RELEASE,
    REDUCE,
    GATE_TYPE,
    CC_GATE,
    NUM_PARAMS
  };

  enum GateType
  {
    GATE_HALF = 0,
    GATE_QUARTER,
    GATE_EIGHTH,
    GATE_SIXTEENTH,
    GATE_CC,
    GATE_PAD,
    NUM_GATE_TYPES,
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    int32_t attack_ms;
    int32_t release_ms;
    int32_t reduce_db;
    uint32_t gate_type;
    int32_t gate;

    void reset()
    {
      attack_ms = 5;
      release_ms = 100;
      reduce_db = -144;
      gate_type = GATE_QUARTER;
      gate = 0;
    }

    Params() { reset(); }
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case ATTACK:
      params_.attack_ms = value;
      attack_coef_ = timeToCoef(value);
      break;

    case RELEASE:
      params_.release_ms = value;
      release_coef_ = timeToCoef(value);
      break;

    case REDUCE:
      params_.reduce_db = value;
      // -144 dB is treated as full silence
      floor_gain_ = (value <= -144) ? 0.f : powf(10.f, value / 20.f);
      break;

    case GATE_TYPE:
      if (value >= 0 && value < NUM_GATE_TYPES)
        params_.gate_type = value;
      break;

    case CC_GATE:
      params_.gate = value;
      cc_open_ = (value >= 50);
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    // Note: String memory must be accessible even after function returned.
    static const char *gate_type_strings[NUM_GATE_TYPES] = {
        "1/2",
        "1/4",
        "1/8",
        "1/16",
        "CC",
        "PAD",
    };

    switch (index)
    {
    case GATE_TYPE:
      if (value >= 0 && value < NUM_GATE_TYPES)
        return gate_type_strings[value];
      break;
    default:
      break;
    }

    return nullptr;
  }

  // life-cycle methods
  void init(float *allocated_buffer) override final
  {
    (void)allocated_buffer;
    params_.reset();
    setParameter(ATTACK, params_.attack_ms);
    setParameter(RELEASE, params_.release_ms);
    setParameter(REDUCE, params_.reduce_db);
    setParameter(CC_GATE, params_.gate);
    setTempo(120.f);
    reset();
  }

  void reset() override final
  {
    gain_ = 1.f;
    beat_pos_ = 0.f;
    touching_ = false;
  }

  // audio processing callbacks
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const uint32_t type = params_.gate_type;
    const float attack = attack_coef_;
    const float release = release_coef_;
    const float floor_gain = floor_gain_;
    const float beat_inc = beat_inc_;
    const float division = divisionSixteenths(type);

    // Event-driven gate state is constant over the block
    const bool event_open = (type == GATE_CC) ? cc_open_ : touching_;

    float gain = gain_;
    float pos = beat_pos_;

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      // Advance the beat clock (in 16th notes, wrapped to one bar)
      pos += beat_inc;
      if (pos >= 16.f)
        pos -= 16.f;

      bool open;
      if (division > 0.f)
        open = fmodf(pos, division) < 0.5f * division; // open for the first half of each division
      else
        open = event_open;

      const float target = open ? 1.f : floor_gain;
      const float coef = (target > gain) ? attack : release;
      gain = target + coef * (gain - target);
      if (fabsf(gain - target) < 1e-9f)
        gain = target; // settle exactly and avoid denormals

      out[0] = in[0] * gain;
      out[1] = in[1] * gain;
    }

    gain_ = gain;
    beat_pos_ = pos;
  }

  // XY pad: touching opens the gate, releasing closes it
  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    (void)id;
    (void)x;
    (void)y;

    switch (phase)
    {
    case k_unit_touch_phase_began:
    case k_unit_touch_phase_moved:
    case k_unit_touch_phase_stationary:
      touching_ = true;
      break;
    case k_unit_touch_phase_ended:
    case k_unit_touch_phase_cancelled:
      touching_ = false;
      break;
    default:
      break;
    }
  }

  inline void setTempo(float bpm) override final
  {
    if (bpm < 1.f)
      bpm = 1.f;
    // 16th notes per sample: (bpm / 60) beats per second * 4 sixteenths per beat
    beat_inc_ = bpm * 4.f / (60.f * getSampleRate());
  }

  // Called on every 16th note: resync the beat clock to the host/clock position
  inline void tempo4ppqnTick(uint32_t counter) override final
  {
    beat_pos_ = static_cast<float>(counter & 0xF);
  }

private:
  // One-pole coefficient that reaches ~99% of the target in `ms` milliseconds
  static float timeToCoef(int32_t ms)
  {
    if (ms <= 0)
      return 0.f;
    return expf(-4.6051702f / (ms * 0.001f * getSampleRate()));
  }

  // Length of one gate cycle in 16th notes, or 0 for event-driven gate types
  static float divisionSixteenths(uint32_t type)
  {
    switch (type)
    {
    case GATE_HALF:
      return 8.f;
    case GATE_QUARTER:
      return 4.f;
    case GATE_EIGHTH:
      return 2.f;
    case GATE_SIXTEENTH:
      return 1.f;
    default:
      return 0.f;
    }
  }

  Params params_;

  float attack_coef_ = 0.f;
  float release_coef_ = 0.f;
  float floor_gain_ = 0.f;

  float gain_ = 1.f;     // current smoothed gain
  float beat_pos_ = 0.f; // position within a bar, in 16th notes [0, 16)
  float beat_inc_ = 0.f; // 16th notes advanced per sample

  volatile bool touching_ = false;
  volatile bool cc_open_ = false;
};
