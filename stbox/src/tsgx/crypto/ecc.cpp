#include "common/util.h"
#include "stbox/scope_guard.h"
#include "stbox/stx_common.h"
#include "stbox/tsgx/crypto/ecp_interface.h"
#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <sgx_tseal.h>
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <stdlib.h>
#include <string.h>

#include "stbox/ebyte.h"
#include "stbox/eth/eth_hash.h"
#include "stbox/stx_status.h"
#include "stbox/tsgx/crypto/ecc.h"
#include "stbox/tsgx/crypto/ecc_context.h"
#include "stbox/tsgx/secp256k1/secp256k1.h"
#include "stbox/tsgx/secp256k1/secp256k1_ecdh.h"
#include "stbox/tsgx/secp256k1/secp256k1_preallocated.h"
#include "stbox/tsgx/secp256k1/secp256k1_recovery.h"

#define SECP256K1_PRIVATE_KEY_SIZE 32
#define INITIALIZATION_VECTOR_SIZE 12
#define SGX_AES_GCM_128BIT_TAG_T_SIZE sizeof(sgx_aes_gcm_128bit_tag_t)

using namespace stbox;

namespace stbox {
namespace crypto {
std::shared_ptr<ecc_context> context = std::make_shared<ecc_context>();
}
} // namespace stbox

#define AAD_MAC_TEXT_LEN 64
#define AAD_MAC_PREFIX_POS 24
static char aad_mac_text[AAD_MAC_TEXT_LEN] = "tech.yeez.key.manager";
static uint8_t p_iv_text[INITIALIZATION_VECTOR_SIZE] = {
    89, 101, 101, 90, 70, 105, 100, 101, 108, 105, 117, 115}; //"YeeZFidelius";

static int ecdh_hash_function_sha256(unsigned char *output,
                                     const unsigned char *x,
                                     const unsigned char *y, void *data) {
  uint8_t buf[33];
  unsigned char version = (y[31] & 0x01) | 0x02;
  buf[0] = version;
  memcpy(buf + 1, x, 32);
  sgx_sha256_msg(buf, 33, (sgx_sha256_hash_t *)output);
  return 1;
}

uint32_t gen_pkey_from_skey(const secp256k1_context *ctx, const uint8_t *skey,
                            secp256k1_pubkey *pkey) {
  if (!ctx || !skey) {
    printf("Context or Secret key or Public key is null\n");
    return SGX_ERROR_UNEXPECTED;
  }
  if (!secp256k1_ec_pubkey_create(ctx, pkey, skey)) {
    printf("Pubkey computation failed\n");
    return SGX_ERROR_UNEXPECTED;
  }
  ::ypc::change_endian((uint8_t *)pkey, sizeof(secp256k1_pubkey));
  return SGX_SUCCESS;
}

uint32_t sign_hash(const secp256k1_context *ctx, const uint8_t *hash,
                   const uint8_t *skey, uint8_t *sig) {
  secp256k1_ecdsa_recoverable_signature rsig;
  if (!secp256k1_ecdsa_sign_recoverable(ctx, &rsig, hash, skey, NULL, NULL)) {
    printf("Can't sign\n");
    return SGX_ERROR_UNEXPECTED;
  }
  int recid;
  if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid,
                                                               &rsig)) {
    printf("Can't serialize the signature\n");
    return SGX_ERROR_UNEXPECTED;
  }
  sig[64] = (uint8_t)(recid + 27);
  return SGX_SUCCESS;
}

