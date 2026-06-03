include(./cmake/add_sql_test.cmake)
include(./cmake/add_pipe_test.cmake)

function(add_integration_test TEST_DIR)
    add_sql_test(${TEST_DIR})
    add_pipe_test(${TEST_DIR})
endfunction()
