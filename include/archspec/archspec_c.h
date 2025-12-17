/* This file is a part of Julia. License is MIT: https://julialang.org/license
 *
 * C API for archspec - CPU microarchitecture detection library
 */

#ifndef ARCHSPEC_C_H
#define ARCHSPEC_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the host CPU microarchitecture name (e.g., "haswell", "zen3", "m1")
 * The returned string is statically allocated and must NOT be freed.
 * Returns NULL on failure.
 */
const char* archspec_host_name(void);

/* Returns a comma-separated list of features for the host CPU
 * (e.g., "sse,sse2,avx,avx2,aes")
 * The returned string must be freed with archspec_free().
 * Returns NULL on failure.
 */
char* archspec_host_features(void);

/* Returns the host CPU vendor (e.g., "GenuineIntel", "AuthenticAMD", "Apple")
 * The returned string is statically allocated and must NOT be freed.
 * Returns NULL on failure.
 */
const char* archspec_host_vendor(void);

/* Returns a comma-separated list of features for a given microarchitecture name
 * (e.g., archspec_get_features("haswell") -> "mmx,sse,sse2,...,avx2,...")
 * The returned string must be freed with archspec_free().
 * Returns NULL if the microarchitecture is not found.
 */
char* archspec_get_features(const char* name);

/* Returns optimization flags for a given microarchitecture and compiler
 * Compiler can be: "gcc", "clang", "apple-clang", "intel", "aocc"
 * (e.g., archspec_get_flags("haswell", "gcc") -> "-march=haswell -mtune=haswell")
 * The returned string must be freed with archspec_free().
 * Returns NULL if not found.
 */
char* archspec_get_flags(const char* name, const char* compiler);

/* Returns optimization flags for the host CPU
 * The returned string must be freed with archspec_free().
 * Returns NULL on failure.
 */
char* archspec_host_flags(const char* compiler);

/* Check if a microarchitecture has a specific feature
 * Returns 1 if feature is present, 0 otherwise.
 */
int archspec_has_feature(const char* name, const char* feature);

/* Check if host CPU has a specific feature
 * Returns 1 if feature is present, 0 otherwise.
 */
int archspec_host_has_feature(const char* feature);

/* Returns the number of known microarchitectures */
size_t archspec_target_count(void);

/* Returns the name of the i-th microarchitecture (0-indexed)
 * The returned string is statically allocated and must NOT be freed.
 * Returns NULL if index is out of bounds.
 */
const char* archspec_target_name(size_t index);

/* Check if a microarchitecture name is known
 * Returns 1 if known, 0 otherwise.
 */
int archspec_target_exists(const char* name);

/* Free a string returned by archspec functions
 * Safe to call with NULL.
 */
void archspec_free(char* str);

#ifdef __cplusplus
}
#endif

#endif /* ARCHSPEC_C_H */

