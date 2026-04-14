cmake_minimum_required(VERSION 3.20)
project(skpviz C CXX)
if(APPLE)
    enable_language(OBJC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Must be set before any targets so all libraries link the same CRT as Skia.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# SDL3: windowing, events, clipboard, file dialogs.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
# Disable subsystems we don't use to keep the build small.
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL)

# ImGui (compiled from source via submodule), SDL3 + OpenGL3 backends.
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC third_party/imgui third_party/imgui/backends)
target_link_libraries(imgui PRIVATE SDL3::SDL3-static)

# Skia (built from source via submodule).
set(SKIA_DIR "${CMAKE_SOURCE_DIR}/third_party/skia" CACHE PATH "Path to Skia source tree")
set(SKIA_OUT "${SKIA_DIR}/out/Release" CACHE PATH "Path to Skia build output")
add_library(skia STATIC IMPORTED)
if(WIN32)
    set_target_properties(skia PROPERTIES IMPORTED_LOCATION ${SKIA_OUT}/skia.lib)
else()
    set_target_properties(skia PROPERTIES IMPORTED_LOCATION ${SKIA_OUT}/libskia.a)
endif()
target_include_directories(skia INTERFACE ${SKIA_DIR})

# App
if(MSVC)
    add_compile_options(/GR-)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-fno-rtti)
endif()

add_executable(skpviz main.cpp skia_backend.cpp gl_extras.cpp)
target_link_libraries(skpviz PRIVATE imgui SDL3::SDL3-static skia)

if(APPLE)
    target_link_libraries(skpviz PRIVATE
        "-framework OpenGL"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework Metal"
        "-framework Foundation"
        "-framework ImageIO"
        "-framework ApplicationServices"
    )
elseif(WIN32)
    target_link_libraries(skpviz PRIVATE
        opengl32 gdi32 user32 shell32 ole32 uuid
        fontsub usp10 dwrite
    )
else()
    target_link_libraries(skpviz PRIVATE
        GL pthread dl m
        fontconfig freetype
    )
endif()

# Test SKP generators
add_executable(make-test-skp make_test_skp.cpp)
add_executable(make-test-mskp make_test_mskp.cpp)
add_executable(make-test-layers-mskp make_test_layers_mskp.cpp)
target_link_libraries(make-test-skp PRIVATE skia)
target_link_libraries(make-test-mskp PRIVATE skia)
target_link_libraries(make-test-layers-mskp PRIVATE skia)

if(APPLE)
    set(TESTSKP_FRAMEWORKS
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework Metal"
        "-framework Foundation"
        "-framework ImageIO"
        "-framework ApplicationServices"
    )
    target_link_libraries(make-test-skp PRIVATE ${TESTSKP_FRAMEWORKS})
    target_link_libraries(make-test-mskp PRIVATE ${TESTSKP_FRAMEWORKS})
    target_link_libraries(make-test-layers-mskp PRIVATE ${TESTSKP_FRAMEWORKS})
elseif(WIN32)
    target_link_libraries(make-test-skp PRIVATE gdi32 user32 ole32 uuid fontsub usp10 dwrite)
    target_link_libraries(make-test-mskp PRIVATE gdi32 user32 ole32 uuid fontsub usp10 dwrite)
    target_link_libraries(make-test-layers-mskp PRIVATE gdi32 user32 ole32 uuid fontsub usp10 dwrite)
else()
    target_link_libraries(make-test-skp PRIVATE pthread dl m fontconfig freetype)
    target_link_libraries(make-test-mskp PRIVATE pthread dl m fontconfig freetype)
    target_link_libraries(make-test-layers-mskp PRIVATE pthread dl m fontconfig freetype)
endif()

add_custom_target(skp
    COMMAND make-test-skp ${CMAKE_BINARY_DIR}/test.skp
    DEPENDS make-test-skp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run
    COMMAND skpviz ${CMAKE_BINARY_DIR}/test.skp
    DEPENDS skpviz skp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
