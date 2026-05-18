file(GLOB TEST_DIRS ${CMAKE_SOURCE_DIR}/tests/integration/test_*)

foreach(TEST_DIR ${TEST_DIRS})
    if(IS_DIRECTORY ${TEST_DIR})
        get_filename_component(TEST_NAME ${TEST_DIR} NAME)
        add_sql_test(${TEST_NAME})
    endif()
endforeach()
