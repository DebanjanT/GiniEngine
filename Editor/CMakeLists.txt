# Gini Editor CMakeLists.txt

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
    Utils/FileDialog.mm
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

# Link Cocoa framework for native file dialogs on macOS
if(APPLE)
    target_link_libraries(GiniEditor PRIVATE "-framework Cocoa")
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
