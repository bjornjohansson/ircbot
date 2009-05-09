#pragma once

#include <string>

#include <boost/unordered_map.hpp>
#include <boost/assign/list_of.hpp>

namespace Irc
{

namespace Command
{
    enum Command
    {
	UNKNOWN_COMMAND, // Represents a command code that is unknown
	PASS,
	NICK,
	USER,
	SERVER,
	OPER,
	QUIT,
	SQUIT,
	JOIN,
	PART,
	MODE,
	TOPIC,
	NAMES,
	LIST,
	INVITE,
	KICK,
	VERSION,
	STATS,
	LINKS,
	TIME,
	CONNECT,
	TRACE,
	ADMIN,
	INFO,
	PRIVMSG,
	NOTICE,
	WHO,
	WHOIS,
	WHOWAS,
	KILL,
	PING,
	PONG,
	ERROR,
	ERR_NOSUCHNICK, 
	ERR_NOSUCHSERVER, 
	ERR_NOSUCHCHANNEL, 
	ERR_CANNOTSENDTOCHAN, 
	ERR_TOOMANYCHANNELS, 
	ERR_WASNOSUCHNICK, 
	ERR_TOOMANYTARGETS, 
	ERR_NOORIGIN, 
	ERR_NORECIPIENT, 
	ERR_NOTEXTTOSEND, 
	ERR_NOTOPLEVEL, 
	ERR_WILDTOPLEVEL, 
	ERR_UNKNOWNCOMMAND, 
	ERR_NOMOTD, 
	ERR_NOADMININFO, 
	ERR_FILEERROR, 
	ERR_NONICKNAMEGIVEN, 
	ERR_ERRONEUSNICKNAME, 
	ERR_NICKNAMEINUSE, 
	ERR_NICKCOLLISION, 
	ERR_USERNOTINCHANNEL, 
	ERR_NOTONCHANNEL, 
	ERR_USERONCHANNEL, 
	ERR_NOLOGIN, 
	ERR_SUMMONDISABLED, 
	ERR_USERSDISABLED, 
	ERR_NOTREGISTERED, 
	ERR_NEEDMOREPARAMS, 
	ERR_ALREADYREGISTRED, 
	ERR_NOPERMFORHOST, 
	ERR_PASSWDMISMATCH, 
	ERR_YOUREBANNEDCREEP, 
	ERR_KEYSET, 
	ERR_CHANNELISFULL, 
	ERR_UNKNOWNMODE, 
	ERR_INVITEONLYCHAN, 
	ERR_BANNEDFROMCHAN, 
	ERR_BADCHANNELKEY, 
	ERR_NOPRIVILEGES, 
	ERR_CHANOPRIVSNEEDED, 
	ERR_CANTKILLSERVER, 
	ERR_NOOPERHOST, 
	ERR_UMODEUNKNOWNFLAG, 
	ERR_USERSDONTMATCH, 
	RPL_NONE, 
	RPL_USERHOST, 
	RPL_ISON, 
	RPL_AWAY, 
	RPL_UNAWAY, 
	RPL_NOWAWAY, 
	RPL_WHOISUSER, 
	RPL_WHOISSERVER, 
	RPL_WHOISOPERATOR, 
	RPL_WHOISIDLE, 
	RPL_ENDOFWHOIS, 
	RPL_WHOISCHANNELS, 
	RPL_WHOWASUSER, 
	RPL_ENDOFWHOWAS, 
	RPL_LISTSTART, 
	RPL_LIST, 
	RPL_LISTEND, 
	RPL_CHANNELMODEIS, 
	RPL_NOTOPIC, 
	RPL_TOPIC, 
	RPL_INVITING, 
	RPL_SUMMONING, 
	RPL_VERSION, 
	RPL_WHOREPLY, 
	RPL_ENDOFWHO, 
	RPL_NAMREPLY, 
	RPL_ENDOFNAMES, 
	RPL_LINKS, 
	RPL_ENDOFLINKS, 
	RPL_BANLIST, 
	RPL_ENDOFBANLIST, 
	RPL_INFO, 
	RPL_ENDOFINFO, 
	RPL_MOTDSTART, 
	RPL_MOTD, 
	RPL_ENDOFMOTD, 
	RPL_YOUREOPER, 
	RPL_REHASHING, 
	RPL_TIME, 
	RPL_USERSSTART, 
	RPL_USERS, 
	RPL_ENDOFUSERS, 
	RPL_NOUSERS, 
	RPL_TRACELINK, 
	RPL_TRACECONNECTING, 
	RPL_TRACEHANDSHAKE, 
	RPL_TRACEUNKNOWN, 
	RPL_TRACEOPERATOR, 
	RPL_TRACEUSER, 
	RPL_TRACESERVER, 
	RPL_TRACENEWTYPE, 
	RPL_TRACELOG, 
	RPL_STATSLINKINFO, 
	RPL_STATSCOMMANDS, 
	RPL_STATSCLINE, 
	RPL_STATSNLINE, 
	RPL_STATSILINE, 
	RPL_STATSKLINE, 
	RPL_STATSYLINE, 
	RPL_ENDOFSTATS, 
	RPL_STATSLLINE, 
	RPL_STATSUPTIME, 
	RPL_STATSOLINE, 
	RPL_STATSHLINE, 
	RPL_UMODEIS, 
	RPL_LUSERCLIENT, 
	RPL_LUSEROP, 
	RPL_LUSERUNKNOWN, 
	RPL_LUSERCHANNELS, 
	RPL_LUSERME, 
	RPL_ADMINME, 
	RPL_ADMINLOC1, 
	RPL_ADMINLOC2, 
	RPL_ADMINEMAIL
    };

} // namespace Command

