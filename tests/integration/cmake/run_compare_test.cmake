# arguments: CLI, TEST_DIR, COMPARE_SCRIPT

# check query file
set(QUERY_FILE ${TEST_DIR}/query.sql)
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
    COMMAND ${CLI} ${QUERY_FILE} -f JSON
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

#function(normalize_output var output)
    #string(REGEX REPLACE "\r\n" "\n" output "${output}")
    #string(REGEX REPLACE "\r" "\n" output "${output}")
    #string(REGEX REPLACE "[ \t]+$" "" output "${output}")
    #string(REGEX REPLACE "\n+" "\n" output "${output}")
    #string(REGEX REPLACE "^\n+" "" output "${output}")
    #string(REGEX REPLACE "\n+$" "" output "${output}")
    #string(REGEX REPLACE "\t+$" "" output "${output}")
    #set(${var} "${output}" PARENT_SCOPE)
#endfunction()

#normalize_output(actual_normalized "${actual_output}")
#normalize_output(expected_normalized "${expected_output}")

#if(NOT actual_normalized STREQUAL expected_normalized)
    #message("Expected output:\n${expected_normalized}")
    #message("Actual output:\n${actual_normalized}")
    #message(FATAL_ERROR "Output mismatch")
#endif()

message("✓ Test passed - output matches expected")
