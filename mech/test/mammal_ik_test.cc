// Copyright 2019 Josh Pieper, jjp@pobox.com.
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

#include "mech/mammal_ik.h"

#include <boost/test/auto_unit_test.hpp>

#include <fmt/format.h>

#include "mjlib/base/fail.h"

#include "base/common.h"

namespace base = mjmech::base;
using namespace mjmech::mech;

namespace {
class Fixture {
 public:

  MammalIk dut{[]() {
      MammalIk::Config config;

      config.shoulder.pose = {0.020, 0.0, 0.0};
      config.shoulder.id = 1;
      config.femur.pose = {0.0, 0.0, 0.100};
      config.femur.id = 2;
      config.tibia.pose = {0.0, 0.0, 0.100};
      config.tibia.id = 3;

      return config;
    }()};

  using J = IkSolver::Joint;
};

IkSolver::Joint GetJoint(const IkSolver::JointAngles& joints, int id) {
  for (const auto& joint : joints) {
    if (joint.id == id) { return joint; }
  }
  mjlib::base::Fail("joint not found");
}

IkSolver::Joint Shoulder(const IkSolver::JointAngles& joints) {
  return GetJoint(joints, 1);
}

IkSolver::Joint Femur(const IkSolver::JointAngles& joints) {
  return GetJoint(joints, 2);
}

IkSolver::Joint Tibia(const IkSolver::JointAngles& joints) {
  return GetJoint(joints, 3);
}

class OffsetFixture {
 public:

  MammalIk dut{[]() {
      MammalIk::Config config;

      config.shoulder.pose = {0.020, 0.030, 0.0};
      config.shoulder.id = 1;
      config.femur.pose = {0.0, 0.0, 0.100};
      config.femur.id = 2;
      config.tibia.pose = {0.0, 0.0, 0.100};
      config.tibia.id = 3;

      return config;
    }()};

  using J = IkSolver::Joint;
};

struct TorqueTest {
  double x;
  double y;
  double z;

  // double x_m_s;
  // double y_m_s;
  // double z_m_s;

  double x_N;
  double y_N;
  double z_N;

  double expected_shoulder_deg;
  double expected_femur_deg;
  double expected_tibia_deg;

  // double expected_shoulder_dps;
  // double expected_femur_dps;
  // double expected_tibia_dps;

  double expected_shoulder_Nm;
  double expected_femur_Nm;
  double expected_tibia_Nm;
};

}


BOOST_FIXTURE_TEST_CASE(MammalPoseTest, Fixture) {
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2),
        J().set_id(3)});
    BOOST_TEST(result_G.pose == Eigen::Vector3d(0.020, 0, 0.200));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

  // Move the shoulder joint.
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1).set_angle_deg(10),
        J().set_id(2),
        J().set_id(3)});
    BOOST_TEST(result_G.pose.isApprox(
                   Eigen::Vector3d(0.020,
                                   std::sin(base::Radians(-10)) * 0.200,
                                   std::cos(base::Radians(-10)) * 0.200)));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

  // Move the femur joint.
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2).set_angle_deg(10),
        J().set_id(3)});
    BOOST_TEST(result_G.pose.isApprox(
                   Eigen::Vector3d(0.020 + std::sin(base::Radians(10)) * 0.200,
                                   0,
                                   std::cos(base::Radians(10)) * 0.200)));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

  // Move the tibia joint.
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2),
        J().set_id(3).set_angle_deg(10)});
    BOOST_TEST(result_G.pose.isApprox(
                   Eigen::Vector3d(0.020 + std::sin(base::Radians(10)) * 0.100,
                                   0,
                                   0.100 + std::cos(base::Radians(10)) * 0.100)));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

}

