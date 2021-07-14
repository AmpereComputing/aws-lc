// -----------------------------------------------------------------------------
// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
// -----------------------------------------------------------------------------

#ifndef HEADER_SYSGENID_TEST_UTIL
#define HEADER_SYSGENID_TEST_UTIL

#include <openssl/base.h>

#if defined(OPENSSL_LINUX)

#define SNAPSAFE_SUPPORTED 0
#define SNAPSAFE_NOT_SUPPORTED 1

#define MUST_BE_MOCKED 0
#define PREFER_REAL_SYSGENID_DEVICE 1

// new_sysgenid_value sets a new SysGenID value. The value set depends on
// whether the real or mocked SysGenID device is used.
// Real SysGenID device: Increment SysGenID value.
// Mocked SysGenID device: Set the new value to the hint value passed in the
// parameter |new_sysgenid_value_hint|.
// Returns 1 if successful and 0 otherwise.
int new_sysgenid_value(uint32_t new_sysgenid_value_hint);

// setup_sysgenid_support determines if SysGenID is supported, and if not,
// initialises a back up method that simulates the SysGenID device.
// Returns 1 if successful and 0 otherwise.
int setup_sysgenid_support(int preference);

// Maybe clean up after tests: removes any SysGenID mock artifacts and resets
// Snapsafe detection.
void maybe_cleanup(void);

#endif // defined(OPENSSL_LINUX)

#endif // HEADER_SYSGENID_TEST_UTIL
