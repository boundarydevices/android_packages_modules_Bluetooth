/*
 * Copyright 2020 The Android Open Source Project
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

#include "sysprops/sysprops_module.h"

#include <filesystem>

#include "os/handler.h"
#include "os/log.h"
#include "os/parameter_provider.h"
#include "os/system_properties.h"
#include "storage/legacy_config_file.h"

namespace bluetooth {
namespace sysprops {

static const size_t kDefaultCapacity = 10000;

const ModuleFactory SyspropsModule::Factory = ModuleFactory([]() { return new SyspropsModule(); });

struct SyspropsModule::impl {
  impl(os::Handler* sysprops_handler) : sysprops_handler_(sysprops_handler) {}

  os::Handler* sysprops_handler_;
};

void SyspropsModule::ListDependencies(ModuleList* list) const {}

void SyspropsModule::Start() {
  std::string file_path = os::ParameterProvider::SyspropsFilePath();
  if (!file_path.empty()) {
    parse_config(file_path);
    // Merge config fragments
    std::string override_dir = file_path + ".d";
    if (std::filesystem::exists(override_dir)) {
      for (const auto& entry : std::filesystem::directory_iterator(override_dir)) {
        parse_config(entry.path());
      }
    }
  }

  pimpl_ = std::make_unique<impl>(GetHandler());
}

void SyspropsModule::Stop() {
  pimpl_.reset();
}

std::string SyspropsModule::ToString() const {
  return "Sysprops Module";
}

void SyspropsModule::parse_config(std::string file_path) {
  const std::list<std::string> supported_sysprops = {"bluetooth.core.gap.le.privacy.enabled"};

  auto config = storage::LegacyConfigFile::FromPath(file_path).Read(kDefaultCapacity);
  if (!config) {
    return;
  }

  for (auto s = supported_sysprops.begin(); s != supported_sysprops.end(); s++) {
    auto str = config->GetProperty("Sysprops", *s);
    if (str) {
      bluetooth::os::SetSystemProperty(*s, *str);
    }
  }
}

}  // namespace sysprops
}  // namespace bluetooth
