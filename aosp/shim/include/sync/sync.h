// libsync shim — sync FDs don't exist on macOS; return not-implemented for
// callers that try to use them.
#pragma once
#include <cstdint>

static inline int sync_wait(int /*fd*/, int /*timeout*/) { return -1; }
static inline int sync_merge(const char * /*name*/, int, int) { return -1; }

struct sync_fence_info {
  char obj_name[32];
  char driver_name[32];
  int32_t status;
  uint32_t flags;
  uint64_t timestamp_ns;
};

struct sync_file_info {
  char name[32];
  int32_t status;
  uint32_t flags;
  uint32_t num_fences;
  uint32_t pad;
  uint64_t sync_fence_info;
};

static inline struct sync_file_info *sync_file_info(int /*fd*/) {
  return nullptr;
}
static inline void sync_file_info_free(struct sync_file_info * /*info*/) {}
