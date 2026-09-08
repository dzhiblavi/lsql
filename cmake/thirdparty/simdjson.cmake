include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.6.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(simdjson)

# Treat third-party headers as system headers so project warning policy does not
# turn warnings in the pinned simdjson release into build errors.
set_target_properties(
  simdjson
  PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
             $<TARGET_PROPERTY:simdjson,INTERFACE_INCLUDE_DIRECTORIES>)
