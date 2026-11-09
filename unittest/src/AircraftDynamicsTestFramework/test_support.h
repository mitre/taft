// ****************************************************************************
// NOTICE
//
// This is the copyright work of The MITRE Corporation, and was produced for the U. S. Government under
// Contract Number 693KA8-22-C-00001, and is subject to Federal Aviation Administration Acquisition
// Management System Clause 3.5-13, Rights In Data-General (Oct. 2014), Alt. III and Alt. IV (Jan.
// 2009).

// No other use other than that granted to the U. S. Government, or to those acting on behalf of
// the U. S. Government, under that Clause is authorized without the express written permission of The
// MITRE Corporation.

// For further information, please contact The MITRE Corporation, Contracts
// Management Office, 7515 Colshire Drive, McLean, VA 22102-7539, (703) 983-6000.
//
// (c) 2026 The MITRE Corporation. All Rights Reserved.
// ****************************************************************************

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

namespace mitre::oss::taft::test {
class TemporaryDirectoryTest : public ::testing::Test {
  protected:
   void SetUp() override {
      directory = std::filesystem::temp_directory_path() /
                  ("taft-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(directory);
   }
   void TearDown() override { std::filesystem::remove_all(directory); }
   std::filesystem::path Write(const std::string &name, const std::string &text) {
      const auto path = directory / name;
      std::filesystem::create_directories(path.parent_path());
      std::ofstream stream(path);
      stream << text;
      return path;
   }
   std::filesystem::path directory;
};
}
