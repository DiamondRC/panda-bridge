# Sanitizer selection, shared via the lqr::sanitizer interface target.
# Set by the CMake presets: LQR_SANITIZER = none | address | thread | undefined.
#
# These are our stand-in for Rust's borrow checker: ASan (memory), TSan (data
# races), UBSan (undefined behaviour). Run the test suite under each.
set(LQR_SANITIZER "none" CACHE STRING "none | address | thread | undefined")
set_property(CACHE LQR_SANITIZER PROPERTY STRINGS none address thread undefined)

add_library(lqr_sanitizer INTERFACE)
add_library(lqr::sanitizer ALIAS lqr_sanitizer)

if(NOT LQR_SANITIZER STREQUAL "none")
    set(_flags -fsanitize=${LQR_SANITIZER} -fno-omit-frame-pointer -g)
    if(LQR_SANITIZER STREQUAL "undefined")
        # Make UB a hard failure, not a warning printed then continued.
        list(APPEND _flags -fno-sanitize-recover=all)
    endif()
    target_compile_options(lqr_sanitizer INTERFACE ${_flags})
    target_link_options(lqr_sanitizer INTERFACE -fsanitize=${LQR_SANITIZER})
endif()
