# LeakSanitizer suppressions for third-party (fontconfig/offscreen plugin) one-off allocations.
set(TEST_ENV "QT_QPA_PLATFORM=offscreen" "LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/lsan.supp")

set(NOTEMANAGER_CORE_TESTS
    tst_validator
    tst_search
    tst_repository
    tst_services
    tst_commandqueue
    tst_controller
    tst_exporter
)

foreach(test_name ${NOTEMANAGER_CORE_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE notemanager_core Qt5::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "${TEST_ENV}")
endforeach()

# GUI tests run with the offscreen platform plugin (no display required).
add_executable(tst_ui tst_ui.cpp)
target_link_libraries(tst_ui PRIVATE notemanager_ui Qt5::Test)
add_test(NAME tst_ui COMMAND tst_ui)
set_tests_properties(tst_ui PROPERTIES ENVIRONMENT "${TEST_ENV}")
