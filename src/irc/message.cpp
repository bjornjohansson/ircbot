#include "message.hpp"
#include "../exception.hpp"

#include <sstream>

#include <boost/regex.hpp>

boost::regex Irc::Message::dataRegex_;

Irc::Message::Message(const std::string& data)
{
    try
    {
	if ( dataRegex_.empty() )
	{
	    // First match prefix, then command, then non-space parameters
	    // and finally any last parameter preceeded by colon
	    dataRegex_ = "(?::([^ ]+) )?([^ ]+)(?: ([^:]+))?(?: :(.*))?";
	}

	boost::smatch matches;
	if ( boost::regex_match(data, matches, dataRegex_) )
	{
	    // matches[0] is the entire match, submatches start at 1
	    prefix_ = Prefix(matches[1]);
	    command_ = matches[2];

	    // The non-space parameters are matched as an entire string.
	    // Since boost might not support captures for each match
	    // we will have to split this string ourselves
	    std::stringstream stream(matches[3]);
	    std::copy(std::istream_iterator<std::string>(stream),
		      std::istream_iterator<std::string>(),
		      std::back_inserter(parameters_));

	    // Last parameter if any
	    if ( matches[4].matched )
	    {
		parameters_.push_back(matches[4]);
	    }
	}
    }
    catch ( boost::regex_error& e )
    {
	throw Exception(__FILE__, __LINE__, e.what());
    }
}
