#ifndef CLIENT_VIEW_PLAYER_VIEW_H_
#define CLIENT_VIEW_PLAYER_VIEW_H_

#include "client/view/entity_view.h"

namespace client {

class PlayerView : public EntityView {
 public:
  explicit PlayerView(const shared::VisibleEntitySnapshot& snapshot)
      : EntityView(snapshot.entity_id, snapshot.position,
                   EntityViewKind::kPlayer) {}
};

}  // namespace client

#endif  // CLIENT_VIEW_PLAYER_VIEW_H_
