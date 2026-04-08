# Gini Editor CMakeLists.txt

# Platform-specific file dialog implementation
if(APPLE)
    set(FILEDIALOG_SOURCE Utils/FileDialog.mm)
elseif(WIN32)
    set(FILEDIALOG_SOURCE Utils/FileDialog_Win32.cpp)
elseif(UNIX)
    set(FILEDIALOG_SOURCE Utils/FileDialog_Linux.cpp)
endif()

set(EDITOR_SOURCES
    EditorApp.cpp
    Panels/SceneHierarchyPanel.cpp
    Panels/PropertiesPanel.cpp
    Panels/StatsPanel.cpp
    Panels/ConsolePanel.cpp
    Panels/TerrainPanel.cpp
    Panels/TerrainEditorWindow.cpp
    Panels/AssetBrowserPanel.cpp
    Panels/MaterialEditorPanel.cpp
    Panels/ProjectLauncher.cpp
    Panels/ScenePropertiesPanel.cpp
    Panels/WeatherPanel.cpp
    Panels/ThreadAnalysisPanel.cpp
    Panels/ModelImportDialog.cpp
    ${FILEDIALOG_SOURCE}
)

set(EDITOR_HEADERS
    EditorApp.h
    Panels/EditorPanel.h
    Panels/SceneHierarchyPanel.h
    Panels/PropertiesPanel.h
    Panels/StatsPanel.h
    Panels/ConsolePanel.h
    Panels/TerrainPanel.h
    Panels/TerrainEditorWindow.h
    Panels/AssetBrowserPanel.h
    Panels/MaterialEditorPanel.h
    Panels/ProjectLauncher.h
    Panels/ScenePropertiesPanel.h
    Panels/WeatherPanel.h
    Panels/ThreadAnalysisPanel.h
    Panels/ModelImportDialog.h
    Utils/FileDialog.h
)

add_executable(GiniEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(GiniEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(GiniEditor PRIVATE
    GiniEngine
)

# Platform-specific link dependencies for native file dialogs
if(APPLE)
    target_link_libraries(GiniEditor PRIVATE "-framework Cocoa")
elseif(WIN32)
    target_link_libraries(GiniEditor PRIVATE comdlg32 ole32 shell32)
endif()

set_target_properties(GiniEditor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy Editor Assets (fonts, etc.)
add_custom_command(TARGET GiniEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:GiniEditor>/assets
)
