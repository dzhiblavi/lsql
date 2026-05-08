# Common compile options for C++

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")

# https://clang.llvm.org/docs/DiagnosticsReference.html
add_compile_options(-Wall -Wextra -Wpedantic -fno-omit-frame-pointer)

# Turn warnings into errors
add_compile_options(-Wno-language-extension-token)

add_compile_options(-Wno-error=unused-command-line-argument)

# libc++
if(LOGSQL_USE_CUSTOM_LIBCXX)
  message(STATUS "Using custom libc++ located at ${LOGSQL_USE_CUSTOM_LIBCXX}")

  add_compile_options(-nostdinc++ -isystem
                      "${LOGSQL_USE_CUSTOM_LIBCXX}/include/c++/v1")

  add_link_options(
    -nostdlib++ "-L${LOGSQL_USE_CUSTOM_LIBCXX}/lib/c++"
    -Wl,-rpath,"${LOGSQL_USE_CUSTOM_LIBCXX}"/lib/c++ -lc++ -lc++abi)
else()
  message(STATUS "Using system libc++")

  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
endif(LOGSQL_USE_CUSTOM_LIBCXX)

message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
