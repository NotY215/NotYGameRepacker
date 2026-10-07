#ifndef NOTY_CORE_H
#define NOTY_CORE_H

/*
 * NotY Core - C interface.
 *
 * This header is the only public surface of the C core module. It must
 * remain compilable as pure C17 and must never include C++ headers.
 *
 * All functions here are thread-safe and non-throwing.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOTY_CORE_VERSION_MAJOR 0
#define NOTY_CORE_VERSION_MINOR 1
#define NOTY_CORE_VERSION_PATCH 0

/* Returns a pointer to a static, null-terminated UTF-8 version string.
 * The pointer is valid for the lifetime of the process. */
const char* noty_core_version_string(void);

/* Returns the number of logical CPUs visible to the process.
 * Always >= 1. */
uint32_t noty_core_logical_cpu_count(void);

/* Returns total physical memory in bytes, or 0 on failure. */
uint64_t noty_core_total_physical_memory(void);

/* Returns currently available physical memory in bytes, or 0 on failure. */
uint64_t noty_core_available_physical_memory(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NOTY_CORE_H */