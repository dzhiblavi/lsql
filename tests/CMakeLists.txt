add_subdirectory(integration)

add_unit_test(optimize/TestAggregateFold.cpp lsql-optimize)
add_unit_test(optimize/TestConstFold.cpp lsql-optimize)
add_unit_test(optimize/TestEmptyRelationPrune.cpp lsql-optimize)
add_unit_test(optimize/TestLimitPushdown.cpp lsql-optimize)
add_unit_test(optimize/TestProjectionCollapse.cpp lsql-optimize)
add_unit_test(optimize/TestRelationSimplify.cpp lsql-optimize)
add_unit_test(optimize/TestScalarSimplify.cpp lsql-optimize)

add_unit_test(back/exec/op/TestMergeSorted.cpp lsql-back-exec)
add_unit_test(back/exec/op/TestUnionAll.cpp lsql-back-exec)
add_unit_test(back/exec/op/TestLimit.cpp lsql-back-exec)
