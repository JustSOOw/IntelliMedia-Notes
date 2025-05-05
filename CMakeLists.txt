cmake_minimum_required(VERSION 3.16)

project(IntelliMedia_Notes VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt components (Adjust if you added more modules like QuickWidgets, Sql)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets QuickWidgets Sql Svg QuickControls2)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets QuickWidgets Sql Svg Core5Compat QuickControls2)

# Tell AutoUic where to find UI files
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

# Define project sources using the new structure
set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/sidebarmanager.cpp
        src/sidebarmanager.h
        src/databasemanager.cpp
        src/databasemanager.h
        src/searchmanager.cpp
        src/searchmanager.h
        src/texteditormanager.cpp
        src/texteditormanager.h
        src/fixedwidthfontcombo.h
        src/aiassistantdialog.cpp
        src/aiassistantdialog.h
        forms/mainwindow.ui   # UI file is now in forms/
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(IntelliMedia_Notes
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources/resources.qrc # Resource file is now in resources/
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET IntelliMedia_Notes APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    # Handling for Qt 5 (Keep paths updated if needed)
    if(ANDROID)
        add_library(IntelliMedia_Notes SHARED
            ${PROJECT_SOURCES}
             resources/resources.qrc # Add resource file here too for Qt 5 library
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(IntelliMedia_Notes
            ${PROJECT_SOURCES}
            resources/resources.qrc # Add resource file here too for Qt 5 executable
        )
    endif()
endif()

# Link necessary Qt libraries (Ensure QuickWidgets and Sql are linked if used)
target_link_libraries(IntelliMedia_Notes PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::QuickWidgets 
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Sql 
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Core5Compat
)   


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.IntelliMedia_Notes)
endif()
set_target_properties(IntelliMedia_Notes PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS IntelliMedia_Notes
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(IntelliMedia_Notes)
endif()
