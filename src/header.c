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
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit generic effect unit header definition
 *
 */

#include "unit_genericfx.h"   // Note: Include base definitions for genericfx units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),           // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx, // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                                  // API version for which unit was built. See runtime.h
    .dev_id = 0x0,                                            // Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md
    .unit_id = 0x0U,                                          // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00010000U,                                   // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "RHYTHM GATE",                                      // Name for this unit, will be displayed on device
    .num_params = 5,                                          // Number of valid parameter descriptors. (max. 8)

    .params = {
      // Format: min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name
      // Note: keep defaults in sync with Effect::Params::reset() in effect.h

      // Attack: time for the gate to open, 0 .. 2000 ms
      {0, 2000, 0, 5, k_unit_param_type_msec, 0, 0, 0, {"ATTACK"}},

      // Release: time for the gate to close, 0 .. 2000 ms
      {0, 2000, 0, 100, k_unit_param_type_msec, 0, 0, 0, {"RELEASE"}},

      // Reduction: gain applied while the gate is closed, 0 dB (no change) .. -144 dB (silence)
      {-144, 0, 0, -144, k_unit_param_type_db, 0, 0, 0, {"REDUCE"}},

      // Gate type: tempo divisions, MIDI CC (via GATE), or XY pad touch
      {0, 5, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"TYPE"}},

      // Gate control for CC mode: gate is open at >= 50%. Assign it to Pad X/Y or Depth
      // on the device, then drive it with that control's MIDI CC (CC value >= 64 opens).
      {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"CC GATE"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
  },
  .default_mappings = {
    // By default, the parameters described above will be mapped to controls as described below.
    // These assignments can be overriden by the user.

    // Format: assign, curve, curve polarity, min, max, default value

    // ATTACK on the X axis, exponential curve so short times get more resolution
    {k_genericfx_param_assign_x, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 0, 2000, 5},

    // RELEASE on the Y axis, exponential curve
    {k_genericfx_param_assign_y, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 0, 2000, 100},

    // REDUCE on the depth control
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, -144, 0, -144},

    // TYPE not assigned to a control, defaults to 1/4
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 5, 1},

    // CC GATE not assigned by default (assign it on the device to use CC mode)
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 0},

    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
