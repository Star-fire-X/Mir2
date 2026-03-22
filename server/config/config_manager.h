#ifndef SERVER_CONFIG_CONFIG_MANAGER_H_
#define SERVER_CONFIG_CONFIG_MANAGER_H_

namespace server {

class ConfigManager {
 public:
  ConfigManager() = default;

  bool IsLoaded() const;

 private:
  bool is_loaded_ = false;
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_MANAGER_H_
