cmake_minimum_required(VERSION 3.14)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(TAFT_TEST_SOURCE
   ${UNITTEST_DIR}/src/AircraftDynamicsTestFramework/framework_tests.cpp
   ${UNITTEST_DIR}/src/AircraftDynamicsTestFramework/true_weather_tests.cpp
)
set(TAFT_TEST_OUTPUT_DIR ${PROJECT_BINARY_DIR}/unittest/bin)
add_executable(taft_test
   ${TAFT_TEST_SOURCE}
   ${UNITTEST_DIR}/src/main.cpp)
target_link_libraries(taft_test
   gtest
   taft_framework
)
target_include_directories(taft_test
    PRIVATE
    ${nlohmann_json_INCLUDE_DIR}
)
set_target_properties(taft_test PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY ${TAFT_TEST_OUTPUT_DIR}
   EXCLUDE_FROM_ALL TRUE)
add_custom_target(run_taft_test
   $<TARGET_FILE:taft_test> --gtest_output=xml:${PROJECT_BINARY_DIR}/unittest/taft_unit_test_results.xml
   DEPENDS taft_test
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/unittest/
)
