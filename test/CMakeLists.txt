add_executable(smoke_test smoke_test.cpp)

target_link_libraries(smoke_test PRIVATE
    lqr::bridge
    doctest::doctest
    lqr::warnings
    lqr::sanitizer)

add_test(NAME smoke_test COMMAND smoke_test)
