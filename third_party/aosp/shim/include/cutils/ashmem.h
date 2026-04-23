// ashmem shim — anonymous shared memory isn't available on macOS. Calls that
// try to allocate fall back to no-ops and return error fds.
#pragma once
#include <cstddef>

static inline int ashmem_create_region(const char * /*name*/, size_t /*size*/) {
  return -1;
}
static inline int ashmem_valid(int /*fd*/) { return 0; }
static inline int ashmem_set_prot_region(int /*fd*/, int /*prot*/) {
  return -1;
}
static inline int ashmem_pin_region(int /*fd*/, size_t /*offset*/,
                                    size_t /*len*/) {
  return -1;
}
static inline int ashmem_unpin_region(int /*fd*/, size_t /*offset*/,
                                      size_t /*len*/) {
  return -1;
}
static inline int ashmem_get_size_region(int /*fd*/) { return -1; }
