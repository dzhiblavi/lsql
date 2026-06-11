# arguments: CLI, QUERY_FILE, EXPECTED_ERROR_FILE

if(NOT EXISTS ${QUERY_FILE})
    message(FATAL_ERROR "Query file not found: ${QUERY_FILE}")
endif()

if(NOT EXISTS ${EXPECTED_ERROR_FILE})
    message(FATAL_ERROR "Expected error file not found: ${EXPECTED_ERROR_FILE}")
endif()

execute_process(
    COMMAND ${CLI} ${QUERY_FILE} -f JSON
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE error_output
    RESULT_VARIABLE exit_code
)

if(exit_code EQUAL 0)
    message(
        FATAL_ERROR
        "Query succeeded but failure was expected.\nstdout:\n${actual_output}\nstderr:\n${error_output}"
    )
endif()

set(command_output "${actual_output}\n${error_output}")

file(STRINGS ${EXPECTED_ERROR_FILE} expected_lines)
foreach(line IN LISTS expected_lines)
    string(STRIP "${line}" expected)
    if(expected STREQUAL "" OR expected MATCHES "^#")
        continue()
    endif()

    string(REGEX REPLACE "^contains:[ ]*" "" expected "${expected}")
    string(FIND "${command_output}" "${expected}" found_at)

    if(found_at EQUAL -1)
        message(
            FATAL_ERROR
            "Expected command output to contain '${expected}'.\nstdout:\n${actual_output}\nstderr:\n${error_output}"
        )
    endif()
endforeach()

message("✓ Test passed - command failed with expected output")
