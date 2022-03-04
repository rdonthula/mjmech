// Copyright 2014-2020 Josh Pieper, jjp@pobox.com.
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

#include <cmath>

#include <boost/assert.hpp>

#include <Eigen/Core>

#include "euler.h"
#include "point3d.h"

namespace mjmech {
namespace base {
class Quaternion;
inline Quaternion operator*(const Quaternion& lhs,
                            const Quaternion& rhs);

class Quaternion {
 public:
  Quaternion(double w, double x, double y, double z)
      : w_(w), x_(x), y_(y), z_(z) {}

  Quaternion(Eigen::Quaterniond rhs)
      : w_(rhs.w()), x_(rhs.x()), y_(rhs.y()), z_(rhs.z()) {}

  Quaternion()
      : w_(1.0), x_(0.), y_(0.), z_(0.) {}

  std::string str() const;

  Point3D Rotate(const Point3D& vector3d) const {
    Quaternion p(0.0,
                 vector3d(0),
                 vector3d(1),
                 vector3d(2));
    Quaternion q = *this * p * conjugated();
    return Point3D(q.x(), q.y(), q.z());
  }

  Quaternion conjugated() const {
    return Quaternion(w_, -x_, -y_, -z_);
  }

  double norm() const {
    return std::sqrt(w_ * w_ + x_ * x_ + y_ * y_ + z_ * z_);
  }

  Quaternion normalized() const {
    const double n = norm();
    return Quaternion(w_ / n, x_ / n, y_ / n, z_ / n);
  }

  Eigen::Quaterniond eigen() const {
    return Eigen::Quaterniond(w(), x(), y(), z());
  }

  Eigen::Matrix<double, 3, 3> matrix() const {
    Eigen::Matrix<double, 3, 3> r;

    r(0, 0) = w_ * w_ + x_ * x_ - y_ * y_ - z_ * z_;
    r(0, 1) = 2 * (x_ * y_ - w_ * z_);
    r(0, 2) = 2 * (w_ * y_ + x_ * z_);
    r(1, 0) = 2 * (x_ * y_ + w_ * z_);
    r(1, 1) = w_ * w_ - x_ * x_ + y_ * y_ - z_ * z_;
    r(1, 2) = 2 * (y_ * z_ - w_ * x_);
    r(2, 0) = 2 * (x_ * z_ - w_ * y_);
    r(2, 1) = 2 * (w_ * x_ + y_ * z_);
    r(2, 2) = w_ * w_ - x_ * x_ - y_ * y_ + z_ * z_;

    return r;
  }

  Euler euler_rad() const {
    Euler result_rad;

    const double sinp = 2.0 * (w_ * y_ - z_ * x_);
    if (sinp >= (1.0 - 1e-8)) {
      result_rad.pitch = M_PI_2;
      result_rad.roll = 0.0;
      result_rad.yaw = -2.0 * std::atan2(x_, w_);
    } else if (sinp <= (-1.0 + 1e-8)) {
      result_rad.pitch = -M_PI_2;
      result_rad.roll = 0.0;
      result_rad.yaw = 2.0 * std::atan2(x_, w_);
    } else {
      result_rad.pitch = std::asin(sinp);

      const double sinr_cosp = 2.0 * (w_ * x_ + y_ * z_);
      const double cosr_cosp = 1.0 - 2.0 * (x_ * x_ + y_ * y_);
      result_rad.roll = std::atan2(sinr_cosp, cosr_cosp);

      const double siny_cosp = 2.0 * (w_ * z_ + x_ * y_);
      const double cosy_cosp = 1.0 - 2.0 * (y_ * y_ + z_ * z_);
      result_rad.yaw = std::atan2(siny_cosp, cosy_cosp);
    }

    return result_rad;
  }

  static Quaternion FromEuler(
      double roll_rad, double pitch_rad, double yaw_rad) {
    // Quaternions multiply in opposite order, and we want to get into
    // roll, pitch, then yaw as standard.
    return (Quaternion::FromAxisAngle(yaw_rad, 0, 0, 1) *
            Quaternion::FromAxisAngle(pitch_rad, 0, 1, 0) *
            Quaternion::FromAxisAngle(roll_rad, 1, 0, 0));
  }

  static Quaternion FromEuler(Euler euler_rad) {
    return FromEuler(euler_rad.roll, euler_rad.pitch, euler_rad.yaw);
  }

  static Quaternion FromAxisAngle(
      double angle_rad, double x, double y, double z) {
    double c = std::cos(angle_rad / 2.0);
    double s = std::sin(angle_rad / 2.0);

    return Quaternion(c, x * s, y * s, z * s);
  }

  static Quaternion FromAxisAngle(
      double angle_rad, const base::Point3D& axis) {
    return FromAxisAngle(angle_rad, axis.x(), axis.y(), axis.z());
  }

  struct AxisAngle {
    double angle_rad = 0;
    base::Point3D axis;

    base::Point3D magnitude_vector() const {
      return angle_rad * axis;
    }
  };

  static Quaternion FromAxisAngle(const AxisAngle& axis_angle) {
    return FromAxisAngle(
        axis_angle.angle_rad,
        axis_angle.axis.x(),
        axis_angle.axis.y(),
        axis_angle.axis.z());
  }

  AxisAngle axis_angle() const {
    AxisAngle result;
    result.angle_rad = 2.0 * std::acos(w_);
    const double denom = std::sqrt(1.0 - w_ * w_);
    result.axis.x() = x_ / denom;
    result.axis.y() = y_ / denom;
    result.axis.z() = z_ / denom;
    return result;
  }

  static Quaternion IntegrateRotationRate(
      const Point3D& rate_rps,
      double dt_s) {
    // This simple technique will yield terrible results if the total
    // delta is too large.
    const double kMaxIntegrationAngle = 0.5;

    BOOST_VERIFY(rate_rps.x() * dt_s < kMaxIntegrationAngle);
    BOOST_VERIFY(rate_rps.y() * dt_s < kMaxIntegrationAngle);
    BOOST_VERIFY(rate_rps.z() * dt_s < kMaxIntegrationAngle);

    return Quaternion(1.0,
                      0.5 * rate_rps.x() * dt_s,
                      0.5 * rate_rps.y() * dt_s,
                      0.5 * rate_rps.z() * dt_s).normalized();
  }

  double w() const { return w_; }
  double x() const { return x_; }
  double y() const { return y_; }
  double z() const { return z_; }

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(mjlib::base::MakeNameValuePair(&w_, "w"));
    a->Visit(mjlib::base::MakeNameValuePair(&x_, "x"));
    a->Visit(mjlib::base::MakeNameValuePair(&y_, "y"));
    a->Visit(mjlib::base::MakeNameValuePair(&z_, "z"));
  }

 private:
  double w_;
  double x_;
  double y_;
  double z_;
};

inline Quaternion operator*(const Quaternion& lhs,
                            const Quaternion& rhs) {
  double a = lhs.w();
  double b = lhs.x();
  double c = lhs.y();
  double d = lhs.z();

  double e = rhs.w();
  double f = rhs.x();
  double g = rhs.y();
  double h = rhs.z();

  return Quaternion(a * e - b * f - c * g - d * h,
                    b * e + a * f + c * h - d * g,
                    a * g - b * h + c * e + d * f,
                    a * h + b * g - c * f + d * e);
}
}
}
