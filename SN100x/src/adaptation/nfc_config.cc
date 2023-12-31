/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/******************************************************************************
 *
 *  Copyright 2018-2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "nfc_config.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <cutils/properties.h>
#include <utils/Log.h>

#include <config.h>

#include "NfcAdaptation.h"

using namespace ::std;
using namespace ::android::base;
#if(NXP_EXTNS == TRUE)
#define PATH_TRANSIT_CONF "/data/nfc/libnfc-nxpTransit.conf"
#endif
namespace {
std::string searchConfigPath(std::string file_name) {
  const std::vector<std::string> search_path = {
      "/odm/etc/", "/vendor/etc/", "/system_ext/etc/", "/etc/", "/product/etc/", //To pick the correct config file from /vendor/etc/
  };
  for (std::string path : search_path) {
    path.append(file_name);
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0) continue;
    if (S_ISREG(file_stat.st_mode)) return path;
  }
  return "";
}
// Configuration File Search sequence
// 1. If prop_config_file_name is defined.(where prop_config_file_name is the
//   value of the property persist.nfc_cfg.config_file_name)
//   Search a file matches prop_config_file_name.
// 2. If SKU is defined (where SKU is the value of the property
//   ro.boot.product.hardware.sku)
//   Search a file matches libnfc-nci-SKU.conf
// 3. If none of 1,2 is defined, search a default file name "libnfc-nci.conf".
std::string findConfigPath() {
  char system_device_prop[PROPERTY_VALUE_MAX] = {0};
  int ret = 0;

  string f_path = searchConfigPath(
      android::base::GetProperty("persist.nfc_cfg.config_file_name", ""));
  if (!f_path.empty()) return f_path;

  // Search for libnfc-nci-SKU.conf
  f_path = searchConfigPath(
      "libnfc-nci-" +
      android::base::GetProperty("ro.boot.product.hardware.sku", "") + ".conf");
  if (!f_path.empty()) return f_path;

  // load default file if the desired file not found.
  ret = __system_property_get("ro.product.system.device", system_device_prop);
  if (ret <= 0) {
      ALOGE("Failure to read system device property");
      return searchConfigPath("libnfc-nci.conf");
  // load SN100 file to support SN1XX for msm8937
  } else if (!strcmp(system_device_prop, "msm8937_64") ||
             !strcmp(system_device_prop, "msm8937_32")) {
      return searchConfigPath("libnfc-nci_SN100.conf");
  } else {
      return searchConfigPath("libnfc-nci.conf");
  }
}

}  // namespace

void NfcConfig::loadConfig() {
  string config_path = findConfigPath();
  CHECK(config_path != "");
  config_.parseFromFile(config_path);
  /* Read vendor specific configs */
  NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
  std::map<std::string, ConfigValue> configMap;
  theInstance.GetVendorConfigs(configMap);
#if(NXP_EXTNS == TRUE)
  struct stat file_stat;

  std::string nxp_config;
  nxp_config = theInstance.HalGetProperty("libnfc-nxp.conf");
  config_.cur_file_name_ = "nxpConfig";
  config_.parseFromString(nxp_config);
  /* Read Transit configs if available */
  if (stat(PATH_TRANSIT_CONF, &file_stat) == 0)
    config_.parseFromFile(PATH_TRANSIT_CONF);
#endif
  for (auto config : configMap) {
    config_.addConfig(config.first, config.second);
  }
}

NfcConfig::NfcConfig() { loadConfig(); }

NfcConfig& NfcConfig::getInstance() {
  static NfcConfig theInstance;
  if (theInstance.config_.isEmpty()) {
    theInstance.loadConfig();
  }
  return theInstance;
}

bool NfcConfig::hasKey(const std::string& key) {
  return getInstance().config_.hasKey(key);
}

std::string NfcConfig::getString(const std::string& key) {
  return getInstance().config_.getString(key);
}

std::string NfcConfig::getString(const std::string& key,
                                 std::string default_value) {
  if (hasKey(key)) return getString(key);
  return default_value;
}

unsigned NfcConfig::getUnsigned(const std::string& key) {
  return getInstance().config_.getUnsigned(key);
}

unsigned NfcConfig::getUnsigned(const std::string& key,
                                unsigned default_value) {
  if (hasKey(key)) return getUnsigned(key);
  return default_value;
}

std::vector<uint8_t> NfcConfig::getBytes(const std::string& key) {
  return getInstance().config_.getBytes(key);
}

void NfcConfig::clear() { getInstance().config_.clear(); }
