# https://github.com/horance-liu/flink.cmake

include(CMakeParseArguments)

function(target_do_force_link_libraries target visibility lib)
  if(MSVC)
    target_link_libraries(${target} ${visibility} "/WHOLEplatform::ArcHIVE:${lib}")
  elseif(APPLE)
    target_link_libraries(${target} ${visibility} "-Wl,-force_load" ${lib})
  else()
    target_link_libraries(${target} ${visibility} "-Wl,--whole-platform::Archive" ${lib}
                          "-Wl,--no-whole-platform::Archive")
  endif()
endfunction()

function(target_force_link_libraries target)
  cmake_parse_arguments(FLINK "" "" "PUBLIC;INTERFACE;PRIVATE" ${ARGN})

  foreach(lib IN LISTS FLINK_PUBLIC)
    target_do_force_link_libraries(${target} PUBLIC ${lib})
  endforeach()

  foreach(lib IN LISTS FLINK_INTERFACE)
    target_do_force_link_libraries(${target} INTERFACE ${lib})
  endforeach()

  foreach(lib IN LISTS FLINK_PRIVATE)
    target_do_force_link_libraries(${target} PRIVATE ${lib})
  endforeach()
endfunction()
