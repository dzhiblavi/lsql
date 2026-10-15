include(./cmake/add_sql_test.cmake)

function(add_integration_test TEST_DIR)
    add_sql_test(${TEST_DIR})
endfunction()
