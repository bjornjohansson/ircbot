#pragma once
#include "message.fwd.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

class Server;

typedef boost::function<void (Server&, const Irc::Message&)> ServerReceiver;
typedef boost::shared_ptr<ServerReceiver> ServerReceiverHandle;
