# Package TAFT executable and runtime files
install(TARGETS TAFT RUNTIME DESTINATION bin COMPONENT TAFTRuntime)
install(FILES LICENSE README.md log4cplus.properties
        DESTINATION . COMPONENT TAFTRuntime)
install(DIRECTORY docs Run_Files DESTINATION . COMPONENT TAFTRuntime
        PATTERN ".gitignore" EXCLUDE PATTERN "_config.yml" EXCLUDE)

# Include notices shipped by dependencies linked into executable
foreach(dependency IN ITEMS aircraft_simulation_core fsloader cppuom log4cplus
                            geolib_idealab minicsv nlohmann_json)
    set(dependency_source "${CPM_PACKAGE_${dependency}_SOURCE_DIR}")
    if(dependency_source)
        file(GLOB dependency_notices LIST_DIRECTORIES FALSE
             "${dependency_source}/LICENSE*" "${dependency_source}/COPYING*"
             "${dependency_source}/NOTICE*")
        install(FILES ${dependency_notices}
                DESTINATION "licenses/${dependency}" COMPONENT TAFTRuntime)
    endif()
endforeach()

string(TOLOWER "${CMAKE_SYSTEM_NAME}" default_package_platform)
set(TAFT_PACKAGE_PLATFORM "${default_package_platform}" CACHE STRING "Distribution label for the binary archive")
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_NAME taft)
set(CPACK_PACKAGE_VENDOR "The MITRE Corporation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Test Automation Framework for TOAC")
set(CPACK_PACKAGE_VERSION "${TAFT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "taft-${TAFT_VERSION}-${TAFT_PACKAGE_PLATFORM}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_INSTALL_CMAKE_PROJECTS "${PROJECT_BINARY_DIR};${PROJECT_NAME};TAFTRuntime;/")
include(CPack)
