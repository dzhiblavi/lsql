add_subdirectory(integration)

add_unit_test(optimize/TestAggregateFold.cpp lsql-optimize)
add_unit_test(optimize/TestConstFold.cpp lsql-optimize)
add_unit_test(optimize/TestEmptyRelationPrune.cpp lsql-optimize)
add_unit_test(optimize/TestLimitPushdown.cpp lsql-optimize)
add_unit_test(optimize/TestProjectionCollapse.cpp lsql-optimize)
add_unit_test(optimize/TestRelationSimplify.cpp lsql-optimize)
add_unit_test(optimize/TestScalarSimplify.cpp lsql-optimize)

add_unit_test(back/exec/phys/ops/TestMergeSorted.cpp lsql-back-exec)
add_unit_test(back/exec/phys/ops/TestUnionAll.cpp lsql-back-exec)
add_unit_test(back/exec/phys/ops/TestLimit.cpp lsql-back-exec)
