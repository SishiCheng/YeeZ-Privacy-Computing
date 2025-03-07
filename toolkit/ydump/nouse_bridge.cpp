#include "sgx_dh.h"
#include "sgx_urts.h"
#include "stbox/tsgx/channel/dh_cdef.h"
#include "ypc/sgx/parser_sgx_module.h"

using stx_status = stbox::stx_status;

extern "C" {
uint32_t datahub_session_request_ocall(sgx_dh_msg1_t *dh_msg1,
                                       uint32_t *session_id);
uint32_t datahub_exchange_report_ocall(sgx_dh_msg2_t *dh_msg2,
                                       sgx_dh_msg3_t *dh_msg3,
                                       uint32_t session_id);
uint32_t datahub_send_request_ocall(uint32_t session_id,
                                    secure_message_t *req_message,
                                    size_t req_message_size,
                                    size_t max_payload_size,
                                    secure_message_t *resp_message,
                                    size_t resp_message_size);
uint32_t datahub_end_session_ocall(uint32_t session_id);

uint32_t km_session_request_ocall(sgx_dh_msg1_t *dh_msg1, uint32_t *session_id);
uint32_t km_exchange_report_ocall(sgx_dh_msg2_t *dh_msg2,
                                  sgx_dh_msg3_t *dh_msg3, uint32_t session_id);
uint32_t km_send_request_ocall(uint32_t session_id,
                               secure_message_t *req_message,
                               size_t req_message_size, size_t max_payload_size,
                               secure_message_t *resp_message,
                               size_t resp_message_size);
uint32_t km_end_session_ocall(uint32_t session_id);

uint32_t ocall_read_next_extra_data_item(uint8_t *data_hash,
                                         uint32_t hash_size);
uint32_t ocall_get_next_extra_data_item_size();
uint32_t ocall_get_next_extra_data_item_data(uint8_t *item_data,
                                             uint32_t ndi_size);
}

uint32_t datahub_session_request_ocall(sgx_dh_msg1_t *dh_msg1,
                                       uint32_t *session_id) {
  return 0;
}
uint32_t datahub_exchange_report_ocall(sgx_dh_msg2_t *dh_msg2,
                                       sgx_dh_msg3_t *dh_msg3,
                                       uint32_t session_id) {
  return 0;
}
uint32_t datahub_send_request_ocall(uint32_t session_id,
                                    secure_message_t *req_message,
                                    size_t req_message_size,
                                    size_t max_payload_size,
                                    secure_message_t *resp_message,
                                    size_t resp_message_size) {
  return 0;
}
uint32_t datahub_end_session_ocall(uint32_t session_id) { return 0; }

uint32_t km_session_request_ocall(sgx_dh_msg1_t *dh_msg1,
                                  uint32_t *session_id) {
  return 0;
}
uint32_t km_exchange_report_ocall(sgx_dh_msg2_t *dh_msg2,
                                  sgx_dh_msg3_t *dh_msg3, uint32_t session_id) {
  return 0;
}
uint32_t km_send_request_ocall(uint32_t session_id,
                               secure_message_t *req_message,
                               size_t req_message_size, size_t max_payload_size,
                               secure_message_t *resp_message,
                               size_t resp_message_size) {
  return 0;
}
uint32_t km_end_session_ocall(uint32_t session_id) { return 0; }

uint32_t ocall_read_next_extra_data_item(uint8_t *data_hash,
                                         uint32_t hash_size) {
  return 0;
}
uint32_t ocall_get_next_extra_data_item_size() { return 0; }
uint32_t ocall_get_next_extra_data_item_data(uint8_t *item_data,
                                             uint32_t ndi_size) {
  return 0;
}