BOOST_FIXTURE_TEST_CASE(MammalVelocityTest, Fixture) {
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1).set_velocity_dps(10.0),
        J().set_id(2),
        J().set_id(3)});
    BOOST_TEST(result_G.pose == Eigen::Vector3d(0.020, 0, 0.200));
    BOOST_TEST(result_G.velocity.isApprox(
                   Eigen::Vector3d(0, base::Radians(-10) * 0.200, 0)));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2).set_velocity_dps(10.0),
        J().set_id(3)});
    BOOST_TEST(result_G.pose == Eigen::Vector3d(0.020, 0, 0.200));
    BOOST_TEST(result_G.velocity.isApprox(
                   Eigen::Vector3d(base::Radians(10) * 0.200, 0, 0)));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }

  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2),
        J().set_id(3).set_velocity_dps(10.0)});
    BOOST_TEST(result_G.pose == Eigen::Vector3d(0.020, 0, 0.200));
    BOOST_TEST(result_G.velocity.isApprox(
                   Eigen::Vector3d(base::Radians(10) * 0.100, 0, 0)));
    BOOST_TEST(result_G.force_N == Eigen::Vector3d(0, 0, 0));
  }
}

BOOST_FIXTURE_TEST_CASE(MammalForceTest, Fixture) {
  {
    const auto result_G = dut.Forward_G({
        J().set_id(1).set_torque_Nm(1.0),
        J().set_id(2),
        J().set_id(3)});
    BOOST_TEST(result_G.pose == Eigen::Vector3d(0.020, 0, 0.200));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N.isApprox(
                   Eigen::Vector3d(0, -5, 0), 1e-3));
  }

  {
    const auto result_G = dut.Forward_G({
        J().set_id(1),
        J().set_id(2).set_angle_deg(30),
        J().set_id(3).set_angle_deg(-60).set_torque_Nm(-2)});
    BOOST_TEST(result_G.pose.isApprox(Eigen::Vector3d(0.020, 0, 0.173205),
                                         1e-3));
    BOOST_TEST(result_G.velocity == Eigen::Vector3d(0, 0, 0));
    BOOST_TEST(result_G.force_N.isApprox(
                   Eigen::Vector3d(0, 0, -39.98), 1e-3));
  }
}

