cmake_minimum_required(VERSION 3.29)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(DATA_LOADER_FILES
   loaders/ConfigurationLoader.cpp
   loaders/ApplicationLoader.cpp
   loaders/SpeedCommandsLoader.cpp
   loaders/GuidanceDataLoader.cpp
   loaders/FrameworkAircraftLoader.cpp
)

set(DATA_WRITER_FILES
        writers/AircraftStateWriter.cpp
)
set(DATA_READER_FILES
        EnvReader.cpp
        RefReader.cpp
        HfpReaderPre2020.cpp
        HfpReader2020.cpp
        WaypointSequenceReader.cpp
)

set(SOURCE_FILES
        SpeedCommandsFromStaticData.cpp
        PreloadedAdsbReceiver.cpp
        TestFrameworkAircraft.cpp
        TestFrameworkScenario.cpp
        GuidanceFromStaticData.cpp
        WeatherTruthFromStaticData.cpp
        WindInterpolator.cpp
)

set(TAFT_BADA_TARGET "" CACHE STRING "Optional BADA implementation target")

include(bada_library.cmake OPTIONAL)
if("${TAFT_BADA_TARGET}" STREQUAL "" AND DEFINED BADA_LIBRARY)
   set(TAFT_BADA_TARGET "${BADA_LIBRARY}")
endif()

add_library(taft_framework STATIC ${SOURCE_FILES} ${DATA_READER_FILES} ${DATA_LOADER_FILES} ${DATA_WRITER_FILES})
add_library(mitre::oss::taft::framework ALIAS taft_framework)
set_target_properties(taft_framework PROPERTIES
   ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
)
target_include_directories(taft_framework PUBLIC
        ${aaesim_INCLUDE_DIRS})
target_link_libraries(taft_framework
   PUBLIC
   log4cplus::log4cplus
   mitre::oss::simcore
   PRIVATE yaml-cpp::yaml-cpp)
if (TAFT_BADA_TARGET)
   target_link_libraries(taft_framework PUBLIC ${TAFT_BADA_TARGET})
   target_compile_definitions(taft_framework PUBLIC MITRE_BADA3_LIBRARY)
endif()
