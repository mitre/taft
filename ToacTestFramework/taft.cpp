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

#include <log4cplus/initializer.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <iostream>

#include "framework/Configuration.h"
#include "framework/TestFrameworkScenario.h"
#include "public/Log4cplusSetup.h"

#ifndef TAFT_VERSION
#define TAFT_VERSION "unknown"
#endif

int main(int argc, char *argv[]) {
   log4cplus::Initializer initializer;
   mitre::oss::simcore::LoadLoggerProperties();
   const auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
   LOG4CPLUS_INFO(logger, "running " << TAFT_VERSION);
   if (argc != 2) {
      LOG4CPLUS_FATAL(logger, "Provide exactly one argument: --version or a run YAML file.");
      return EXIT_FAILURE;
   }
   if (std::string(argv[1]) == "--version") {
      std::cout << "TAFT version " << TAFT_VERSION << std::endl;
      return EXIT_SUCCESS;
   }
   try {
      const auto configurations = mitre::oss::taft::LoadRunScenarios(argv[1]);
      for (const auto &configuration : configurations) {
         LOG4CPLUS_INFO(logger, "Processing Scenario File: " << configuration.source_file.string());
         mitre::oss::taft::TestFrameworkScenario scenario(configuration);
         scenario.SimulateAllIterations();
      }
   } catch (const std::exception &error) {
      LOG4CPLUS_FATAL(logger, error.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
