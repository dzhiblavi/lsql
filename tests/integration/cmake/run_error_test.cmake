execute_process(
    COMMAND ${CLI} ${QUERY}
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE error_output
    RESULT_VARIABLE exit_code
)

if(exit_code EQUAL 0)
    message(FATAL_ERROR "Expected CLI to fail, but it succeeded")
endif()

file(READ ${EXPECTED_ERROR} expected_error)

string(FIND "${error_output}" "${expected_error}" found)
if(found EQUAL -1)
    message("Expected error to contain:\n${expected_error}")
    message("Actual error:\n${error_output}")
    message(FATAL_ERROR "Error output mismatch")
endif()

message("✓ Test passed - correct error received")