namespace stbox {
namespace crypto {
uint32_t get_secp256k1_public_key_size() {
  return sizeof(secp256k1_pubkey) * sizeof(unsigned char);
}
uint32_t get_secp256k1_sealed_private_key_size() {
  return sgx_calc_sealed_data_size(
      strlen(aad_mac_text), SECP256K1_PRIVATE_KEY_SIZE * sizeof(unsigned char));
}
uint32_t gen_secp256k1_skey(uint32_t skey_size, uint8_t *skey) {
  secp256k1_context *ctx = ::stbox::crypto::context->ctx();
  if (!ctx) {
    printf("Context or Secret key is null\n");
    return SGX_ERROR_UNEXPECTED;
  }
  sgx_status_t se_ret;
  do {
    se_ret = sgx_read_rand(skey, skey_size);
    if (se_ret != SGX_SUCCESS) {
      printf("Failed to generate rand skey\n");
      return SGX_ERROR_UNEXPECTED;
    }
  } while (!secp256k1_ec_seckey_verify(ctx, skey));
  return se_ret;
}

uint32_t seal_secp256k1_private_key(const uint8_t *skey,
                                    uint8_t *sealed_private_key,
                                    uint32_t sealed_size) {
  uint32_t skey_size = SECP256K1_PRIVATE_KEY_SIZE;

  sgx_status_t se_ret = sgx_seal_data(
      (const uint32_t)strlen(aad_mac_text), (const uint8_t *)aad_mac_text,
      (const uint32_t)skey_size, (const uint8_t *)skey,
      (const uint32_t)sealed_size, (sgx_sealed_data_t *)sealed_private_key);
#ifdef STBOX_CRYPTO_VERBOSE
  printf("sealed secret key hex: ");
  print_hex(sealed_private_key, sealed_size);
#endif
  return se_ret;
}
uint32_t unseal_secp256k1_private_key(const uint8_t *sealed_private_key,
                                      uint32_t sealed_size, uint8_t *skey) {
  uint32_t aad_mac_len =
      sgx_get_add_mac_txt_len((const sgx_sealed_data_t *)sealed_private_key);
  if (aad_mac_len == 0xFFFFFFFF) {
    return SGX_ERROR_UNEXPECTED;
  }
  uint32_t skey_size;
  uint8_t *mac_text;
  ff::scope_guard _mac_text_desc([&]() { mac_text = new uint8_t[aad_mac_len]; },
                                 [&]() { delete[] mac_text; });

  auto se_ret = sgx_unseal_data((const sgx_sealed_data_t *)sealed_private_key,
                                (uint8_t *)mac_text, (uint32_t *)&aad_mac_len,
                                skey, &skey_size);
  if (se_ret) {
    return se_ret;
  }

  auto t = memcmp(mac_text, aad_mac_text, aad_mac_len);
  if (t != 0) {
    return SGX_ERROR_MAC_MISMATCH;
  }
  if (skey_size != SECP256K1_PRIVATE_KEY_SIZE) {
    return static_cast<uint32_t>(stx_status::ecc_invalid_skey_size);
  }
  return se_ret;
}

uint32_t generate_secp256k1_pkey_from_skey(const uint8_t *skey, uint8_t *pkey,
                                           uint32_t pkey_size) {
  auto se_ret = (sgx_status_t)gen_pkey_from_skey(context->ctx(), skey,
                                                 (secp256k1_pubkey *)pkey);
  return se_ret;
}

uint32_t get_secp256k1_signature_size() {
  return 1 + sizeof(secp256k1_ecdsa_signature) * sizeof(unsigned char);
}
uint32_t sign_message(const uint8_t *skey, uint32_t skey_size,
                      const uint8_t *data, uint32_t data_size, uint8_t *sig,
                      uint32_t sig_size) {
#ifdef STBOX_CRYPTO_VERBOSE
  printf("unseal secret size: %u\n", skey_size);
  printf("unseal secret key hex: ");
  print_hex(skey, skey_size);
#endif
  sgx_status_t se_ret;

  secp256k1_context *ctx = context->ctx();
  auto hash = stbox::eth::msg_hash(data, data_size);
  sig_size = get_secp256k1_signature_size();
  se_ret = (sgx_status_t)sign_hash(ctx, hash.value(), skey, sig);
#ifdef STBOX_CRYPTO_VERBOSE
  printf("signature size: %u\n", sig_size);
  printf("signature hex: ");
  print_hex(sig, sig_size);
#endif
  return se_ret;
}
uint32_t verify_signature(const uint8_t *data, uint32_t data_size,
                          const uint8_t *sig, uint32_t sig_size,
                          const uint8_t *public_key, uint32_t pkey_size) {
  sgx_status_t se_ret;
  secp256k1_context *ctx = context->ctx();

  secp256k1_pubkey secp256k1_pkey;
  memcpy(&secp256k1_pkey, public_key, pkey_size);
  ::ypc::change_endian((uint8_t *)&secp256k1_pkey, sizeof(secp256k1_pubkey));

  secp256k1_ecdsa_recoverable_signature rsig;
  se_ret = (sgx_status_t)secp256k1_ecdsa_recoverable_signature_parse_compact(
      ctx, &rsig, sig, *(sig + 64) - 27);
  if (!se_ret) {
    return SGX_ERROR_UNEXPECTED;
  }
  secp256k1_ecdsa_signature secp256k1_sig;
  se_ret = (sgx_status_t)secp256k1_ecdsa_recoverable_signature_convert(
      ctx, &secp256k1_sig, &rsig);
  if (!se_ret) {
    return SGX_ERROR_UNEXPECTED;
  }

  auto hash = stbox::eth::msg_hash(data, data_size);
  se_ret = (sgx_status_t)secp256k1_ecdsa_verify(ctx, &secp256k1_sig,
                                                hash.value(), &secp256k1_pkey);
  if (!se_ret) {
    printf("Verify signature failed!\n");
    return SGX_ERROR_UNEXPECTED;
  }
  return SGX_SUCCESS;
}

uint32_t get_encrypt_message_size_with_prefix(uint32_t data_size) {
  return internal::get_rijndael128GCM_encrypt_size(data_size);
}
uint32_t get_decrypt_message_size_with_prefix(uint32_t data_size) {
  return internal::get_rijndael128GCM_decrypt_size(data_size);
}

uint32_t encrypt_message_with_prefix(const uint8_t *public_key,
                                     uint32_t pkey_size, const uint8_t *data,
                                     uint32_t data_size, uint32_t prefix,
                                     uint8_t *cipher, uint32_t cipher_size) {
  sgx_status_t se_ret;
  uint32_t _pkey_size = get_secp256k1_public_key_size();
  uint8_t *_public_key = cipher + data_size;
  const bytes &gskey = context->skey();
  const bytes &gpkey_big = context->pkey_big_endian();
  memcpy(_public_key, gpkey_big.value(), _pkey_size);

  sgx_ec_key_128bit_t derived_key;
  se_ret = (sgx_status_t)internal::gen_sgx_ec_key_128bit(
      public_key, pkey_size, gskey.value(), gskey.size(),
      (uint8_t *)&derived_key);
  if (se_ret) {
    return se_ret;
  }

  uint8_t mac_text[AAD_MAC_TEXT_LEN];
  memcpy(mac_text, aad_mac_text, AAD_MAC_TEXT_LEN);
  uint32_t *p_prefix = (uint32_t *)(mac_text + AAD_MAC_PREFIX_POS);
  *p_prefix = prefix;

  sgx_aes_gcm_128bit_tag_t *p_out_mac =
      (sgx_aes_gcm_128bit_tag_t *)(cipher + data_size + _pkey_size);
  se_ret = sgx_rijndael128GCM_encrypt(
      (const sgx_aes_gcm_128bit_key_t *)derived_key, data, data_size, cipher,
      p_iv_text, INITIALIZATION_VECTOR_SIZE, mac_text, AAD_MAC_TEXT_LEN,
      p_out_mac);
  return se_ret;
}

uint32_t decrypt_message_with_prefix(const uint8_t *skey, uint32_t skey_size,
                                     const uint8_t *cipher,
                                     uint32_t cipher_size, uint8_t *data,
                                     uint32_t data_size, uint32_t prefix) {
  sgx_status_t se_ret;
  sgx_ec_key_128bit_t derived_key;
  uint32_t pkey_size = get_secp256k1_public_key_size();
  se_ret = (sgx_status_t)internal::gen_sgx_ec_key_128bit(
      cipher + data_size, pkey_size, skey, skey_size, (uint8_t *)&derived_key);
  if (se_ret) {
    return se_ret;
  }

  uint8_t mac_text[AAD_MAC_TEXT_LEN];
  memcpy(mac_text, aad_mac_text, AAD_MAC_TEXT_LEN);
  uint32_t *p_prefix = (uint32_t *)(mac_text + AAD_MAC_PREFIX_POS);
  *p_prefix = prefix;

  se_ret = sgx_rijndael128GCM_decrypt(
      (const sgx_aes_gcm_128bit_key_t *)derived_key, cipher, data_size, data,
      p_iv_text, INITIALIZATION_VECTOR_SIZE, mac_text, AAD_MAC_TEXT_LEN,
      (const sgx_aes_gcm_128bit_tag_t *)(cipher + data_size + pkey_size));
  return se_ret;
}

namespace internal {
uint32_t get_rijndael128GCM_encrypt_size(uint32_t data_size) {
  return data_size + get_secp256k1_public_key_size() +
         SGX_AES_GCM_128BIT_TAG_T_SIZE;
}
uint32_t get_rijndael128GCM_decrypt_size(uint32_t cipher_size) {
  return cipher_size - get_secp256k1_public_key_size() -
         SGX_AES_GCM_128BIT_TAG_T_SIZE;
}

uint32_t gen_sgx_ec_key_128bit(const uint8_t *pkey, uint32_t pkey_size,
                               const uint8_t *skey, uint32_t skey_size,
                               uint8_t *derived_key) {
  sgx_status_t se_ret;

  secp256k1_pubkey lpkey;
  memcpy((uint8_t *)&lpkey, pkey, sizeof(secp256k1_pubkey));
  ::ypc::change_endian((uint8_t *)&lpkey, sizeof(secp256k1_pubkey));

  secp256k1_context *ctx = ::stbox::crypto::context->ctx();
  sgx_ec256_dh_shared_t ec256_dh_shared_key;
  se_ret =
      (sgx_status_t)secp256k1_ecdh(ctx, (uint8_t *)&ec256_dh_shared_key, &lpkey,
                                   skey, ecdh_hash_function_sha256, NULL);

  if (!se_ret) {
    return SGX_ERROR_UNEXPECTED;
  }
  uint32_t aad_mac_len = strlen(aad_mac_text);
  se_ret =
      (sgx_status_t)derive_key(&ec256_dh_shared_key, aad_mac_text, aad_mac_len,
                               (sgx_ec_key_128bit_t *)derived_key);
  return se_ret;
}
} // namespace internal

} // namespace crypto
} // namespace stbox

