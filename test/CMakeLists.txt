# One doctest executable per .cpp,
# registered with CTest.
function(lqr_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        lqr::bridge
        doctest::doctest
        lqr::warnings
        lqr::sanitizer)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

lqr_add_test(transport_test)
lqr_add_test(fixed_point_test)
