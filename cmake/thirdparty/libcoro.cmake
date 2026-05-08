include(FetchContent)

set(LIBCORO_BUILD_TESTS OFF)
set(LIBCORO_BUILD_EXAMPLES OFF)
set(LIBCORO_FEATURE_TLS OFF)
set(LIBCORO_FEATURE_NETWORKING OFF)

FetchContent_Declare(
    libcoro
    GIT_REPOSITORY https://github.com/jbaldwin/libcoro.git
    GIT_TAG v0.16.0
    GIT_PROGRESS TRUE
)
FetchContent_GetProperties(libcoro)
FetchContent_MakeAvailable(libcoro)

# Set library's include directory as SYSTEM so it does not raise unneeded
# warnings
set_target_properties(
    libcoro
    PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    $<TARGET_PROPERTY:libcoro,INTERFACE_INCLUDE_DIRECTORIES>)
