// Copyright 2019-2020 Josh Pieper, jjp@pobox.com.
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

#include "mjlib/base/fail.h"

#include "mech/quadruped_command.h"

namespace mjmech {
namespace mech {

template <typename T>
QuadrupedCommand::Leg& GetLeg_R(T* legs_R, int id) {
  for (auto& leg_R : *legs_R) {
    if (leg_R.leg_id == id) { return leg_R; }
  }
  mjlib::base::AssertNotReached();
}

struct FilterCommandState {
  Eigen::Vector3d v;
  Eigen::Vector3d w;
};

inline FilterCommandState FilterCommand(
    const FilterCommandState& current,
    const FilterCommandState& desired,
    double acceleration,
    double alpha_rad_s2,
    double delta_s) {
  FilterCommandState result = current;

  const base::Point3D input_delta = (desired.v - current.v);
  const double input_delta_norm = input_delta.norm();
  const double max_delta = acceleration * delta_s;
  const base::Point3D delta =
      (input_delta_norm < max_delta) ?
      input_delta :
      input_delta.normalized() * max_delta;

  result.v += delta;
  // We require this.
  result.v.z() = 0;

  const base::Point3D input_delta_rad_s = (desired.w - current.w);
  const double input_delta_norm_rad_s = input_delta_rad_s.norm();
  const double max_delta_rad_s = alpha_rad_s2 * delta_s;
  const base::Point3D delta_rad_s =
      (input_delta_norm_rad_s < max_delta_rad_s) ?
      input_delta_rad_s :
      input_delta_rad_s.normalized() * max_delta_rad_s;

  result.w += delta_rad_s;
  // We only allow a z value.
  result.w.x() = result.w.y() = 0.0;

  return result;
}


}
}
