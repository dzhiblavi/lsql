execute_process(
    COMMAND ${CLI} ${QUERY}
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE error_output
    RESULT_VARIABLE exit_code
)

if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "CLI failed with exit code ${exit_code}\nError: ${error_output}")
endif()

file(READ ${EXPECTED} expected_output)

function(normalize_output var output)
    string(REGEX REPLACE "\r\n" "\n" output "${output}")
    string(REGEX REPLACE "\r" "\n" output "${output}")
    string(REGEX REPLACE "[ \t]+$" "" output "${output}")
    string(REGEX REPLACE "\n+" "\n" output "${output}")
    string(REGEX REPLACE "^\n+" "" output "${output}")
    string(REGEX REPLACE "\n+$" "" output "${output}")
    string(REGEX REPLACE "\t+$" "" output "${output}")
    set(${var} "${output}" PARENT_SCOPE)
endfunction()

normalize_output(actual_normalized "${actual_output}")
normalize_output(expected_normalized "${expected_output}")

if(NOT actual_normalized STREQUAL expected_normalized)
    message("Expected output:\n${expected_normalized}")
    message("Actual output:\n${actual_normalized}")
    message(FATAL_ERROR "Output mismatch")
endif()

message("✓ Test passed - output matches expected")
