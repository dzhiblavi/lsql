find_package(magic_enum REQUIRED)

add_library(
    lsql-ast STATIC
    BinExpression.cpp
    UnaryExpression.cpp
    UnaryAggregateExpression.cpp
)

target_link_libraries(lsql-ast PRIVATE magic_enum::magic_enum)
