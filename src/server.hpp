#ifndef SERVER_HPP
#define SERVER_HPP

#include "server.fwd.hpp"

#include "message.hpp"

#include <unicode/unistr.h>

#include <string>
#include <list>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread.hpp>

class Server : boost::noncopyable
{
public:
    /**
     * @throw Exception if unable to connect
     */
    Server(const UnicodeString& id,
           const UnicodeString& host,
           unsigned int port,
           const std::string& logsDirectory,
           const UnicodeString& nick,
           const UnicodeString& serverPassword = UnicodeString());
    virtual ~Server();

    virtual void Connect() = 0;
    virtual void Send(const std::string& data) = 0;

    virtual const UnicodeString& GetId() const;
    virtual const UnicodeString& GetHostName() const;
    virtual unsigned int GetPort() const;
    virtual const UnicodeString& GetNick() const;

    typedef ServerReceiver Receiver;
    typedef ServerReceiverHandle ReceiverHandle;

    /**
     * Register a function as a receiver, the return value is a handle
     * to the receiver, whenever this handle ceases to exist the server
     * will no longer send data to the corresponding receiver.
     */
    ReceiverHandle RegisterReceiver(Receiver receiver);

    /**
     * Get path to log file for the given target which can be a channel or
     * a nick.
     */
    std::string GetLogName(const std::string& target) const;

    // Add a channel to the list of channels to maintain membership of.
    // This means the channel will be automatically joined on connecting
    // and when being removed from the channel.
    void AddChannel(const std::string& channel, const UnicodeString& key);

    // Communication methods
    virtual void JoinChannel(const std::string& channel,
                             const UnicodeString& key) = 0;
    virtual void ChangeNick(const UnicodeString& nick) = 0;
    virtual void SendMessage(const std::string& target,
                             const UnicodeString& message) = 0;
    virtual void Kick(const std::string& channel,
                      const std::string& user,
                      const UnicodeString& message) = 0;

    /**
     * @throw Exception if channel not found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel) const;

protected:
    void LogMessage(const std::string& target,
                    const std::string& text);
    void SetNick(const UnicodeString& nick);

    void JoinAllChannels();
    // Get the key for a given channel, returns NULL if there is no key stored
    // for this channel.
    const UnicodeString* GetChannelKey(const std::string& channel) const;

    void RemoveChannelNickChannel(const std::string& channel);
    void ClearAllChannelNicks(const std::string& channel);
    void AddChannelNick(const std::string& channel, const std::string& nick);
    void ChangeChannelNick(const std::string& oldNick, const std::string& newNick);
    // Remove nick from ALL channels
    void RemoveChannelNick(const std::string& nick);
    // Remove nick from specified channel
    void RemoveChannelNick(const std::string& channel, const std::string& nick);

    const UnicodeString& GetServerPassword() const;

    void NotifyReceiver(const Message& message);
protected:
    const UnicodeString& GetHost() const;

private:
    void ManageLogStreams();

    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    boost::shared_mutex receiversMutex_;

    UnicodeString id_;
    UnicodeString host_;
    unsigned int port_;
    UnicodeString nick_;
    UnicodeString serverPassword_;
    std::string logDirectory_;
    mutable boost::shared_mutex nickMutex_;

    typedef boost::shared_ptr<std::ofstream> OfstreamPtr;
    typedef std::pair<OfstreamPtr,time_t> OfstreamAndTimestamp;
    typedef std::map<std::string, OfstreamAndTimestamp> LogStreamMap;
    LogStreamMap logStreams_;
    boost::mutex logMutex_;

    typedef std::map<std::string, UnicodeString> ChannelKeyMap;
    ChannelKeyMap channels_;
    mutable boost::shared_mutex channelsMutex_;

    typedef std::map<std::string,std::set<std::string> > ChannelNicksContainer;
    ChannelNicksContainer channelNicks_;
    boost::shared_mutex channelNicksMutex_;
};

#endif
