#ifndef SERVER_CONFIG_CONFIG_VALIDATOR_H_
#define SERVER_CONFIG_CONFIG_VALIDATOR_H_

#include "server/config/config_model.h"

namespace server {

class ConfigValidator {
 public:
  static bool Validate(const GameConfig& config);
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_VALIDATOR_H_
