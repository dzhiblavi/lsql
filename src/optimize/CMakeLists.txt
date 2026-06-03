add_library(lsql-optimize STATIC)

target_sources(
    lsql-optimize PRIVATE
    aggregate_fold.cpp
    const_fold.cpp
    relation_simplify.cpp
    scalar_simplify.cpp
    projection_collapse.cpp
    empty_relation_prune.cpp
    optimize.cpp
)

target_link_libraries(
    lsql-optimize PUBLIC
    lsql-ir
    lsql-core
    reflectcpp
)
