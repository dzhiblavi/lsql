set(TARGET_FILE ${CMAKE_BINARY_DIR}/src/cli/lsql)

function(add_sql_test TEST_DIR)
    set(TEST_NAME "${TEST_DIR}_integration")
    set(QUERY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/query.sql")
    set(EXPECTED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/output.txt")
    set(EXPECTED_ERROR_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/error.txt")

    if(NOT EXISTS ${QUERY_FILE})
        message(FATAL_ERROR "Query file not found: ${QUERY_FILE}")
    endif()

    if(EXISTS ${EXPECTED_FILE})
        add_test(
            NAME ${TEST_NAME}
            COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:lsql>
                -DQUERY=${QUERY_FILE}
                -DEXPECTED=${EXPECTED_FILE}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_compare_test.cmake
        )

    elseif(EXISTS ${EXPECTED_ERROR_FILE})
        add_test(
            NAME ${TEST_NAME}
            COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:lsql>
                -DQUERY=${QUERY_FILE}
                -DEXPECTED_ERROR=${EXPECTED_ERROR_FILE}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_error_test.cmake
        )
    else()
        message(FATAL_ERROR "Error/Output file not found in ${TEST_DIR}")
    endif()

    set_tests_properties(
        ${TEST_NAME}
        PROPERTIES
        TIMEOUT 10
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}"
    )
endfunction()
