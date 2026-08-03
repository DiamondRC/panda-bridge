# A curated warning wall, shared via the lqr::warnings interface target.
# Link it into every target we own (never into third-party code).
add_library(lqr_warnings INTERFACE)
add_library(lqr::warnings ALIAS lqr_warnings)

option(LQR_WERROR "Treat warnings as errors" ON)

set(_common
    -Wall -Wextra -Wpedantic
    -Wshadow                 # a local shadows something in an outer scope
    -Wconversion             # implicit conversions that may change a value
    -Wsign-conversion        # implicit signed <-> unsigned
    -Wcast-align             # a cast increases required alignment
    -Wold-style-cast         # C casts in C++ (use static_cast/bit_cast)
    -Wnull-dereference
    -Wdouble-promotion       # silent float -> double
    -Wformat=2
    -Wimplicit-fallthrough
    -Woverloaded-virtual
    -Wnon-virtual-dtor)

target_compile_options(lqr_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:${_common}>
    $<$<AND:$<BOOL:${LQR_WERROR}>,$<CXX_COMPILER_ID:GNU,Clang>>:-Werror>)
