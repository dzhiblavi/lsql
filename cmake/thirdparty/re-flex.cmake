include(FetchContent)

FetchContent_Declare(
  reflex_cpp
  GIT_REPOSITORY https://github.com/Genivia/RE-flex
  GIT_TAG v6.1.0
  GIT_PROGRESS TRUE
  INSTALL_COMMAND "")

FetchContent_GetProperties(reflex_cpp)
FetchContent_MakeAvailable(reflex_cpp)

# Set library's include directory as SYSTEM so it does not raise unneeded
# warnings
set_target_properties(
    ReflexLib
    PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    $<TARGET_PROPERTY:ReflexLib,INTERFACE_INCLUDE_DIRECTORIES>)
