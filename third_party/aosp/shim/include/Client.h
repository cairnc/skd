// SF Client shim — LayerCreationArgs has `wp<Client>` but nothing in the
// replay path creates real clients.
#pragma once
#include <utils/RefBase.h>
namespace android {
class Client : public RefBase {};
} // namespace android
