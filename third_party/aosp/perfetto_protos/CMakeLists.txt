# Perfetto SurfaceFlinger layer-trace protos.
#
# We compile a handful of .proto files out of perfetto's trace schema into C++
# via protoc. Protos are proto2 and use AOSP's "protos/perfetto/trace/..." path
# style, so we set PROTO_ROOT as the include path.

find_package(Protobuf REQUIRED)

set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_FILES
    protos/perfetto/trace/android/graphics/rect.proto
    protos/perfetto/trace/android/surfaceflinger_common.proto
    protos/perfetto/trace/android/surfaceflinger_layers.proto
    protos/perfetto/trace/android/surfaceflinger_transactions.proto
    protos/perfetto/trace/trace.proto
)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN_DIR})

set(GEN_SRCS)
set(GEN_HDRS)
foreach(PROTO ${PROTO_FILES})
    get_filename_component(PROTO_STEM ${PROTO} NAME_WE)
    get_filename_component(PROTO_DIR ${PROTO} DIRECTORY)
    set(OUT_CC  ${GEN_DIR}/${PROTO_DIR}/${PROTO_STEM}.pb.cc)
    set(OUT_H   ${GEN_DIR}/${PROTO_DIR}/${PROTO_STEM}.pb.h)
    add_custom_command(
        OUTPUT ${OUT_CC} ${OUT_H}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${PROTO_ROOT}
            --cpp_out=${GEN_DIR}
            ${PROTO_ROOT}/${PROTO}
        DEPENDS ${PROTO_ROOT}/${PROTO}
        COMMENT "protoc ${PROTO}"
        VERBATIM
    )
    list(APPEND GEN_SRCS ${OUT_CC})
    list(APPEND GEN_HDRS ${OUT_H})
endforeach()

add_library(aosp_perfetto_protos STATIC ${GEN_SRCS})
target_include_directories(aosp_perfetto_protos PUBLIC
    ${GEN_DIR}          # so generated cross-includes "protos/perfetto/..." resolve
    ${GEN_DIR}/protos   # so upstream-style <perfetto/trace/...> resolves
    ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(aosp_perfetto_protos PUBLIC ${Protobuf_LIBRARIES})
# Generated code has many signed/unsigned warnings in protobuf 30+.
if(NOT MSVC)
    target_compile_options(aosp_perfetto_protos PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
    )
endif()
