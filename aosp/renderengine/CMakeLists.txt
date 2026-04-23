# RenderEngine port — AOSP interface (LayerSettings, DisplaySettings,
# RenderEngine base class) + a custom DesktopGLRenderEngine that uses our
# shared GrDirectContext instead of EGL/GLES2.
#
# SkiaRenderEngine is NOT ported: its transitive Vulkan / Graphite / AHB
# dependencies don't compile against a host macOS Skia build, and reimplementing
# drawLayers with Skia primitives directly is a tiny fraction of that code.

set(RE_SRC
    src/ExternalTexture.cpp
    src/DesktopGLRenderEngine.cpp
)

add_library(aosp_renderengine STATIC ${RE_SRC})
target_include_directories(aosp_renderengine PUBLIC include)
target_include_directories(aosp_renderengine PRIVATE src)
target_link_libraries(aosp_renderengine PUBLIC
    aosp_platform
    aosp_libutils aosp_libbase aosp_libmath aosp_ftl aosp_libui
    skia
)
target_include_directories(aosp_renderengine PRIVATE
    ${SKIA_DIR}
    ${SKIA_DIR}/include/core
    ${SKIA_DIR}/include/effects
    ${SKIA_DIR}/include/utils
    ${SKIA_DIR}/include/private
    ${SKIA_DIR}/include/private/base
    ${SKIA_DIR}/include/gpu
)
if(NOT MSVC)
    target_compile_options(aosp_renderengine PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-missing-field-initializers
        -Wno-sign-compare
        -Wno-deprecated-declarations
    )
endif()
