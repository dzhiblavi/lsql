add_subdirectory(integration)

add_unit_test(exec/op/TestMergeSorted.cpp lsql-exec)
add_unit_test(exec/op/TestUnionAll.cpp lsql-exec)
add_unit_test(exec/op/TestLimit.cpp lsql-exec)
