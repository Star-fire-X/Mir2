#ifndef CLIENT_VIEW_DROP_VIEW_H_
#define CLIENT_VIEW_DROP_VIEW_H_

#include "client/view/entity_view.h"

namespace client {

class DropView : public EntityView {
 public:
  explicit DropView(const shared::VisibleEntitySnapshot& snapshot)
      : EntityView(snapshot.entity_id, snapshot.position,
                   EntityViewKind::kDrop) {}
};

}  // namespace client

#endif  // CLIENT_VIEW_DROP_VIEW_H_
