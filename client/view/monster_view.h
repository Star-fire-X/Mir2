#ifndef CLIENT_VIEW_MONSTER_VIEW_H_
#define CLIENT_VIEW_MONSTER_VIEW_H_

#include "client/view/entity_view.h"

namespace client {

class MonsterView : public EntityView {
 public:
  explicit MonsterView(const shared::VisibleEntitySnapshot& snapshot)
      : EntityView(snapshot.entity_id, snapshot.position,
                   EntityViewKind::kMonster) {}
};

}  // namespace client

#endif  // CLIENT_VIEW_MONSTER_VIEW_H_
