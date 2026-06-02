add_subdirectory(integration)

add_unit_test(exec/op/TestMergeSorted.cpp lsql-back-exec)
add_unit_test(exec/op/TestUnionAll.cpp lsql-back-exec)
add_unit_test(exec/op/TestLimit.cpp lsql-back-exec)
