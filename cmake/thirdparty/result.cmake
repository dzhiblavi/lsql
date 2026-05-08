include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

FetchContent_Declare(
  result
  GIT_REPOSITORY https://github.com/dzhiblavi/result.git
  GIT_TAG main
  GIT_PROGRESS TRUE
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(result)
