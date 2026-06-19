add_library(lsql-optimize STATIC)

target_sources(
    lsql-optimize PRIVATE
    const_aggregate_fold.cpp
    const_scalar_fold.cpp
    relation_simplify.cpp
    scalar_simplify.cpp
    projection_collapse.cpp
    empty_relation_prune.cpp
    limit_pushdown.cpp
    optimize.cpp
)

target_link_libraries(
    lsql-optimize PUBLIC
    lsql-ir
    lsql-core
    reflectcpp
)
