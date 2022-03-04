// Copyright 2015-2019 Josh Pieper, jjp@pobox.com.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <optional>

#include "mjlib/base/visitor.h"

namespace mjmech {
namespace mech {

struct TurretCommand {
  /// Command the turret to move at the given rate in the IMU
  /// coordinate frame.
  struct Rate {
    double x_deg_s = 0.0;
    double y_deg_s = 0.0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(x_deg_s));
      a->Visit(MJ_NVP(y_deg_s));
    }
  };
  std::optional<Rate> rate;

  /// If set, the IMU coordinate value takes precedence over the
  /// Rate value.
  struct Imu {
    double x_deg = 0.0;
    double y_deg = 0.0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(x_deg));
      a->Visit(MJ_NVP(y_deg));
    }
  };
  std::optional<Imu> imu;

  /// If set, the Absolute coordinate value takes precedence over
  /// the IMU and Rate values.
  struct Absolute {
    double x_deg = 0.0;
    double y_deg = 0.0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(x_deg));
      a->Visit(MJ_NVP(y_deg));
    }
  };
  std::optional<Absolute> absolute;

  /// If set, the TargetRelative coordinate takes precedence over all
  /// other commands.  It attempts to position any detected target at
  /// the given absolute x and y image coordinates.
  struct TargetRelative {
    int x = 0;
    int y = 0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(x));
      a->Visit(MJ_NVP(y));
    }
  };
  std::optional<TargetRelative> target_relative;

  struct Fire {
    /// The sequence number must be updated to something different
    /// for a new fire command to take effect.  This number should
    /// never be set by C++ software running in this process, but
    /// should only be taken directly from network messages.
    int sequence = 0;

    enum Mode : int {
      kOff,
          kInPos1,
          kInPos2,
          kInPos3,
          kInPos5,
          kNow1,
          kCont,
          };

    Mode command = kOff;

    static std::map<Mode, const char*> CommandMapper() {
      return std::map<Mode, const char*>{
        {kOff, "kOff"},
        {kInPos1, "kInPos1"},
        {kInPos2, "kInPos2"},
        {kInPos3, "kInPos3"},
        {kInPos5, "kInPos5"},
        {kNow1, "kNow1"},
        {kCont, "kCont"},
            };
    }

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(sequence));
      a->Visit(MJ_ENUM(command, &Fire::CommandMapper));
    }
  };

  enum class AgitatorMode {
    kOff,
    kOn,
    kAuto,
  };

  static std::map<AgitatorMode, const char*> AgitatorModeMapper() {
    return std::map<AgitatorMode, const char*>{
      {AgitatorMode::kOff, "kOff"},
      {AgitatorMode::kOn, "kOn"},
      {AgitatorMode::kAuto, "kAuto"},
    };
  }

  struct FireControl {
    Fire fire;
    AgitatorMode agitator = AgitatorMode::kOff;
    bool laser_on = false;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(fire));
      a->Visit(MJ_ENUM(agitator, AgitatorModeMapper));
      a->Visit(MJ_NVP(laser_on));
    }
  };

  FireControl fire_control;

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(MJ_NVP(rate));
    a->Visit(MJ_NVP(imu));
    a->Visit(MJ_NVP(absolute));
    a->Visit(MJ_NVP(target_relative));
    a->Visit(MJ_NVP(fire_control));
  }
};

}
}
