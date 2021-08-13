/* Copyright (c) 2015, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef OPENSSL_HEADER_CRYPTO_RAND_INTERNAL_H
#define OPENSSL_HEADER_CRYPTO_RAND_INTERNAL_H

#include <openssl/aes.h>
#include <openssl/cpu.h>

#include "../../internal.h"
#include "../modes/internal.h"

#if defined(__cplusplus)
extern "C" {
#endif


#if !defined(OPENSSL_WINDOWS) && !defined(OPENSSL_FUCHSIA) && \
    !defined(BORINGSSL_UNSAFE_DETERMINISTIC_MODE) && !defined(OPENSSL_TRUSTY)
#define OPENSSL_URANDOM
#endif

// Supported CTR_DRBG AES-based key lengths.
typedef enum {
  CTR_DRBG_AES_128,
  CTR_DRBG_AES_256
} ctr_drbg_key_len_t;

// RAND_bytes_with_additional_data samples from the RNG after mixing 32 bytes
// from |user_additional_data| in.
void RAND_bytes_with_additional_data(uint8_t *out, size_t out_len,
                                     const uint8_t user_additional_data[32],
                                     ctr_drbg_key_len_t ctr_drbg_key_len);

#if defined(BORINGSSL_FIPS)

// We overread from /dev/urandom or RDRAND by a factor of 10 and XOR to whiten.
#define BORINGSSL_FIPS_OVERREAD 10

// CRYPTO_get_seed_entropy writes |out_entropy_len| bytes of entropy, suitable
// for seeding a DRBG, to |out_entropy|. It sets |*out_used_cpu| to one if the
// entropy came directly from the CPU and zero if it came from the OS. It
// actively obtains entropy from the CPU/OS and so should not be called from
// within the FIPS module.
void CRYPTO_get_seed_entropy(uint8_t *out_entropy, size_t out_entropy_len,
                             int *out_used_cpu);

// RAND_load_entropy supplies |entropy_len| bytes of entropy to the module. The
// |from_cpu| parameter is true iff the entropy was obtained directly from the
// CPU.
void RAND_load_entropy(const uint8_t *entropy, size_t entropy_len,
                       int from_cpu);

// RAND_need_entropy is implemented outside of the FIPS module and is called
// when the module has stopped because it has run out of entropy.
void RAND_need_entropy(size_t bytes_needed);

#endif  // BORINGSSL_FIPS

// CRYPTO_sysrand fills |len| bytes at |buf| with entropy from the operating
// system.
void CRYPTO_sysrand(uint8_t *buf, size_t len);

#if defined(OPENSSL_URANDOM)
// CRYPTO_init_sysrand initializes long-lived resources needed to draw entropy
// from the operating system.
void CRYPTO_init_sysrand(void);

// CRYPTO_sysrand_for_seed fills |len| bytes at |buf| with entropy from the
// operating system. It may draw from the |GRND_RANDOM| pool on Android,
// depending on the vendor's configuration.
void CRYPTO_sysrand_for_seed(uint8_t *buf, size_t len);

// CRYPTO_sysrand_if_available fills |len| bytes at |buf| with entropy from the
// operating system, or early /dev/urandom data, and returns 1, _if_ the entropy
// pool is initialized or if getrandom() is not available and not in FIPS mode.
// Otherwise it will not block and will instead fill |buf| with all zeros and
// return 0.
int CRYPTO_sysrand_if_available(uint8_t *buf, size_t len);
#else
OPENSSL_INLINE void CRYPTO_init_sysrand(void) {}

OPENSSL_INLINE void CRYPTO_sysrand_for_seed(uint8_t *buf, size_t len) {
  CRYPTO_sysrand(buf, len);
}

OPENSSL_INLINE int CRYPTO_sysrand_if_available(uint8_t *buf, size_t len) {
  CRYPTO_sysrand(buf, len);
  return 1;
}
#endif

// rand_fork_unsafe_buffering_enabled returns whether fork-unsafe buffering has
// been enabled via |RAND_enable_fork_unsafe_buffering|.
int rand_fork_unsafe_buffering_enabled(void);

// CTR_DRBG_STATE contains the state of a CTR_DRBG based on AES-{128,256}. See
// SP 800-90Ar1.
#define CTR_DRBG_STATE_COUNTER_LEN_IN_BYTES AES_BLOCK_SIZE
#define CTR_DRBG_STATE_COUNTER_LEN_IN_WORDS (CTR_DRBG_STATE_COUNTER_LEN_IN_BYTES / 4)
typedef struct {
  AES_KEY ks;
  block128_f block;
  ctr128_f ctr;
  union {
    uint8_t bytes[CTR_DRBG_STATE_COUNTER_LEN_IN_BYTES];
    uint32_t words[CTR_DRBG_STATE_COUNTER_LEN_IN_WORDS];
  } counter;
  uint64_t reseed_counter;
  size_t aes_key_len;
  size_t entropy_len;
} CTR_DRBG_STATE;

// See SP 800-90Ar1, 9.3.1 (counted in bytes).
#define CTR_DRBG_AES_OUTPUT_LEN AES_BLOCK_SIZE
// See SP 800-90Ar1, table 3 (counted in bytes).
#define CTR_DRBG_AES_128_KEY_LEN 16
#define CTR_DRBG_AES_128_ENTROPY_LEN (CTR_DRBG_AES_OUTPUT_LEN + CTR_DRBG_AES_128_KEY_LEN)
#define CTR_DRBG_AES_256_KEY_LEN 32
#define CTR_DRBG_AES_256_ENTROPY_LEN (CTR_DRBG_AES_OUTPUT_LEN + CTR_DRBG_AES_256_KEY_LEN)
#define CTR_DRBG_MAX_AES_KEY_LEN CTR_DRBG_AES_256_KEY_LEN
#define CTR_DRBG_MAX_ENTROPY_LEN CTR_DRBG_AES_256_ENTROPY_LEN
#define CTR_DRBG_MAX_GENERATE_LENGTH 65536

// CTR_DRBG_init initialises |*drbg|.
// * |entropy|: |CTR_DRBG_MAX_ENTROPY_LEN| bytes in length.
//      |CTR_DRBG_AES_128_ENTROPY_LEN| or |CTR_DRBG_AES_256_ENTROPY_LEN| used as
//      entropy. The former for 128-bit key lengths and the latter for 256-bit
//      key lengths.
// * |personalization|: Optional personalization string. The maximal amount of
//      bytes accepted is |CTR_DRBG_AES_128_ENTROPY_LEN| or
//      |CTR_DRBG_AES_256_ENTROPY_LEN| depending on the key length. The former
//      for 128-bit key length and the latter for 256-bit key length.
// * |personalization_len|: Length of |personalization|.
// * |ctr_drbg_key_len|: Configures the size of the underlying AES key length.
//      Accepted values are 128 and 256.
// Function returns one on success and zero on error.
OPENSSL_EXPORT int CTR_DRBG_init(CTR_DRBG_STATE *drbg,
                                 const uint8_t entropy[CTR_DRBG_MAX_ENTROPY_LEN],
                                 const uint8_t *personalization,
                                 size_t personalization_len,
                                 size_t ctr_drbg_key_len);

// CTR_DRBG_reseed reseeds |drbg|.
// * |entropy|: |CTR_DRBG_MAX_ENTROPY_LEN| bytes in length.
//      |CTR_DRBG_AES_128_ENTROPY_LEN| or |CTR_DRBG_AES_256_ENTROPY_LEN| used as
//      entropy. The former for 128-bit key lengths and the latter for 256-bit
//      key lengths.
// * |additional_data|: Optional additional data string. The maximal amount of
//      bytes accepted is |CTR_DRBG_AES_128_ENTROPY_LEN| or
//      |CTR_DRBG_AES_256_ENTROPY_LEN| depending on the key length. The former
//      for 128-bit key length and the latter for 256-bit key length.
// * |additional_data_len|: Length of |additional_data|.
// Function returns one on success or zero on error.
OPENSSL_EXPORT int CTR_DRBG_reseed(CTR_DRBG_STATE *drbg,
                                   const uint8_t entropy[CTR_DRBG_MAX_ENTROPY_LEN],
                                   const uint8_t *additional_data,
                                   size_t additional_data_len);

// CTR_DRBG_generate processes up to |CTR_DRBG_MAX_ENTROPY_LEN| bytes of
// additional data (if any) and then writes |out_len| random bytes to |out|,
// where |out_len| <= |CTR_DRBG_MAX_GENERATE_LENGTH|. The accepted max length
// for |additional_data_len| depends on the key length of the underlying block
// cipher (|CTR_DRBG_AES_128_ENTROPY_LEN| for 128-bit key length and
// |CTR_DRBG_AES_256_ENTROPY_LEN| for 256-bit key length. It returns one on
// success or zero on error.
OPENSSL_EXPORT int CTR_DRBG_generate(CTR_DRBG_STATE *drbg, uint8_t *out,
                                     size_t out_len,
                                     const uint8_t *additional_data,
                                     size_t additional_data_len);

// CTR_DRBG_clear zeroises the state of |drbg|.
OPENSSL_EXPORT void CTR_DRBG_clear(CTR_DRBG_STATE *drbg);


#if defined(OPENSSL_X86_64) && !defined(OPENSSL_NO_ASM)

OPENSSL_INLINE int have_rdrand(void) {
  return (OPENSSL_ia32cap_get()[1] & (1u << 30)) != 0;
}

// have_fast_rdrand returns true if RDRAND is supported and it's reasonably
// fast. Concretely the latter is defined by whether the chip is Intel (fast) or
// not (assumed slow).
OPENSSL_INLINE int have_fast_rdrand(void) {
  const uint32_t *const ia32cap = OPENSSL_ia32cap_get();
  return (ia32cap[1] & (1u << 30)) && (ia32cap[0] & (1u << 30));
}

// CRYPTO_rdrand writes eight bytes of random data from the hardware RNG to
// |out|. It returns one on success or zero on hardware failure.
int CRYPTO_rdrand(uint8_t out[8]);

// CRYPTO_rdrand_multiple8_buf fills |len| bytes at |buf| with random data from
// the hardware RNG. The |len| argument must be a multiple of eight. It returns
// one on success and zero on hardware failure.
int CRYPTO_rdrand_multiple8_buf(uint8_t *buf, size_t len);

#else  // OPENSSL_X86_64 && !OPENSSL_NO_ASM

OPENSSL_INLINE int have_rdrand(void) {
  return 0;
}

OPENSSL_INLINE int have_fast_rdrand(void) {
  return 0;
}

#endif  // OPENSSL_X86_64 && !OPENSSL_NO_ASM

// Don't retry forever. There is no science in picking this number and can be
// adjusted in the future if need be. We do not backoff forever, because we
// believe that it is easier to detect failing calls than detecting infinite
// spinning loops.
#define MAX_BACKOFF_RETRIES 9
OPENSSL_EXPORT void HAZMAT_set_urandom_test_mode_for_testing(void);

#if defined(__cplusplus)
}  // extern C
#endif

#endif  // OPENSSL_HEADER_CRYPTO_RAND_INTERNAL_H
