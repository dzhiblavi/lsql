include(./cmake/git/GetGitRevisionDescription.cmake)

get_git_head_revision(GIT_REFSPEC GIT_SHA1)

# Configure header
set(BUILD_INFO_HEADER_REL_PATH "src/util/build_info.h")
configure_file("${CMAKE_SOURCE_DIR}/${BUILD_INFO_HEADER_REL_PATH}.in"
               "${CMAKE_BINARY_DIR}/${BUILD_INFO_HEADER_REL_PATH}" @ONLY)
include_directories("${CMAKE_BINARY_DIR}/src/")

# Configure source
set(BUILD_INFO_SOURCE_REL_PATH "src/util/build_info.cpp")
string(TIMESTAMP LSQL_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%S")
configure_file("${CMAKE_SOURCE_DIR}/${BUILD_INFO_SOURCE_REL_PATH}.in"
               "${CMAKE_BINARY_DIR}/${BUILD_INFO_SOURCE_REL_PATH}" @ONLY)
