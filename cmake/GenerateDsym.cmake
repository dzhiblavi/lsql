function(generate_dsym target)
    if(APPLE)
        set(debug_configs "$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>")

        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND "$<${debug_configs}:dsymutil>"
                    "$<${debug_configs}:$<TARGET_FILE:${target}>>"
            COMMAND_EXPAND_LISTS
            COMMENT "Generating dSYM for ${target}"
        )
    endif()
endfunction()
