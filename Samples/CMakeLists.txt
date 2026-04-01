# Samples CMakeLists.txt

# RTS Demo
add_executable(RTSDemo
    RTSDemo/main.cpp
)

target_include_directories(RTSDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(RTSDemo PRIVATE
    GiniEngine
)

set_target_properties(RTSDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 3D Demo
add_executable(Demo3D
    Demo3D/main.cpp
)

target_include_directories(Demo3D PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(Demo3D PRIVATE
    GiniEngine
)

set_target_properties(Demo3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
