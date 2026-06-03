# arguments: CLI, TEST_DIR, COMPARE_SCRIPT, QUERY_FILE, ARGS

# check query file
if(NOT EXISTS ${QUERY_FILE})
    message(FATAL_ERROR "Query file not found: ${QUERY_FILE}")
endif()

# check expected output file
set(EXPECTED_OUTPUT_FILE ${TEST_DIR}/output.json)
if(NOT EXISTS ${EXPECTED_OUTPUT_FILE})
    message(FATAL_ERROR "Expected output file not found: ${EXPECTED_OUTPUT_FILE}")
endif()

file(READ ${EXPECTED_OUTPUT_FILE} expected_output)

# run prepare.py
set(PREPARE_SCRIPT ${TEST_DIR}/prepare.py)
if(EXISTS ${PREPARE_SCRIPT})
    execute_process(
        COMMAND python3 ${PREPARE_SCRIPT}
        RESULT_VARIABLE py_result
        OUTPUT_VARIABLE py_output
        ERROR_VARIABLE py_error
    )

    if(NOT py_result EQUAL 0)
        message(FATAL_ERROR "Python generator failed: ${py_error}: ${py_output}")
    endif()
endif()

execute_process(
    COMMAND ${CLI} ${QUERY_FILE} -f JSON -j 1
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE error_output
    RESULT_VARIABLE exit_code
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E echo "${actual_output}"
    COMMAND python3 "${COMPARE_SCRIPT}" "${EXPECTED_OUTPUT_FILE}"
    RESULT_VARIABLE COMPARE_RESULT
    ERROR_VARIABLE COMPARE_ERROR
)

if(NOT COMPARE_RESULT EQUAL 0)
    message(FATAL_ERROR "JSON comparison failed: ${COMPARE_ERROR}")
endif()

message("✓ Test passed - output matches expected")
