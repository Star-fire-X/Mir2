#ifndef MIR2_SERVER_CONFIG_CONFIG_MANAGER_H_
#define MIR2_SERVER_CONFIG_CONFIG_MANAGER_H_

namespace server {

class ConfigManager {
 public:
  ConfigManager() = default;

  bool IsLoaded() const;

 private:
  bool is_loaded_ = false;
};

}  // namespace server

#endif  // MIR2_SERVER_CONFIG_CONFIG_MANAGER_H_
