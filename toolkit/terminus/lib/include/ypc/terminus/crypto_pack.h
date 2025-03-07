#pragma once
#include "ypc/byte.h"

namespace ypc {
namespace terminus {
class crypto_pack {
public:
  virtual bytes gen_ecc_private_key() = 0;
  virtual bytes
  gen_ecc_public_key_from_private_key(const bytes &private_key) = 0;

  virtual bytes sign_message(const bytes &msg, const bytes &private_key);
  virtual bool verify_message_signature(const bytes &sig, const bytes &message,
                                        const bytes &pubkey);

  virtual bytes chain_msg(const bytes &message) = 0;
  virtual bytes chain_hash(const bytes &message) = 0;

  // keccak256 on eth
  virtual bytes hash(const bytes &msg) = 0;

  virtual bytes ecc_encrypt(const bytes &msg, const bytes &pubic_key,
                            uint32_t prefix) = 0;
  virtual bytes ecc_decrypt(const bytes &cipher, const bytes &private_key,
                            uint32_t prefix) = 0;

protected:
  virtual bool verify_hash_signature(const bytes &sig, const bytes &hash,
                                     const bytes &pubkey) = 0;
  virtual bytes sign_hash(const bytes &hash, const bytes &private_key) = 0;
};

std::unique_ptr<crypto_pack> intel_sgx_and_eth_compatible();
std::unique_ptr<crypto_pack> sm_compatible();

} // namespace terminus
} // namespace ypc
