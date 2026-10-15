function(add_hardening_test SCENARIO QUERY PROFILE SEED)
    get_filename_component(QUERY_NAME "${QUERY}" NAME_WE)
    set(TEST_NAME "integration/hardening/test_${SCENARIO}_${QUERY_NAME}_${PROFILE}_seed${SEED}_sql")
    set(TEST_DIR_BINARY "${CMAKE_CURRENT_BINARY_DIR}/hardening/${SCENARIO}/${QUERY_NAME}/${PROFILE}_seed${SEED}")

    file(MAKE_DIRECTORY "${TEST_DIR_BINARY}")

    add_test(
        NAME ${TEST_NAME}
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/hardening/run.py"
            --cli $<TARGET_FILE:lsql>
            --query "${CMAKE_CURRENT_SOURCE_DIR}/hardening/test_${SCENARIO}/${QUERY}"
            --profile ${PROFILE}
            --seed ${SEED}
            ${ARGN}
    )

    set_tests_properties(
        ${TEST_NAME}
        PROPERTIES
        TIMEOUT 45
        LABELS hardening
        RUN_SERIAL TRUE
        WORKING_DIRECTORY "${TEST_DIR_BINARY}"
    )
endfunction()
