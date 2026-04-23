# libgui subset — upstream frameworks/native/libs/gui impls for layer_state_t,
# gui::WindowInfo, and gui::LayerMetadata. Bytes identical to upstream. Every
# Parcel write/read method in these files ends up as a no-op at runtime
# because our Parcel shim (shim/include/binder/Parcelable.h) returns OK
# without touching memory — the replayer never IPCs.
file(GLOB LIBGUI_SRC CONFIGURE_DEPENDS src/*.cpp)

add_library(aosp_libgui STATIC ${LIBGUI_SRC})
target_link_libraries(aosp_libgui PUBLIC
    aosp_platform aosp_libutils aosp_libbase aosp_libmath aosp_libui aosp_ftl
)
if(NOT MSVC)
    target_compile_options(aosp_libgui PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-missing-field-initializers
        -Wno-sign-compare
        -Wno-deprecated-declarations
    )
endif()
