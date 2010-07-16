#include "message.hpp"
#include "../exception.hpp"

#include <sstream>

#include <boost/regex/icu.hpp>

boost::regex Irc::Message::dataRegex_;

Irc::Message::Message(const std::string& data)
	: isCtcp_(false)
{
	try
	{
		if (dataRegex_.empty())
		{
			// First match prefix, then command, then non-space parameters
			// and finally any last parameter preceeded by colon
			dataRegex_ = boost::regex(
					"(?::([^ ]+) )?([^ ]+)(?: ([^:]+))?(?: :(.*))?");
		}

		boost::smatch matches;
		if (boost::regex_match(data, matches, dataRegex_))
		{
			// matches[0] is the entire match, submatches start at 1
			prefix_ = Prefix(matches[1].str());
			try
			{
				command_ = Commands.at(matches[2].str());
			}
			catch (std::out_of_range&)
			{
				command_ = Command::UNKNOWN_COMMAND;
			}

			// The non-space parameters are matched as an entire string.
			// Since boost might not support captures for each match
			// we will have to split this string ourselves
			std::stringstream stream(matches[3]);
			std::copy(std::istream_iterator<std::string>(stream),
					std::istream_iterator<std::string>(), std::back_inserter(
							parameters_));

			// Last parameter if any
			if (matches[4].matched)
			{
				parameters_.push_back(matches[4]);
			}
		    CheckCtcp();
		}
	} catch (boost::regex_error& e)
	{
		throw Exception(__FILE__, __LINE__, e.what());
	}
}

const std::string& Irc::Message::GetTarget() const
{
	if (GetCommand() == Command::PRIVMSG)
	{
		return parameters_[0];
	}
	throw Exception(__FILE__, __LINE__, AsUnicode(
			"Invalid call to Irc::Message::GetTarget() for message of type "
			+ StringFromCommand(GetCommand())));
}

const std::string& Irc::Message::GetText() const
{
	if (GetCommand() == Command::PRIVMSG)
	{
		return parameters_[1];
	}
	throw Exception(__FILE__, __LINE__, AsUnicode(
			"Invalid call to Irc::Message::GetText() for message of type "
			+ StringFromCommand(GetCommand())));
}

const std::string& Irc::Message::GetReplyTo() const
{
	// If the message was sent to a channel we reply to that channel.
	// If it was sent directly to us we reply to the sender
	if (GetTarget().find_first_of("&#") == 0)
	{
		return GetTarget();
	}
	return GetPrefix().GetNick();
}

void Irc::Message::CheckCtcp()
{
    if(command_ == Irc::Command::PRIVMSG && parameters_.size() >= 2 && parameters_[1].find("\1") == 0){
        isCtcp_ = true;
        parameters_[1].erase(0, 1);
        std::string::size_type pos = parameters_[1].rfind("\1");
        if(pos == parameters_[1].size() - 1){
            parameters_[1].erase(pos);
        }
    }

}
