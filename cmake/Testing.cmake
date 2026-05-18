option(LOGSQL_BUILD_TESTS "Build tests" ON)

if(LOGSQL_BUILD_TESTS)
    if (PROJECT_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(BUILD_TESTING ON)
        include(CTest)
        enable_testing()
    endif()

    function(add_unit_test path)
        include(Catch)
        find_package(Catch2 CONFIG REQUIRED)

        string(REPLACE ".cpp" "" name ${path})
        string(REPLACE "/" "_" name ${name})

        add_executable("${name}" "${path}")

        target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARGN})
        target_include_directories(${name} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>)

        catch_discover_tests(${name} DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY
                             ${CMAKE_CURRENT_SOURCE_DIR})
    endfunction(add_unit_test)

else()
    function(add_unit_test path)
        # nothing
    endfunction()
endif()
