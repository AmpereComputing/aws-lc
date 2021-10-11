// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <openssl/crypto.h>
#include <openssl/service_indicator.h>
#include "internal.h"

#if defined(AWSLC_FIPS)

// Should only be called once per thread. Only called when initializing the state
// in |FIPS_service_indicator_before_call|.
static int FIPS_service_indicator_init_state(void) {
  struct fips_service_indicator_state *indicator;
  indicator = OPENSSL_malloc(sizeof(struct fips_service_indicator_state));
  if (indicator == NULL || !CRYPTO_set_thread_local(
      AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE, indicator, OPENSSL_free)) {
    return 0;
  }
  indicator->lock_state = STATE_UNLOCKED;
  indicator->counter = 0;
  return 1;
}

static uint64_t FIPS_service_indicator_get_counter(void) {
  struct fips_service_indicator_state *indicator =
      CRYPTO_get_thread_local(AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE);
  if (indicator == NULL) {
    return 0;
  }
  return indicator->counter;
}

uint64_t FIPS_service_indicator_before_call(void) {
  struct fips_service_indicator_state *indicator =
      CRYPTO_get_thread_local(AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE);
  if (indicator == NULL) {
    if(!FIPS_service_indicator_init_state()) {
      return 0;
    }
  }
  return FIPS_service_indicator_get_counter();
}

uint64_t FIPS_service_indicator_after_call(void) {
  return FIPS_service_indicator_get_counter();
}

int FIPS_service_indicator_check_approved(uint64_t before, uint64_t after) {
  if(before != after) {
    return AWSLC_APPROVED;
  }
  return AWSLC_NOT_APPROVED;
}

void FIPS_service_indicator_update_state(void) {
  struct fips_service_indicator_state *indicator =
      CRYPTO_get_thread_local(AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE);
  if (indicator == NULL) {
    return;
  }
  if(indicator->lock_state == STATE_UNLOCKED) {
    indicator->counter++;
  }
}

void FIPS_service_indicator_lock_state(void) {
  struct fips_service_indicator_state *indicator =
      CRYPTO_get_thread_local(AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE);
  if (indicator == NULL) {
    return;
  }
  // This shouldn't overflow unless |FIPS_service_indicator_unlock_state| wasn't
  // correctly called after |FIPS_service_indicator_lock_state| in the same
  // function.
  indicator->lock_state++;
}

void FIPS_service_indicator_unlock_state(void) {
  struct fips_service_indicator_state *indicator =
      CRYPTO_get_thread_local(AWSLC_THREAD_LOCAL_FIPS_SERVICE_INDICATOR_STATE);
  if (indicator == NULL) {
    return;
  }
  // This shouldn't overflow unless |FIPS_service_indicator_lock_state| wasn't
  // correctly called before |FIPS_service_indicator_unlock_state| in the same
  // function.
  indicator->lock_state--;
}

void AES_verify_service_indicator(const EVP_CIPHER_CTX *ctx, const unsigned key_rounds) {
  if(ctx != NULL) {
    switch(EVP_CIPHER_CTX_nid(ctx)) {
      case NID_aes_128_ecb:
      case NID_aes_192_ecb:
      case NID_aes_256_ecb:

      case NID_aes_128_cbc:
      case NID_aes_192_cbc:
      case NID_aes_256_cbc:

      case NID_aes_128_ctr:
      case NID_aes_192_ctr:
      case NID_aes_256_ctr:
        FIPS_service_indicator_update_state();
        break;
      default:
        break;
    }
  } else {
    // hwaes_capable when enabled in x86 uses 9, 11, 13 for key rounds.
    // hwaes_capable when enabled in ARM uses 10, 12, 14 for key rounds.
    // When compiling with different ARM specific platforms, 9, 11, 13 are used for
    // key rounds.
    switch (key_rounds) {
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
        FIPS_service_indicator_update_state();
        break;
      default:
        break;
    }
  }
}

void AEAD_GCM_verify_service_indicator(const EVP_AEAD_CTX *ctx) {
  // We only have support for 128 bit and 256 bit keys for AES-GCM. AES-GCM is
  // approved only with an internal IV, see SP 800-38D Sec 8.2.2.
  // Not the best way to write this, but the delocate parser for ARM/clang can't
  // recognize || if statements, or switch statements for this.
  // TODO: Update the delocate parser to be able to recognize a more readable
  // version of this.
   if(EVP_AEAD_key_length(ctx->aead) == 16) {
    FIPS_service_indicator_update_state();
   } else if(EVP_AEAD_key_length(ctx->aead) == 32) {
    FIPS_service_indicator_update_state();
   }
}

void AEAD_CCM_verify_service_indicator(const EVP_AEAD_CTX *ctx) {
  // Only 128 bit keys with 32 bit tag lengths are approved for AES-CCM.
  if(EVP_AEAD_key_length(ctx->aead) == 16 && ctx->tag_len == 4) {
    FIPS_service_indicator_update_state();
  }
}

void AES_CMAC_verify_service_indicator(const CMAC_CTX *ctx) {
  // Only 128 and 256 bit keys are approved for AES-CMAC.
  switch (ctx->cipher_ctx.key_len) {
    case 16:
    case 32:
      FIPS_service_indicator_update_state();
      break;
    default:
      break;
  }
}

void HMAC_verify_service_indicator(const EVP_MD *evp_md) {
  // HMAC with SHA1, SHA224, SHA256, SHA384, and SHA512 are approved.
  switch (evp_md->type){
    case NID_sha1:
    case NID_sha224:
    case NID_sha256:
    case NID_sha384:
    case NID_sha512:
      FIPS_service_indicator_update_state();
      break;
    default:
      break;
  }
}


#else

uint64_t FIPS_service_indicator_before_call(void) { return 0; }
uint64_t FIPS_service_indicator_after_call(void) { return 0; }


// Service indicator check functions listed below are optimized to not do extra
// checks, when not in FIPS mode. Arguments are cast with |OPENSSL_UNUSED| in an
// attempt to avoid unused warnings.

int FIPS_service_indicator_check_approved(OPENSSL_UNUSED uint64_t before,
                                          OPENSSL_UNUSED uint64_t after) {
  return AWSLC_APPROVED;
}

void AES_verify_service_indicator(OPENSSL_UNUSED const EVP_CIPHER_CTX *ctx,
                                  OPENSSL_UNUSED const unsigned key_rounds) { }

void AEAD_GCM_verify_service_indicator(OPENSSL_UNUSED const EVP_AEAD_CTX *ctx) { }

void AEAD_CCM_verify_service_indicator(OPENSSL_UNUSED const EVP_AEAD_CTX *ctx) { }

void AES_CMAC_verify_service_indicator(OPENSSL_UNUSED const CMAC_CTX *ctx) { }

void HMAC_verify_service_indicator(OPENSSL_UNUSED const EVP_MD *evp_md) { }

#endif // AWSLC_FIPS



