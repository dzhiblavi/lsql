set(LEXER_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l)
set(LEXER_OUTPUT_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)
set(LEXER_OUTPUT_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.h)

set_source_files_properties(
    ${LEXER_OUTPUT_SOURCE_FILE}
    ${LEXER_OUTPUT_HEADER_FILE}
    PROPERTIES GENERATED TRUE
)

add_custom_command(
    OUTPUT ${LEXER_OUTPUT_SOURCE_FILE} ${LEXER_OUTPUT_HEADER_FILE}  # ← List BOTH outputs
    COMMAND reflex --flex
        --header-file=${LEXER_OUTPUT_HEADER_FILE}
        --outfile=${LEXER_OUTPUT_SOURCE_FILE}
        ${LEXER_SOURCE_FILE}
    DEPENDS ${LEXER_SOURCE_FILE}
    COMMENT "Generating lexer from ${LEXER_SOURCE_FILE}"
    VERBATIM
)

# Create a custom target that will trigger the custom command
# The target doesn't depend on the outputs; it's just a name
add_custom_target(lsql-front-pipe-parser-lexer-generate
    DEPENDS ${LEXER_OUTPUT_SOURCE_FILE}
)

add_library(lsql-front-pipe-parser-lexer STATIC)
target_link_libraries(
    lsql-front-pipe-parser-lexer PUBLIC lsql-front-pipe-parser-grammar ReflexLibStatic)
target_sources(lsql-front-pipe-parser-lexer PRIVATE ${LEXER_OUTPUT_SOURCE_FILE})
target_include_directories(lsql-front-pipe-parser-lexer INTERFACE ${CMAKE_BINARY_DIR}/src)

# Suppress warnings in generated code
target_compile_options(lsql-front-pipe-parser-lexer PRIVATE -w)

add_dependencies(lsql-front-pipe-parser-lexer-generate lsql-front-pipe-parser-grammar-generate)
