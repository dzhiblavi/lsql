function(add_pipe_test TEST_DIR)
    set(TEST_NAME "${TEST_DIR}_pipe")

    set(TEST_DIR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}")
    set(TEST_DIR_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_DIR}")
    set(COMPARE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/compare_json.py")

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:lpipe>
            -DTEST_DIR=${TEST_DIR_BINARY}
            -DCOMPARE_SCRIPT=${COMPARE_SCRIPT}
            -DQUERY_FILE=${TEST_DIR_BINARY}/query.pipe
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_compare_test.cmake
    )

    file(COPY "${TEST_DIR_SOURCE}/" DESTINATION "${TEST_DIR_BINARY}")

    set_tests_properties(
        ${TEST_NAME}
        PROPERTIES
        TIMEOUT 10
        WORKING_DIRECTORY "${TEST_DIR_BINARY}"
    )
endfunction()
