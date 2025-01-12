/*
 * Copyright (c) 2024-2025 The mlkem-native project authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MLK_CONFIG_H
#define MLK_CONFIG_H

/* Namespacing: All symbols are of the form mlkem*. Level-specific
 * symbols are further prefixed with their security level, e.g.
 * mlkem512*, mlkem768*, mlkem1024*. */
#define MLK_NAMESPACE_PREFIX mlkem
#define MLK_NAMESPACE_PREFIX_ADD_LEVEL
#define MLK_MONOBUILD

#endif /* MLkEM_NATIVE_CONFIG_H */
