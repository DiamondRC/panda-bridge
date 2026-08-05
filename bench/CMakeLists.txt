# Check validity of benchmarking
function(tick_bench name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        lqr::bridge
        lqr::warnings
    )
endfunction()

tick_bench(bench_smoke)
tick_bench(bench_quantise)