    // This maps incoming strings to the enum listed above
    typedef boost::unordered_map<std::string, Command::Command> CommandMap;
    static CommandMap Commands =
    boost::assign::map_list_of
    ("PASS", Command::PASS)
    ("NICK", Command::NICK)
    ("USER", Command::USER)
    ("SERVER", Command::SERVER)
    ("OPER", Command::OPER)
    ("QUIT", Command::QUIT)
    ("SQUIT", Command::SQUIT)
    ("JOIN", Command::JOIN)
    ("PART", Command::PART)
    ("MODE", Command::MODE)
    ("TOPIC", Command::TOPIC)
    ("NAMES", Command::NAMES)
    ("LIST", Command::LIST)
    ("INVITE", Command::INVITE)
    ("KICK", Command::KICK)
    ("VERSION", Command::VERSION)
    ("STATS", Command::STATS)
    ("LINKS", Command::LINKS)
    ("TIME", Command::TIME)
    ("CONNECT", Command::CONNECT)
    ("TRACE", Command::TRACE)
    ("ADMIN", Command::ADMIN)
    ("INFO", Command::INFO)
    ("PRIVMSG", Command::PRIVMSG)
    ("NOTICE", Command::NOTICE)
    ("WHO", Command::WHO)
    ("WHOIS", Command::WHOIS)
    ("WHOWAS", Command::WHOWAS)
    ("KILL", Command::KILL)
    ("PING", Command::PING)
    ("PONG", Command::PONG)
    ("ERROR", Command::ERROR)
    ("401", Command::ERR_NOSUCHNICK)
    ("402", Command::ERR_NOSUCHSERVER) 
    ("403", Command::ERR_NOSUCHCHANNEL) 
    ("404", Command::ERR_CANNOTSENDTOCHAN) 
    ("405", Command::ERR_TOOMANYCHANNELS) 
    ("406", Command::ERR_WASNOSUCHNICK) 
    ("407", Command::ERR_TOOMANYTARGETS) 
    ("409", Command::ERR_NOORIGIN) 
    ("411", Command::ERR_NORECIPIENT) 
    ("412", Command::ERR_NOTEXTTOSEND) 
    ("413", Command::ERR_NOTOPLEVEL) 
    ("414", Command::ERR_WILDTOPLEVEL) 
    ("421", Command::ERR_UNKNOWNCOMMAND) 
    ("422", Command::ERR_NOMOTD) 
    ("423", Command::ERR_NOADMININFO) 
    ("424", Command::ERR_FILEERROR) 
    ("431", Command::ERR_NONICKNAMEGIVEN) 
    ("432", Command::ERR_ERRONEUSNICKNAME) 
    ("433", Command::ERR_NICKNAMEINUSE) 
    ("436", Command::ERR_NICKCOLLISION) 
    ("441", Command::ERR_USERNOTINCHANNEL) 
    ("442", Command::ERR_NOTONCHANNEL) 
    ("443", Command::ERR_USERONCHANNEL) 
    ("444", Command::ERR_NOLOGIN) 
    ("445", Command::ERR_SUMMONDISABLED) 
    ("446", Command::ERR_USERSDISABLED) 
    ("451", Command::ERR_NOTREGISTERED) 
    ("461", Command::ERR_NEEDMOREPARAMS) 
    ("462", Command::ERR_ALREADYREGISTRED) 
    ("463", Command::ERR_NOPERMFORHOST) 
    ("464", Command::ERR_PASSWDMISMATCH) 
    ("465", Command::ERR_YOUREBANNEDCREEP) 
    ("467", Command::ERR_KEYSET) 
    ("471", Command::ERR_CHANNELISFULL) 
    ("472", Command::ERR_UNKNOWNMODE) 
    ("473", Command::ERR_INVITEONLYCHAN) 
    ("474", Command::ERR_BANNEDFROMCHAN) 
    ("475", Command::ERR_BADCHANNELKEY) 
    ("481", Command::ERR_NOPRIVILEGES) 
    ("482", Command::ERR_CHANOPRIVSNEEDED) 
    ("483", Command::ERR_CANTKILLSERVER) 
    ("491", Command::ERR_NOOPERHOST) 
    ("501", Command::ERR_UMODEUNKNOWNFLAG) 
    ("502", Command::ERR_USERSDONTMATCH) 
    ("300", Command::RPL_NONE) 
    ("302", Command::RPL_USERHOST) 
    ("303", Command::RPL_ISON) 
    ("301", Command::RPL_AWAY) 
    ("305", Command::RPL_UNAWAY) 
    ("306", Command::RPL_NOWAWAY) 
    ("311", Command::RPL_WHOISUSER) 
    ("312", Command::RPL_WHOISSERVER) 
    ("313", Command::RPL_WHOISOPERATOR) 
    ("317", Command::RPL_WHOISIDLE) 
    ("318", Command::RPL_ENDOFWHOIS) 
    ("319", Command::RPL_WHOISCHANNELS) 
    ("314", Command::RPL_WHOWASUSER) 
    ("369", Command::RPL_ENDOFWHOWAS) 
    ("321", Command::RPL_LISTSTART) 
    ("322", Command::RPL_LIST) 
    ("323", Command::RPL_LISTEND) 
    ("324", Command::RPL_CHANNELMODEIS) 
    ("331", Command::RPL_NOTOPIC) 
    ("332", Command::RPL_TOPIC) 
    ("341", Command::RPL_INVITING) 
    ("342", Command::RPL_SUMMONING) 
    ("351", Command::RPL_VERSION) 
    ("352", Command::RPL_WHOREPLY) 
    ("315", Command::RPL_ENDOFWHO) 
    ("353", Command::RPL_NAMREPLY) 
    ("366", Command::RPL_ENDOFNAMES) 
    ("364", Command::RPL_LINKS) 
    ("365", Command::RPL_ENDOFLINKS) 
    ("367", Command::RPL_BANLIST) 
    ("368", Command::RPL_ENDOFBANLIST) 
    ("371", Command::RPL_INFO) 
    ("374", Command::RPL_ENDOFINFO) 
    ("375", Command::RPL_MOTDSTART) 
    ("372", Command::RPL_MOTD) 
    ("376", Command::RPL_ENDOFMOTD) 
    ("381", Command::RPL_YOUREOPER) 
    ("382", Command::RPL_REHASHING) 
    ("391", Command::RPL_TIME) 
    ("392", Command::RPL_USERSSTART) 
    ("393", Command::RPL_USERS) 
    ("394", Command::RPL_ENDOFUSERS) 
    ("395", Command::RPL_NOUSERS) 
    ("200", Command::RPL_TRACELINK) 
    ("201", Command::RPL_TRACECONNECTING) 
    ("202", Command::RPL_TRACEHANDSHAKE) 
    ("203", Command::RPL_TRACEUNKNOWN) 
    ("204", Command::RPL_TRACEOPERATOR) 
    ("205", Command::RPL_TRACEUSER) 
    ("206", Command::RPL_TRACESERVER) 
    ("208", Command::RPL_TRACENEWTYPE) 
    ("261", Command::RPL_TRACELOG) 
    ("211", Command::RPL_STATSLINKINFO) 
    ("212", Command::RPL_STATSCOMMANDS) 
    ("213", Command::RPL_STATSCLINE) 
    ("214", Command::RPL_STATSNLINE) 
    ("215", Command::RPL_STATSILINE) 
    ("216", Command::RPL_STATSKLINE) 
    ("218", Command::RPL_STATSYLINE) 
    ("219", Command::RPL_ENDOFSTATS) 
    ("241", Command::RPL_STATSLLINE) 
    ("242", Command::RPL_STATSUPTIME) 
    ("243", Command::RPL_STATSOLINE) 
    ("244", Command::RPL_STATSHLINE) 
    ("221", Command::RPL_UMODEIS) 
    ("251", Command::RPL_LUSERCLIENT) 
    ("252", Command::RPL_LUSEROP) 
    ("253", Command::RPL_LUSERUNKNOWN) 
    ("254", Command::RPL_LUSERCHANNELS) 
    ("255", Command::RPL_LUSERME) 
    ("256", Command::RPL_ADMINME) 
    ("257", Command::RPL_ADMINLOC1) 
    ("258", Command::RPL_ADMINLOC2) 
    ("259", Command::RPL_ADMINEMAIL);

    inline std::string StringFromCommand(const Command::Command& command)
    {
	for(CommandMap::iterator i = Commands.begin();i != Commands.end();++i)
	{
	    if ( i->second == command )
	    {
		return i->first;
	    }
	}
	return "UNKNOWN_COMMAND";
    }

} // namespace Irc
