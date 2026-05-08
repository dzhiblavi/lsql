option(MODULECONF_BUILD_TESTS "Build tests" ON)

if(MODULECONF_BUILD_TESTS)
    if (PROJECT_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        enable_testing()
    endif()

    function(add_test path)
        include(CTest)
        include(Catch)
        find_package(Catch2 CONFIG REQUIRED)

        string(REPLACE ".cpp" "" name ${path})
        string(REPLACE "./" "" name ${name})

        add_executable("${name}" "${path}")

        target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARGN})
        target_include_directories(${name} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>)

        catch_discover_tests(${name} DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY
                             ${CMAKE_CURRENT_SOURCE_DIR})
    endfunction(add_test)

else()
    function(add_test path)
        # nothing
    endfunction()
endif()
