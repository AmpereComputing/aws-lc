# AWS-LC Dilithium readme file

The source code in this folder implements support for [Dilithium](https://www.pq-crystals.org/dilithium/index.shtml) Post-Quantum (PQ) signature scheme (SIG). The Dilithium signature scheme was submitted to the [NIST PQ Crypto standardization project](https://csrc.nist.gov/projects/post-quantum-cryptography/post-quantum-cryptography-standardization) by the PQ-Crystals team. The team also developed and maintains Dilithium’s source code repository, publicly available at Github ([link](https://github.com/pq-crystals/dilithium)).

Dilithium is specified with three parameter sets targeting security levels 2, 3, and 5 as defined by NIST. These three versions are denoted by Dilithium2, Dilithium3, and Dilithium5. Moreover, the Crystals team defined additional variants of each version that internally uses AES instead of SHA3 and SHAKE algorithms. These versions are denoted Dilithium2-AES, Dilithium3-AES, and Dilithium5-AES.

The AWS-LC team considers the official repository of [Dilithium](https://github.com/pq-crystals/dilithium) the primary source of Dilithium’s implementation and takes the code directly from it. The code is integrated in AWS-LC with only minimal changes that are required to build on the platforms AWS-LC supports (see below for details).

NIST has not published the final PQ standard yet, and is not expected to do so until 2024. Therefore, the specification and implementation of Dilithium is not finalized yet. Potentially, there will be changes to Dilithium in the future. Some changes might even break backwards compatibility. The AWS-LC team follows the developments around the PQC project and will update the implementation and documentation if necessary. Therefore, AWS-LC can not promise backward compatibility of the Dilithium implementation and API until NIST locks in the specification and reserves the right to change the implementation if necessary.

**Supported versions.** AWS-LC supports only Dilithium3 algorithm at this point. The NID assigned to Dilithium3 is `NID_DILITHIUM3` and the corresponding `PKEY` identifier is `EVP_PKEY_DILITHIUM3`.

**Source code origin and modifications.** The source code was taken from the primary source of Diltihium at [link](https://github.com/pq-crystals/dilithium), at [commit](https://github.com/pq-crystals/dilithium/commit/3e9b9f1412f6c7435dbeb4e10692ea58f181ee51) as of 9th May 2022.
The `api.h`, `fips202.h` and `params.h` header files were modified [in this PR](https://github.com/awslabs/aws-lc/pull/655) to support our [prefixed symbols build](https://github.com/awslabs/aws-lc/blob/main/BUILDING.md#building-with-prefixed-symbols).

Only the reference C implementation of Dilithium3 is currently integrated. The code is in the `pqcrystals-dilithium_dilithium3_ref` folder. The following changes were made to the code.

* `randombytes.{h|c}` are deleted because we are using the randomness generation functions provided by AWS-LC.
* `rng.{h|c}` are deleted because we are using the randomness generation functions provided by AWS-LC.
* `symmetric-aes.c` are removed because we are using only the SHA3 based Dilithium.
* `sign.c`: calls to `randombytes` function is replaced with calls to `pq_custom_randombytes` and the appropriate header file is included (`crypto/rand_extra/pq_custom_randombytes.h`).
* `sign.c` was modified from `*mlen = -1;` to `*mlen = 0;` within the `crypto_sign_open` function. This was modified as `*mlen = -1;` is attempting to assign `-1` to an unsigned integer, giving the warning: ` warning C4245: '=': conversion from 'int' to 'std::size_t'`. This warning would cause Windows x86 builds to fail. See [pq-crystals/dilithium#65](https://github.com/pq-crystals/dilithium/issues/65) for upstream issue raised.
* `sign.c` was modified to correct the documentation around `crypto_sign_verify`. See [pq-crystals/dilithium/#66](https://github.com/pq-crystals/dilithium/pull/66) for upstream PR.
* `config.h`: `#define DILITHIUM_MODE 3`  is explicitly defined (to specify Dilithium3).

**Usage.** The API is defined and documented in `include/openssl/evp.h`. To see examples of how to use Dilithium3, see `crypto/dilithium/p_dilithium_test.cc`. TLDR:

```
// Creates a new PKEY and PKEY context with Dilithium3
EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DILITHIUM3, nullptr);
EVP_PKEY *dilithium_pkey = EVP_PKEY_new();

// Generates a (public, private) key pair
EVP_PKEY_keygen(ctx, &dilithium_pkey)

// Sign
bssl::ScopedEVP_MD_CTX md_ctx;
EVP_DigestSignInit(md_ctx.get(), NULL, NULL, NULL, dilithium_pkey);
EVP_DigestSign(md_ctx.get(), signature, &signature_len, msg, msg_len);

// Verify
EVP_DigestVerify(md_ctx.get(), signature, signature_len, msg, msg_len);
```