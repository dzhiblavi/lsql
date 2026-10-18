option(LSQL_BUILD_TESTS "Build tests" ON)

if(LSQL_BUILD_TESTS)
    if (PROJECT_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(BUILD_TESTING ON)
        include(CTest)
        enable_testing()
    endif()

    function(add_unit_test path)
        include(Catch)
        find_package(Catch2 CONFIG REQUIRED)

        file(RELATIVE_PATH test_path "${PROJECT_SOURCE_DIR}/tests"
             "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
        string(REGEX REPLACE "\\.cpp$" "" test_name "${test_path}")
        string(REPLACE "/" "_" target_name "${test_name}")

        add_executable("${target_name}" "${path}")

        target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain ${ARGN})
        target_include_directories(${target_name} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>)

        catch_discover_tests(${target_name} TEST_PREFIX "${test_name}/"
                             DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY
                             ${CMAKE_CURRENT_SOURCE_DIR})
    endfunction(add_unit_test)

else()
    function(add_unit_test path)
        # nothing
    endfunction()
endif()
