add_subdirectory(${FRAMEWORK_DIR})

if(TAFT_BUILD_EXECUTABLE)
    SET(TAFT_MAIN_SRC ${FRAMEWORK_DIR}/taft.cpp)

    add_executable(TAFT ${TAFT_MAIN_SRC})
    add_executable(mitre::oss::taft ALIAS TAFT)
    target_link_libraries(TAFT taft_framework)
    target_compile_definitions(TAFT PRIVATE "TAFT_VERSION=\"${TAFT_VERSION}\"")
    target_include_directories(TAFT PUBLIC
        $<BUILD_INTERFACE:${aaesim_INCLUDE_DIRS}>
    )
    set_target_properties(TAFT PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
else()
    # Ensure taft_framework library is a build target even though nothing depends on it
    set_target_properties(taft_framework PROPERTIES EXCLUDE_FROM_ALL FALSE)
endif()
