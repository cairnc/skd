# Perfetto trace protos. We vendor the *amalgamated* trace schema
# (perfetto_trace.proto) — a single auto-generated file Perfetto publishes
# containing every data-source message in one namespace. One protoc call,
# one .pb.h/.pb.cc pair, and we get access to all packet types (SF,
# ProcessTree, ftrace, track_event, etc.) without having to cherry-pick
# individual upstream files.
#
# Protoc, libprotobuf, and abseil all come from our submodules under
# third_party/protobuf and third_party/abseil-cpp — no brew dependency.

set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_FILES
    protos/perfetto/trace/perfetto_trace.proto
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
        COMMAND $<TARGET_FILE:protoc>
            --proto_path=${PROTO_ROOT}
            --cpp_out=${GEN_DIR}
            ${PROTO_ROOT}/${PROTO}
        DEPENDS ${PROTO_ROOT}/${PROTO} protoc
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
)
target_link_libraries(aosp_perfetto_protos PUBLIC protobuf::libprotobuf
                                                   absl::hash)
# Generated code has many signed/unsigned warnings in protobuf 30+.
if(NOT MSVC)
    target_compile_options(aosp_perfetto_protos PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
    )
endif()
