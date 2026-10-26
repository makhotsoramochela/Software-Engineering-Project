add_executable(lms_tests
    test_main.cpp
    test_rules.cpp
    test_workflow.cpp
    test_security.cpp)
target_link_libraries(lms_tests PRIVATE lms_data lms_warnings)
add_test(NAME lms_tests COMMAND lms_tests)

# Timing check (not part of ctest). Run it by hand: build/tests/lms_bench
add_executable(lms_bench bench.cpp)
target_link_libraries(lms_bench PRIVATE lms_data lms_warnings)
