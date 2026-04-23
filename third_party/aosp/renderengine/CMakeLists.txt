# RenderEngine port. SkiaRenderEngine is the graphics-API-agnostic base
# (SkiaGpuContext abstraction over Ganesh / Graphite / GL / Vk). We keep it
# intact and swap only the backend — SkiaGLRenderEngine (EGL+GLES2) is
# replaced by SkiaDesktopGLRenderEngine against our host GrDirectContext.

set(RE_SRC
    src/RenderEngine.cpp
    src/ExternalTexture.cpp
    src/skia/SkiaRenderEngine.cpp
    src/skia/AutoBackendTexture.cpp
    src/skia/ColorSpaces.cpp
    src/skia/compat/GaneshBackendTexture.cpp
    src/skia/compat/GaneshGpuContext.cpp
    src/skia/debug/CaptureTimer.cpp
    src/skia/debug/CommonPool.cpp
    # SkiaCapture.cpp replaced by a header-only stub.
    src/skia/debug/SkiaMemoryReporter.cpp
    src/skia/filters/BlurFilter.cpp
    src/skia/filters/EdgeExtensionShaderFactory.cpp
    src/skia/filters/GainmapFactory.cpp
    src/skia/filters/GaussianBlurFilter.cpp
    src/skia/filters/KawaseBlurDualFilter.cpp
    src/skia/filters/KawaseBlurFilter.cpp
    src/skia/filters/LinearEffect.cpp
    src/skia/filters/LutShader.cpp
    src/skia/filters/MouriMap.cpp
    src/skia/filters/StretchShaderFactory.cpp

    # Our replacement for SkiaGLRenderEngine (EGL/GLES2 variant).
    src/skia/SkiaDesktopGLRenderEngine.cpp
)

add_library(aosp_renderengine STATIC ${RE_SRC})
target_include_directories(aosp_renderengine PUBLIC include)
target_include_directories(aosp_renderengine PRIVATE src)
target_link_libraries(aosp_renderengine PUBLIC
    aosp_platform
    aosp_libutils aosp_libbase aosp_libmath aosp_ftl aosp_libui
    aosp_tonemap aosp_shaders
    skia
)
target_include_directories(aosp_renderengine PRIVATE
    ${SKIA_DIR}
    ${SKIA_DIR}/include/core
    ${SKIA_DIR}/include/effects
    ${SKIA_DIR}/include/docs
    ${SKIA_DIR}/include/utils
    ${SKIA_DIR}/include/private
    ${SKIA_DIR}/include/private/base
    ${SKIA_DIR}/include/gpu
)
if(NOT MSVC)
    target_compile_options(aosp_renderengine PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-sign-compare
        -Wno-deprecated-declarations
        -Wno-format
        -Wno-format-security
        -Wno-gnu-statement-expression
        -Wno-c99-designator
    )
endif()
