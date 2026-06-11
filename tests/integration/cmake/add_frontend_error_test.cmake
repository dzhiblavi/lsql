function(add_frontend_error_test TEST_DIR FRONTEND)
    set(TEST_DIR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}")
    set(TEST_DIR_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_DIR}")

    if("${FRONTEND}" STREQUAL "sql")
        set(CLI_TARGET lsql)
        set(QUERY_FILE_NAME query.sql)
    elseif("${FRONTEND}" STREQUAL "pipe")
        set(CLI_TARGET lpipe)
        set(QUERY_FILE_NAME query.pipe)
    else()
        message(FATAL_ERROR "Unknown frontend '${FRONTEND}' for ${TEST_DIR}")
    endif()

    set(QUERY_FILE_SOURCE "${TEST_DIR_SOURCE}/${QUERY_FILE_NAME}")
    if(NOT EXISTS "${QUERY_FILE_SOURCE}")
        message(FATAL_ERROR "Query file not found for ${TEST_DIR}: ${QUERY_FILE_SOURCE}")
    endif()

    set(TEST_NAME "${TEST_DIR}_${FRONTEND}")
    set(EXPECTED_ERROR_FILE_SOURCE "${TEST_DIR_SOURCE}/error.${FRONTEND}.txt")

    if(NOT EXISTS "${EXPECTED_ERROR_FILE_SOURCE}")
        message(FATAL_ERROR "Expected error file not found for ${TEST_NAME}")
    endif()

    file(COPY "${TEST_DIR_SOURCE}/" DESTINATION "${TEST_DIR_BINARY}")

    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" SAFE_TEST_NAME "${TEST_NAME}")

    add_test(
        NAME ${SAFE_TEST_NAME}
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:${CLI_TARGET}>
            -DQUERY_FILE=${TEST_DIR_BINARY}/${QUERY_FILE_NAME}
            -DEXPECTED_ERROR_FILE=${TEST_DIR_BINARY}/error.${FRONTEND}.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_error_test.cmake
    )

    set_tests_properties(
        ${SAFE_TEST_NAME}
        PROPERTIES
        TIMEOUT 10
        WORKING_DIRECTORY "${TEST_DIR_BINARY}"
    )
endfunction()

function(add_error_test TEST_DIR)
    add_frontend_error_test(${TEST_DIR} sql)
    add_frontend_error_test(${TEST_DIR} pipe)
endfunction()
