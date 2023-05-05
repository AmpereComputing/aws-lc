// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Weak symbols are only supported well on Linux platforms. The FIPS failure
// callback functions that attempt to use weak symbols in bcm.c are disabled
// on other platforms. Therefore, only run these tests on supported platforms.
#if defined(__ELF__) && defined(__GNUC__)

#include <gtest/gtest.h>
#include <openssl/ec_key.h>
#include <openssl/nid.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <algorithm>
#include <list>
#include <string>
#include "test/gtest_main.h"
using namespace std;

extern "C" {
OPENSSL_EXPORT void AWS_LC_fips_failure_callback(const char* error);
}

int failure_count = 0;
list<string> failure_messages = {};
void AWS_LC_fips_failure_callback(const char* error) {
  failure_count++;
  failure_messages.emplace_back(error);
}

static bool message_in_errors(const string& expected_message) {
  for (const auto& msg : failure_messages) {
    if (msg.find(expected_message) != string::npos) {
      return true;
    }
  }
  return false;
}

struct test_config {
  string expected_failure_message;
  int initial_failure_count;
};

// If hmac sha-256 is broken the integrity check can not be trusted to check
// itself and fails earlier
set<string> integrity_test_names = {"SHA-256", "HMAC-SHA-256"};
const test_config integrity_test_config = {
    "BORINGSSL_integrity_test",
    1,
};

// The lazy tests are not run at power up, only when called directly with
// BORINGSSL_self_test, therefore the callback should have been called once
set<string> lazy_test_names = {"ECDSA-sign", "ECDSA-verify", "FFDH", "RSA-sign", "RSA-verify", "Z-computation"};
const test_config lazy_test_config = {
    "BORINGSSL_self_test",
    0,
};

// The fast tests run automatically at startup and will report a failure to
// the test callback immediately, and then again when BORINGSSL_self_test is called
const test_config fast_test_config = {
    "boringssl_self_test_startup",
    1,
};

static test_config get_self_test_failure_config(char* broken_kat) {
  if(integrity_test_names.find(broken_kat) != integrity_test_names.end()) {
    return integrity_test_config;
  } else if (lazy_test_names.find(broken_kat) != lazy_test_names.end()) {
    return lazy_test_config;
  } else {
    return fast_test_config;
  }
}

TEST(FIPSCallback, PowerOnTests) {
  ASSERT_EQ(1, FIPS_mode());
  // At this point the library has loaded, if a self test was broken
  // AWS_LC_FIPS_Callback would have already been called. If this test
  // wasn't broken the call count should be zero
  char* broken_kat = getenv("FIPS_CALLBACK_TEST_POWER_ON_TEST_FAILURE");
  if (broken_kat != nullptr) {
    test_config config = get_self_test_failure_config(broken_kat);
    // Fast tests will have already run and if they were broken our callback would
    // have already been called
    ASSERT_EQ(config.initial_failure_count, failure_count);
    // BORINGSSL_self_test will re-run the fast tests and trigger the lazy tests.
    ASSERT_FALSE(BORINGSSL_self_test());
    ASSERT_EQ(config.initial_failure_count + 1, failure_count);
    ASSERT_TRUE(message_in_errors(config.expected_failure_message));
  } else {
    // break-kat.go has not run and corrupted this test yet, everything should work
    ASSERT_TRUE(BORINGSSL_self_test());
    ASSERT_EQ(0, failure_count);
  }
  ASSERT_EQ(1, FIPS_mode());
}

TEST(FIPSCallback, DRBGRuntime) {
  // At this point the library has loaded, if a self test was broken
  // AWS_LC_FIPS_Callback would have already been called. If this test
  // wasn't broken the call count should be zero
  char*broken_runtime_test = getenv("BORINGSSL_FIPS_BREAK_TEST");
  ASSERT_EQ(0, failure_count);
  ASSERT_EQ(1, FIPS_mode());
  uint8_t buf[10];
  if (broken_runtime_test != nullptr && strcmp(broken_runtime_test, "CRNG" ) == 0) {
    ASSERT_FALSE(RAND_bytes(buf, sizeof(buf)));
    ASSERT_EQ(1, failure_count);
  } else {
    // BORINGSSL_FIPS_BREAK_TEST has not been set and everything should work
    ASSERT_TRUE(RAND_bytes(buf, sizeof(buf)));
    ASSERT_EQ(0, failure_count);
  }
  ASSERT_EQ(1, FIPS_mode());
}

TEST(FIPSCallback, RSARuntimeTest) {
  // At this point the library has loaded, if a self test was broken
  // AWS_LC_FIPS_Callback would have already been called. If this test
  // wasn't broken the call count should be zero
  char*broken_runtime_test = getenv("BORINGSSL_FIPS_BREAK_TEST");
  bssl::UniquePtr<RSA> rsa(RSA_new());
  ASSERT_EQ(0, failure_count);
  ASSERT_EQ(1, FIPS_mode());
  if (broken_runtime_test != nullptr && (strcmp(broken_runtime_test, "RSA_PWCT" ) == 0 ||
                                         strcmp(broken_runtime_test, "CRNG" ) == 0)) {
    ASSERT_FALSE(RSA_generate_key_fips(rsa.get(), 2048, nullptr));
    // RSA key generation can call the DRBG multiple times before failing we
    // don't know how many times, but it should fail at least once.
    ASSERT_NE(0, failure_count);
  } else {
    // BORINGSSL_FIPS_BREAK_TEST has not been set and everything should work
    ASSERT_TRUE(RSA_generate_key_fips(rsa.get(), 2048, nullptr));
  }
  ASSERT_EQ(1, FIPS_mode());
}

TEST(FIPSCallback, ECDSARuntimeTest) {
  // At this point the library has loaded, if a self test was broken
  // AWS_LC_FIPS_Callback would have already been called. If this test
  // wasn't broken the call count should be zero
  char*broken_runtime_test = getenv("BORINGSSL_FIPS_BREAK_TEST");
  bssl::UniquePtr<EC_KEY> key(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
  ASSERT_TRUE(key);
  ASSERT_EQ(0, failure_count);
  ASSERT_EQ(1, FIPS_mode());
  if (broken_runtime_test != nullptr && (strcmp(broken_runtime_test, "ECDSA_PWCT" ) == 0 ||
                                         strcmp(broken_runtime_test, "CRNG" ) == 0)) {
    ASSERT_FALSE(EC_KEY_generate_key_fips(key.get()));
    // EC key generation can call the DRBG multiple times before failing, we
    // don't know how many times, but it should fail at least once.
    ASSERT_NE(0, failure_count);
  } else {
    // BORINGSSL_FIPS_BREAK_TEST has not been set and everything should work
    ASSERT_TRUE(EC_KEY_generate_key_fips(key.get()));
  }
  ASSERT_EQ(1, FIPS_mode());
}

#endif
