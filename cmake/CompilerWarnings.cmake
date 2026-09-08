function(capy_chess_set_project_warnings target warnings_as_errors)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(warnings_as_errors)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        return()
    endif()

    target_compile_options(
        ${target}
        PRIVATE
            -Wall
            -Wextra
            -Wshadow
    )

    if(warnings_as_errors)
        target_compile_options(${target} PRIVATE -Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # The constexpr magic bitboard tables require a larger evaluation budget.
        target_compile_options(${target} PRIVATE -fconstexpr-ops-limit=100000000)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang uses a step budget rather than GCC's operation budget.
        target_compile_options(${target} PRIVATE -fconstexpr-steps=100000000)
    endif()
endfunction()
