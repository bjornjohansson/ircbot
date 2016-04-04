#ifndef IRC_SERVER_HPP
#define IRC_SERVER_HPP

#include "../connection/connection.hpp"
#include "../server.hpp"

#include <charsetdetector.hpp>
#include <unicode/unistr.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

namespace Irc {

class IrcServer : public Server
{
public:
    /**
     * @throw Exception if unable to connect
     */
    IrcServer(const UnicodeString& id,
              const UnicodeString& host,
              unsigned int port,
              const std::string& logsDirectory,
              const UnicodeString& nick,
              const UnicodeString& serverPassword = UnicodeString());
    virtual ~IrcServer();

    virtual void Connect();
    virtual void Send(const std::string& data);

    // Irc methods
    virtual void JoinChannel(const std::string& channel,
                             const UnicodeString& key);
    virtual void ChangeNick(const UnicodeString& nick);
    virtual void SendMessage(const std::string& target,
                             const UnicodeString& message);
    virtual void Kick(const std::string& channel,
                      const std::string& user,
                      const UnicodeString& message);

    /**
     * @throw Exception if channel not found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel) const;

private:
    void SendPassString(const UnicodeString& password);
    void SendUserString(const UnicodeString& user, const UnicodeString& name);

    void Receive(Connection& connection, const std::vector<char>& data);
    void OnConnect(Connection& connection);

    void OnText(const std::string& text);

    void RegisterSelfAsReceiver();

    std::string CleanMessageForDisplay(const std::string& nick,
                                       const std::string& message,
                                       bool isCtcp);

    Connection connection_;

    Connection::ReceiverHandle connectionReceiver_;
    Connection::OnConnectHandle onConnectCallback_;

    std::string receiveBuffer_;
    boost::mutex callbackMutex_;

    CharsetDetector detector_;
    bool appendingChannelNicks_;
};

}  // namespace Irc

#endif
