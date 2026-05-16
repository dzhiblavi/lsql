add_subdirectory(integration)

add_test(exec/op/TestMergeSorted.cpp lsql-exec)
add_test(exec/op/TestUnionAll.cpp lsql-exec)
add_test(exec/op/TestLimit.cpp lsql-exec)
