set(NOTEMANAGER_TESTS
    tst_validator
    tst_search
    tst_repository
    tst_services
    tst_commandqueue
    tst_controller
    tst_exporter
)

foreach(test_name ${NOTEMANAGER_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE notemanager_core Qt5::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
