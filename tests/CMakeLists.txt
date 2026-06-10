add_subdirectory(integration)

add_unit_test(back/exec/op/TestMergeSorted.cpp lsql-back-exec)
add_unit_test(back/exec/op/TestUnionAll.cpp lsql-back-exec)
add_unit_test(back/exec/op/TestLimit.cpp lsql-back-exec)
