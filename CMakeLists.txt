cmake_minimum_required(VERSION 3.21)

project(WinTimeMaster VERSION 1.0.0 LANGUAGES CXX)

set(DISPLAY_VERSION "1.0.0-Alpha")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
configure_file(version.rc ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
  add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Widgets
    LinguistTools
)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(WinTimeMaster
    Main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

qt_add_translations(WinTimeMaster
    TS_FILES
        Translations/zh_CN.ts
        Translations/en_US.ts
    RESOURCE_PREFIX "/i18n"
)

qt_add_qml_module(WinTimeMaster
    URI WinTimeMaster
    QML_FILES
        UI/Settings.qml
        UI/RuleEditor.qml
        UI/LockScreen.qml
    SOURCES
        Core/LanguageManager.h
        Core/LanguageManager.cpp
        Core/SettingsController.h
        Core/SettingsController.cpp
        Core/TimeRule.h
        Core/TimeRule.cpp
        Core/TimeRuleModel.h
        Core/TimeRuleModel.cpp
        Core/TrayManager.h
        Core/TrayManager.cpp
    RESOURCES
        Resources.qrc
        SOURCES Core/LockController.h Core/LockController.cpp
)

set_target_properties(WinTimeMaster PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(WinTimeMaster PRIVATE DISPLAY_VERSION="${DISPLAY_VERSION}")

target_link_libraries(WinTimeMaster
    PRIVATE
        Qt6::Quick
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Widgets
)

install(TARGETS WinTimeMaster
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
