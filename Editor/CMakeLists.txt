# Gini Editor CMakeLists.txt

set(EDITOR_SOURCES
    EditorApp.cpp
    Panels/SceneHierarchyPanel.cpp
    Panels/PropertiesPanel.cpp
    Panels/StatsPanel.cpp
    Panels/ConsolePanel.cpp
)

set(EDITOR_HEADERS
    EditorApp.h
    Panels/EditorPanel.h
    Panels/SceneHierarchyPanel.h
    Panels/PropertiesPanel.h
    Panels/StatsPanel.h
    Panels/ConsolePanel.h
)

add_executable(GiniEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(GiniEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(GiniEditor PRIVATE
    GiniEngine
)

set_target_properties(GiniEditor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
