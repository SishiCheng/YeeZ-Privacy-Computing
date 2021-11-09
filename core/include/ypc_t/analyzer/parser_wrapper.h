#pragma once
#include "common/endian.h"
#include "sgx_trts.h"
#include "stbox/stx_status.h"
#include "stbox/tsgx/channel/dh_session_initiator.h"
#include "stbox/tsgx/crypto/ecc.h"
#include "stbox/tsgx/log.h"
#include "stbox/tsgx/ocall.h"
#include "ypc_t/analyzer/ntpackage_item_parser.h"
#include "ypc_t/analyzer/parser_wrapper_base.h"
#include "ypc_t/analyzer/sealed_raw_data.h"
#include "ypc_t/ecommon/package.h"
#include <string.h>

namespace ypc {
namespace internal {

template <typename UserItemT, typename ParserT>
class typed_parser_wrapper : public parser_wrapper_base {
public:
  typedef UserItemT (*item_parser_t)(const stbox::bytes::byte_t *, size_t);

  virtual uint32_t begin_parse_data_item() {
    uint32_t r1 = parser_wrapper_base::begin_parse_data_item();
    m_data_source.reset(
        new sealed_data_provider<UserItemT>(m_datahub_session.get()));
    m_data_source->set_engine(&m_engine);
    if (m_item_parser_func) {
      m_data_source->set_item_parser(m_item_parser_func);
    }
    return r1;
  }

  inline void set_item_parser(item_parser_t func) {
    m_item_parser_func = func;
    if (m_data_source) {
      m_data_source->set_item_parser(m_item_parser_func);
    }
  }

  virtual uint32_t parse_data_item(const uint8_t *sealed_data, uint32_t len) {
    uint32_t r1 = parser_wrapper_base::parse_data_item(sealed_data, len);

    m_parser.reset(new ParserT(m_data_source.get(), m_extra_data_sources));

    if (r1 == static_cast<uint32_t>(stbox::stx_status::success)) {
      m_result_str = m_parser->do_parse(m_param);
      // TODO Calculate actual gas cost
      m_cost_gas = 0;
    }
    return r1;
  }

  virtual uint32_t end_parse_data_item() {
    uint32_t r1 = parser_wrapper_base::end_parse_data_item();
    if (r1 != static_cast<uint32_t>(stbox::stx_status::success)) {
      return r1;
    }
    uint32_t sig_size = stbox::crypto::get_secp256k1_signature_size();
    stbox::bytes cost_gas_str(sizeof(m_cost_gas));
    memcpy((uint8_t *)&cost_gas_str[0], (uint8_t *)&m_cost_gas,
           sizeof(m_cost_gas));
    ypc::utc::endian_swap(cost_gas_str);
    m_result_signature_str = stbox::bytes(sig_size);
    m_cost_signature_str = stbox::bytes(sig_size);

    auto cost_msg = m_encrypted_param + m_data_source->data_hash() +
                    m_enclave_hash + cost_gas_str;
    auto status = stbox::crypto::sign_message(
        (uint8_t *)m_private_key.data(), m_private_key.size(),
        (uint8_t *)&cost_msg[0], cost_msg.size(),
        (uint8_t *)&m_cost_signature_str[0], sig_size);
    if (status != stbox::stx_status::success) {
      LOG(ERROR) << "error for sign cost: " << status;
      return status;
    }

    auto msg = m_encrypted_param + m_data_source->data_hash() + m_enclave_hash +
               cost_gas_str + m_encrypted_result_str;

    status = stbox::crypto::sign_message(
        (uint8_t *)m_private_key.data(), m_private_key.size(),
        (uint8_t *)&msg[0], msg.size(), (uint8_t *)&m_result_signature_str[0],
        sig_size);
    return static_cast<uint32_t>(status);
  }

  virtual stbox::bytes data_hash() const { return m_data_source->data_hash(); }

  virtual bool
  user_def_block_result_merge(const std::vector<stbox::bytes> &block_results) {
    ParserT m;
    return m.merge_parse_result(block_results, m_param, m_result_str);
  }

  virtual utc::parser_type_t get_parser_type() {
    return utc::onchain_result_parser;
  }

  virtual stbox::bytes get_result_encrypt_key() const { return stbox::bytes(); }

protected:
  std::unique_ptr<ParserT> m_parser;
  item_parser_t m_item_parser_func;

  std::unique_ptr<sealed_data_provider<UserItemT>> m_data_source;
};

} // namespace internal

template <typename UserItemT, typename ParserT>
using parser_wrapper = internal::typed_parser_wrapper<UserItemT, ParserT>;

template <typename UserItemT, typename ParserT>
class plugin_parser_wrapper
    : public internal::typed_parser_wrapper<UserItemT, ParserT> {
public:
  virtual uint32_t parse_data_item(const uint8_t *sealed_data, uint32_t len) {
    if (!internal::typed_parser_wrapper<UserItemT,
                                        ParserT>::m_item_parser_func) {
      set_item_parser(::ypc::ntpackage_item_parser<stbox::bytes::byte_t,
                                                   UserItemT>::parser);
    }

    return internal::typed_parser_wrapper<UserItemT, ParserT>::parse_data_item(
        sealed_data, len);

  }
};
} // namespace ypc
