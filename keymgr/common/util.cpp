#include "util.h"
#include "ypc/byte.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

std::string create_dir_if_not_exist(const std::string &base,
                                    const std::string &dir) {
  boost::filesystem::path h(ypc::home_directory());
  h = h / boost::filesystem::path(base) / boost::filesystem::path(dir);
  if (!boost::filesystem::exists(h)) {
    bool t = boost::filesystem::create_directory(h);
    if (!t) {
      std::stringstream ss;
      ss << "Cannot create key directory " << h.generic_string();
      throw std::runtime_error(ss.str());
    }
  } else if (!boost::filesystem::is_directory(h)) {
    throw std::runtime_error(boost::str(
        boost::format("%1% is already exist, yet it's not a directory.") %
        h.generic_string()));
  }
  return h.generic_string();
}

uint32_t write_key_pair_to_file(const std::string &filename,
                                const ypc::bytes &pkey,
                                const ypc::bytes &skey) {
  try {
    boost::property_tree::ptree pt;
    pt.put("public_key", pkey);
    pt.put("private_key", skey);
    boost::property_tree::json_parser::write_json(filename, pt);
  } catch (const std::exception &e) {
    throw std::runtime_error(boost::str(
        boost::format("Write key pair to file failed! Error: %1%") % e.what()));
  }
  return 0;
}

uint32_t read_key_pair_from_file(const std::string &filename,
                                 ypc::bytes &b_pkey, ypc::bytes &b_skey) {
  try {
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(filename, pt);
    b_pkey = pt.get<ypc::hex_bytes>("public_key").as<::ypc::bytes>();
    b_skey = pt.get<ypc::hex_bytes>("private_key").as<::ypc::bytes>();
  } catch (const std::exception &e) {
    throw std::runtime_error(
        boost::str(boost::format("Read key pair from file failed! Error: %1%") %
                   e.what()));
  }
  return 0;
}

uint32_t ocall_load_key_pair(const char *key_path_name, uint32_t path_size,
                             uint8_t *public_key, uint32_t pkey_size,
                             uint8_t *sealed_private_key,
                             uint32_t sealed_size) {
  uint32_t ret(1);
  std::string _key_path(key_path_name, path_size);
  std::string key_dir = create_dir_if_not_exist(".", _key_path.c_str());
  boost::filesystem::path key_path(key_dir);
  if (!ypc::is_dir_exists(key_dir)) {
    throw std::runtime_error(
        boost::str(boost::format("Directory not exist %1%!") % key_dir));
  }
  for (auto &f : boost::make_iterator_range(
           boost::filesystem::directory_iterator(key_path), {})) {
    auto name = f.path().filename().generic_string();
    auto pkey_hex = ypc::bytes(public_key, pkey_size).as<ypc::hex_bytes>();
    if (std::string((const char *)pkey_hex.data(), PKEY_FILE_NAME_LENGTH) ==
        name) {
      boost::filesystem::path p = key_dir / boost::filesystem::path(name);
      ypc::bytes b_pkey, b_skey;
      ret = read_key_pair_from_file(p.generic_string(), b_pkey, b_skey);
      memcpy(sealed_private_key, b_skey.data(), b_skey.size());
      break;
    }
  }
  if (ret) {
    throw std::runtime_error(
        boost::str(boost::format("Key pair not found in path %1%!") % key_dir));
  }
  return ret;
}
