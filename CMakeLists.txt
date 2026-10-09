cmake_minimum_required(VERSION 3.16)
project(NoteManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Build with AddressSanitizer and UBSan" OFF)
option(BUILD_TESTING "Build unit tests" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Svg LinguistTools)

# ---------------------------------------------------------------------------
# Core library: domain + storage + application services. No widgets here, so it
# can be linked into unit tests without a GUI.
# ---------------------------------------------------------------------------
add_library(notemanager_core STATIC
    src/domain/note.h
    src/domain/notevalidator.h
    src/domain/notevalidator.cpp
    src/domain/inoterepository.h
    src/domain/draft.h
    src/domain/idraftrepository.h
    src/domain/itransactionmanager.h
    src/domain/inotesearchstrategy.h
    src/domain/substringsearchstrategy.h
    src/domain/substringsearchstrategy.cpp
    src/domain/searchstrategyfactory.h
    src/domain/searchstrategyfactory.cpp
    src/domain/isettings.h
    src/domain/noteservice.h
    src/domain/noteservice.cpp
    src/domain/draftservice.h
    src/domain/draftservice.cpp
    src/storage/sqliteconnection.h
    src/storage/sqliteconnection.cpp
    src/storage/sqlitenoterepository.h
    src/storage/sqlitenoterepository.cpp
    src/storage/sqlitedraftrepository.h
    src/storage/sqlitedraftrepository.cpp
    src/storage/sqlitetransactionmanager.h
    src/storage/sqlitetransactionmanager.cpp
    src/storage/commands.h
    src/storage/commandqueue.h
    src/storage/commandqueue.cpp
    src/storage/storageworker.h
    src/storage/storageworker.cpp
    src/app/notecontroller.h
    src/app/notecontroller.cpp
    src/app/noteexporter.h
    src/app/noteexporter.cpp
    src/app/qsettingsadapter.h
    src/app/qsettingsadapter.cpp
    src/app/languagemanager.h
    src/app/languagemanager.cpp
)
target_include_directories(notemanager_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(notemanager_core PUBLIC Qt5::Core Qt5::Sql Qt5::Concurrent)

# ---------------------------------------------------------------------------
# Translations (RU). The .ts file is maintained with `cmake --build . --target update_translations`.
# ---------------------------------------------------------------------------
set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/notemanager_ru.ts)
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations DEPENDS ${QM_FILES})
configure_file(resources/i18n.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc COPYONLY)
qt5_add_resources(I18N_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc)

get_target_property(LUPDATE_EXECUTABLE Qt5::lupdate IMPORTED_LOCATION)
add_custom_target(update_translations
    COMMAND ${LUPDATE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src -ts ${TS_FILES}
    COMMENT "Updating translation sources"
    VERBATIM)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
add_executable(NoteManager
    src/main.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/notedialog.h
    src/ui/notedialog.cpp
    src/ui/notelistmodel.h
    src/ui/notelistmodel.cpp
    resources/resources.qrc
    ${I18N_RESOURCES}
)
add_dependencies(NoteManager translations)
target_link_libraries(NoteManager PRIVATE notemanager_core Qt5::Widgets Qt5::Svg)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
