include_directories(${CMAKE_BINARY_DIR}/src)

set(PARSER_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sql_grammar.y)
set(PARSER_OUTPUT_SOURCE_FILE_RAW ${CMAKE_CURRENT_BINARY_DIR}/sql_grammar.c)
set(PARSER_OUTPUT_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/sql_grammar.cpp)
set(PARSER_OUTPUT_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/sql_grammar.h)

# Mark as generated BEFORE the custom command
set_source_files_properties(
    ${PARSER_OUTPUT_SOURCE_FILE}
    ${PARSER_OUTPUT_HEADER_FILE}
    PROPERTIES GENERATED TRUE
)

add_custom_command(
    OUTPUT ${PARSER_OUTPUT_SOURCE_FILE} ${PARSER_OUTPUT_HEADER_FILE}
    COMMAND lemon -q -d${CMAKE_CURRENT_BINARY_DIR} ${PARSER_SOURCE_FILE} || true
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PARSER_OUTPUT_SOURCE_FILE_RAW}
        ${PARSER_OUTPUT_SOURCE_FILE}
    DEPENDS ${PARSER_SOURCE_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating parser from ${PARSER_SOURCE_FILE}"
    VERBATIM
)

# Create a custom target that will trigger the custom command
add_custom_target(lsql-front-sql-parser-grammar-generate
    DEPENDS ${PARSER_OUTPUT_SOURCE_FILE}
)

add_library(lsql-front-sql-parser-grammar STATIC)
add_dependencies(lsql-front-sql-parser-grammar lsql-front-sql-parser-grammar-generate)
target_sources(lsql-front-sql-parser-grammar PRIVATE ${PARSER_OUTPUT_SOURCE_FILE})
target_include_directories(lsql-front-sql-parser-grammar INTERFACE ${CMAKE_BINARY_DIR}/src)
target_link_libraries(lsql-front-sql-parser-grammar PUBLIC lsql-core)

# Suppress warnings in generated code
target_compile_options(lsql-front-sql-parser-grammar PRIVATE -w)
