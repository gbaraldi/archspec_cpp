/* This file is a part of Julia. License is MIT: https://julialang.org/license
 *
 * Example demonstrating the C API for archspec
 */

#include "archspec/archspec_c.h"
#include <stdio.h>

int main(void) {
    printf("=== archspec C API Example ===\n\n");

    /* Host CPU information */
    const char* host_name = archspec_host_name();
    const char* host_vendor = archspec_host_vendor();

    printf("Host CPU:\n");
    printf("  Name:   %s\n", host_name ? host_name : "(unknown)");
    printf("  Vendor: %s\n", host_vendor ? host_vendor : "(unknown)");

    char* host_features = archspec_host_features();
    if (host_features) {
        printf("  Features: %s\n", host_features);
        archspec_free(host_features);
    }

    /* Get optimization flags */
    char* gcc_flags = archspec_host_flags("gcc");
    if (gcc_flags) {
        printf("  GCC flags: %s\n", gcc_flags);
        archspec_free(gcc_flags);
    }

    char* clang_flags = archspec_host_flags("clang");
    if (clang_flags) {
        printf("  Clang flags: %s\n", clang_flags);
        archspec_free(clang_flags);
    }

    /* Check for specific features */
    printf("\nFeature checks:\n");
    printf("  Has SSE4.2: %s\n", archspec_host_has_feature("sse4_2") ? "yes" : "no");
    printf("  Has AVX2:   %s\n", archspec_host_has_feature("avx2") ? "yes" : "no");
    printf("  Has NEON:   %s\n", archspec_host_has_feature("neon") ? "yes" : "no");

    /* Query a specific target */
    printf("\nHaswell features:\n");
    char* haswell_features = archspec_get_features("haswell");
    if (haswell_features) {
        printf("  %s\n", haswell_features);
        archspec_free(haswell_features);
    } else {
        printf("  (not found)\n");
    }

    char* haswell_flags = archspec_get_flags("haswell", "gcc");
    if (haswell_flags) {
        printf("  GCC flags: %s\n", haswell_flags);
        archspec_free(haswell_flags);
    }

    /* List all targets */
    printf("\nKnown targets (%zu total):\n  ", archspec_target_count());
    size_t count = archspec_target_count();
    for (size_t i = 0; i < count && i < 10; i++) {
        const char* name = archspec_target_name(i);
        if (name) {
            printf("%s", name);
            if (i < 9 && i < count - 1)
                printf(", ");
        }
    }
    if (count > 10)
        printf(", ... (%zu more)", count - 10);
    printf("\n");

    /* Check if a target exists */
    printf("\nTarget existence:\n");
    printf("  'skylake' exists: %s\n", archspec_target_exists("skylake") ? "yes" : "no");
    printf("  'zen4' exists:    %s\n", archspec_target_exists("zen4") ? "yes" : "no");
    printf("  'foobar' exists:  %s\n", archspec_target_exists("foobar") ? "yes" : "no");

    return 0;
}
