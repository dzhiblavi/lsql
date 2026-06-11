include(FetchContent)

set(LLOG_BUILD_IPO OFF)
set(LLOG_BUILD_TESTS OFF)

FetchContent_Declare(
  llog-spdlog
  GIT_REPOSITORY https://github.com/dzhiblavi/llog-spdlog.git
  GIT_TAG b8cee45
  GIT_PROGRESS TRUE
  INSTALL_COMMAND "")

FetchContent_GetProperties(llog-spdlog)
FetchContent_MakeAvailable(llog-spdlog)

# Set library's include directory as SYSTEM so it does not raise unneeded
# warnings
set_target_properties(
  llog
  PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
             $<TARGET_PROPERTY:llog,INTERFACE_INCLUDE_DIRECTORIES>)