BOOST_AUTO_TEST_CASE(MammalInverseShoulderTest,
                     * boost::unit_test::tolerance(1e-2)) {
  struct Test {
    double shoulder_y;
    double shoulder_z;
    double point_y;
    double point_z;

    double expected_angle_deg;
  };

  Test tests[] = {
    {0, 0, 0, 0.200, 0.0},
    {0, 0, 0.010, 0.180, -3.18},
    {0, 0, -0.010, 0.180, 3.18},
    {0.010, 0, 0.010, 0.180, 0.0},
    {0.010, 0, 0, 0.180, 3.18},
    {-0.010, 0, 0, 0.180, -3.18},

    {0.030, 0, 0.040, 0.180, -3.165},

  };

  for (auto test : tests) {
    // The Z offset should have no effect on shoulder angle.
    for (double z : {0.0, 0.010}) {
      test.shoulder_z = z;

      BOOST_TEST_CONTEXT(
          fmt::format("sy={} sz={} y={} z={}",
                      test.shoulder_y, test.shoulder_z,
                      test.point_y, test.point_z)) {
        MammalIk dut{[&]() {
            MammalIk::Config config;

            config.shoulder.pose = {0.0, test.shoulder_y, test.shoulder_z};
            config.shoulder.id = 1;
            config.femur.pose = {0.0, 0.0, 0.100};
            config.femur.id = 2;
            config.tibia.pose = {0.0, 0.0, 0.100};
            config.tibia.id = 3;

            return config;
          }()};

        IkSolver::Effector input_G;
        input_G.pose = Eigen::Vector3d(0, test.point_y, test.point_z);
        const auto result = dut.Inverse(input_G, {});
        BOOST_TEST_REQUIRE(!!result);
        BOOST_TEST(Shoulder(*result).angle_deg == test.expected_angle_deg);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(MammalInverseLowerTest,
                     * boost::unit_test::tolerance(1e-2)) {
  // This tests the lower leg logic in the absence of any shoulder
  // rotation.
  struct Test {
    double point_x;
    double point_z;
    bool invert;

    double expected_femur_deg;
    double expected_tibia_deg;
  };

  Test tests[] = {
    {  0, 0.200, false,   0, 0 },
    {  0, 0.200, true,    0, 0 },

    { 0.00195,   0.19499, false,   13.41, -25.51 },
    { 0.00975,   0.19476, false,   15.70, -25.51 },
    { 0.01947,   0.19403, false,   18.56, -25.51 },
    { 0.16409,   0.10536, false,   70.13, -25.51 },
    { 0.19451,   0.01379, false,   98.79, -25.51 },

    { 0.00195,   0.19499, true,   -12.26,  25.51 },
    { 0.00975,   0.19476, true,    -9.97,  25.51 },
    { 0.01947,   0.19403, true,    -7.10,  25.51 },
    { 0.16409,   0.10536, true,   44.46,  25.51 },
    { 0.19451,   0.01379, true,   73.10,  25.51 },
  };

  for (const auto& test : tests) {
    BOOST_TEST_CONTEXT(
        fmt::format("x={} z={} i={}",
                    test.point_x, test.point_z, test.invert)) {
      MammalIk dut{[&]() {
          MammalIk::Config config;

          config.shoulder.pose = {0.0, 0.0, 0.0};
          config.shoulder.id = 1;
          config.femur.pose = {0.0, 0.0, 0.100};
          config.femur.id = 2;
          config.tibia.pose = {0.0, 0.0, 0.100};
          config.tibia.id = 3;
          config.invert = test.invert;

          return config;
        }()};

      IkSolver::Effector input_G;
      input_G.pose = Eigen::Vector3d(test.point_x, 0.0, test.point_z);
      const auto result = dut.Inverse(input_G, {});
      BOOST_TEST_REQUIRE(!!result);
      BOOST_TEST(Femur(*result).angle_deg == test.expected_femur_deg);
      BOOST_TEST(Tibia(*result).angle_deg == test.expected_tibia_deg);
    }
  }
}

BOOST_AUTO_TEST_CASE(OldMammalTest, * boost::unit_test::tolerance(1e-2)) {
  MammalIk dut{[]() {
      MammalIk::Config config;

      config.shoulder.pose = {0.0, 0.030, 0.040};
      config.shoulder.id = 1;
      config.femur.pose = {0.0, 0.0, 0.100};
      config.femur.id = 2;
      config.tibia.pose = {0.0, 0.0, 0.110};
      config.tibia.id = 3;

      return config;
    }()};

  struct Test {
    double x;
    double y;
    double z;

    double expected_shoulder_deg;
    double expected_femur_deg;
    double expected_tibia_deg;
  };

  Test tests[] = {
    {      0, 0.030, 0.250,   0.00,  0.00,   0.00 },

    {      0, 0.030, 0.240,   0.00,  18.65, -35.55 },

    {      0, 0.030, 0.230,   0.00,  26.52, -50.48 },
    {      0, 0.030, 0.210,   0.00,  37.97, -72.00 },
    {      0, 0.030, 0.190,   0.00,  47.16, -88.96 },
    {      0, 0.030, 0.150,   0.00,  62.96, -117.04 },
    {      0, 0.030, 0.090,   0.00,  87.71, -152.98 },
    {  0.020, 0.030, 0.190,   0.00,  54.18, -87.92  },

    { -0.020, 0.030, 0.190,   0.00,  38.99, -87.92  },

    {  0.210, 0.030, 0.040,   0.00,  90, 0  },
    { -0.210, 0.030, 0.040,   0.00, -90, 0  },
    {  0.200, 0.030, 0.040,   0.00,  108.65, -35.55 },
    {      0, 0.040, 0.190,  -3.00,  46.37, -87.52  },
    {      0, 0.020, 0.190,   3.02,  47.72, -89.98 },
  };

  for (const auto& test: tests) {
    BOOST_TEST_CONTEXT(fmt::format("x={} y={} z={}",
                                   test.x, test.y, test.z)) {
      IkSolver::Effector input_G;
      input_G.pose = { test.x, test.y, test.z };
      const auto result = dut.Inverse(input_G, {});
      BOOST_TEST_REQUIRE(!!result);

      BOOST_TEST(Shoulder(*result).angle_deg == test.expected_shoulder_deg);
      BOOST_TEST(Femur(*result).angle_deg == test.expected_femur_deg);
      BOOST_TEST(Tibia(*result).angle_deg == test.expected_tibia_deg);
    }
  }
}

BOOST_AUTO_TEST_CASE(MammalInverseVelocityTest,
                     * boost::unit_test::tolerance(1e-2)) {
  MammalIk dut{[]() {
      MammalIk::Config config;

      config.shoulder.pose = {0.0, 0.0, 0.0};
      config.shoulder.id = 1;
      config.femur.pose = {0.0, 0.0, 0.100};
      config.femur.id = 2;
      config.tibia.pose = {0.0, 0.0, 0.100};
      config.tibia.id = 3;

      return config;
    }()};

  struct Test {
    double x;
    double y;
    double z;

    double vx;
    double vy;
    double vz;

    double expected_shoulder_deg;
    double expected_femur_deg;
    double expected_tibia_deg;

    double expected_shoulder_dps;
    double expected_femur_dps;
    double expected_tibia_dps;
  };

  Test tests[] = {
    { 0,  0,  195,    0,  0,  0,    0,    12.84, -25.68,     0,     0,    0 },

    { 0,  0,  195,   10,  0,  0,    0,    12.84, -25.68,     0,     2.94, 0 },
    { 0,  0,  195,    0, 10,  0,    0,    12.84, -25.68,    -2.94,  0,    0 },
    { 0,  0,  195,    0,  0, 10,    0,    12.84, -25.68,     0,   -12.89, 25.79 },

    { 0,  0,  195,  -10,  0,  0,    0,    12.84, -25.68,     0,    -2.94, 0 },
    { 0,  0,  195,    0,-10,  0,    0,    12.84, -25.68,     2.94,  0,    0 },
    { 0,  0,  195,    0,  0,-10,    0,    12.84, -25.68,     0,    12.89,-25.79 },

    { 0,  0,  170,   10,  0,  0,    0,    31.79, -63.58,     0,     3.37, 0 },
    { 0,  0,  170,    0, 10,  0,    0,    31.79, -63.58,    -3.37,  0,    0 },
    { 0,  0,  170,    0,  0, 10,    0,    31.79, -63.58,     0,    -5.44, 10.88 },

    { 0,  30, 170,   10,  0,  0,  -10.01, 30.33, -60.65,     0,     3.32,  0.0 },
    { 0,  30, 170,    0,  0, 10,  -10.01, 30.33, -60.65,     0.58, -5.59, 11.17 },

    { 20, 30, 170,   10,  0,  0,  -10.01, 36.28, -59.34,     0,     2.61,  1.33 },
    { 20, 30, 170,    0, 10,  0,  -10.01, 36.28, -59.34,    -3.27, -1.07,  2.00 },
    { 20, 30, 170,    0,  0, 10,  -10.01, 36.28, -59.34,     0.58, -6.04, 11.32 },
  };

  for (const auto& test : tests) {
    BOOST_TEST_CONTEXT(
        fmt::format(
            "x={} y={} z={} vx={} vy={} vz={}",
            test.x, test.y, test.z,
            test.vx, test.vy, test.vz)) {
      IkSolver::Effector input_G;
      input_G.pose = { test.x * 0.001, test.y * 0.001, test.z * 0.001 };
      input_G.velocity = { test.vx * 0.001, test.vy * 0.001, test.vz * 0.001 };
      const auto result = dut.Inverse(input_G, {});
      BOOST_TEST_REQUIRE(!!result);

      BOOST_TEST(Shoulder(*result).angle_deg == test.expected_shoulder_deg);
      BOOST_TEST(Femur(*result).angle_deg == test.expected_femur_deg);
      BOOST_TEST(Tibia(*result).angle_deg == test.expected_tibia_deg);
      BOOST_TEST(Shoulder(*result).velocity_dps == test.expected_shoulder_dps);
      BOOST_TEST(Femur(*result).velocity_dps == test.expected_femur_dps);
      BOOST_TEST(Tibia(*result).velocity_dps == test.expected_tibia_dps);
    }
  }
}

BOOST_AUTO_TEST_CASE(MammalInverseForceTest,
                     * boost::unit_test::tolerance(1e-2)) {
  struct Test {
    double sy;

    double x;
    double y;
    double z;

    double fx;
    double fy;
    double fz;

    double e_shoulder_deg;
    double e_femur_deg;
    double e_tibia_deg;

    double e_shoulder_Nm;
    double e_femur_Nm;
    double e_tibia_Nm;
  };

  Test tests[] = {
    { 0, 0,  0,  195,   0,  0,  0,    0,    12.84, -25.68,   0,    0,    0 },

    { 0, 0,  0,  195,   0, 10,  0,    0,    12.84, -25.68,  -1.95, 0,    0 },
    { 0, 0,  0,  195,   0,-10,  0,    0,    12.84, -25.68,   1.95, 0,    0 },

    { 0, 0,  0,  195,   0,  0, 10,    0,    12.84, -25.68,   0,    0,    0.223 },
    { 0, 0,  0,  195,   0,  0,-10,    0,    12.84, -25.68,   0,    0,   -0.223 },

    { 0, 0,  0,  195,  10,  0,  0,    0,    12.84, -25.68,   0,    1.95, 0.98 },
    { 0, 0,  0,  195, -10,  0,  0,    0,    12.84, -25.68,   0,   -1.95,-0.98 },

    { 0, 0,  0,  170,  10,  0,  0,    0,    31.79, -63.58,   0,    1.70, 0.85 },
    { 0, 0,  0,  170,   0, 10,  0,    0,    31.79, -63.58,  -1.70, 0,    0 },
    { 0, 0,  0,  170,   0,  0, 10,    0,    31.79, -63.58,   0,    0,    0.53 },

    { 0, 0, 30,  170,  10,  0,  0,  -10.01, 30.33, -60.65,   0,    1.73, 0.86 },
    { 0, 0, 30,  170,   0, 10,  0,  -10.01, 30.33, -60.65,  -1.70, 0,    0.0878 },
    { 0, 0, 30,  170,   0,  0, 10,  -10.01, 30.33, -60.65,   0.30, 0,    0.497 },

    { 0,20, 30,  170,  10,  0,  0,  -10.01, 36.28, -59.34,   0,    1.73, 0.92 },
    { 0,20, 30,  170,   0, 10,  0,  -10.01, 36.28, -59.34,  -1.70,-0.0347, 0.0681},
    { 0,20, 30,  170,   0,  0, 10,  -10.01, 36.28, -59.34,   0.30,-0.197,  0.386},

    // Directly below a shoulder with an offset.
    {30, 0, 30,  170,   0,  0,  0,    0,    31.79, -63.58,   0,    0,    0 },
    {30, 0, 30,  170,  10,  0,  0,    0,    31.79, -63.58,   0,    1.70, 0.85 },
    {30, 0, 30,  170,   0, 10,  0,    0,    31.79, -63.58,  -1.70, 0,    0 },
    {30, 0, 30,  170,   0,  0, 10,    0,    31.79, -63.58,   0.30, 0,    0.53 },
  };

  for (const auto& test : tests) {
    BOOST_TEST_CONTEXT(
        fmt::format("sy={} x={} y={} z={} fx={} fy={} fz={}",
                    test.sy, test.x, test.y, test.z,
                    test.fx, test.fy, test.fz)) {
      MammalIk dut{[&]() {
          MammalIk::Config config;

          config.shoulder.pose = {0.0, test.sy * 0.001, 0.0};
          config.shoulder.id = 1;
          config.femur.pose = {0.0, 0.0, 0.100};
          config.femur.id = 2;
          config.tibia.pose = {0.0, 0.0, 0.100};
          config.tibia.id = 3;

          return config;
        }()};

      IkSolver::Effector input_G;
      input_G.pose = { test.x * 0.001, test.y * 0.001, test.z * 0.001};
      input_G.force_N = { test.fx, test.fy, test.fz };
      const auto result = dut.Inverse(input_G, {});
      BOOST_TEST_REQUIRE(!!result);

      BOOST_TEST(Shoulder(*result).angle_deg == test.e_shoulder_deg);
      BOOST_TEST(Femur(*result).angle_deg == test.e_femur_deg);
      BOOST_TEST(Tibia(*result).angle_deg == test.e_tibia_deg);
      BOOST_TEST(Shoulder(*result).torque_Nm == test.e_shoulder_Nm);
      BOOST_TEST(Femur(*result).torque_Nm == test.e_femur_Nm);
      BOOST_TEST(Tibia(*result).torque_Nm == test.e_tibia_Nm);
    }
  }
}
