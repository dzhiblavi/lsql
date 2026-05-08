include(FetchContent)

set(REFLECTCPP_BSON OFF)
set(REFLECTCPP_CBOR OFF)
set(REFLECTCPP_FLEXBUFFERS OFF)
set(REFLECTCPP_MSGPACK OFF)
set(REFLECTCPP_TOML OFF)
set(REFLECTCPP_UBJSON OFF)
set(REFLECTCPP_XML OFF)
set(REFLECTCPP_YAML OFF)

FetchContent_Declare(
  reflect_cpp
  GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
  GIT_TAG v0.16.0
  GIT_PROGRESS TRUE
  INSTALL_COMMAND "")

FetchContent_GetProperties(reflect_cpp)
FetchContent_MakeAvailable(reflect_cpp)

# Set library's include directory as SYSTEM so it does not raise unneeded
# warnings
set_target_properties(
  reflectcpp
  PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
             $<TARGET_PROPERTY:reflectcpp,INTERFACE_INCLUDE_DIRECTORIES>)
