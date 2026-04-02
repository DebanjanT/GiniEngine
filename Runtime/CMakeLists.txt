# Gini Runtime CMakeLists.txt

set(RUNTIME_SOURCES
    src/RuntimeLayer.cpp
    src/RuntimeApp.cpp
)

set(RUNTIME_HEADERS
    src/RuntimeLayer.h
)

add_executable(GiniRuntime ${RUNTIME_SOURCES} ${RUNTIME_HEADERS})

target_include_directories(GiniRuntime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(GiniRuntime PRIVATE
    GiniEngine
)

set_target_properties(GiniRuntime PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
