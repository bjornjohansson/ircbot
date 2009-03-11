#include "regexp.hpp"
#include "../exception.hpp"

#include <vector>
#include <sstream>
#include <iostream>

#include <boost/algorithm/string/replace.hpp>

const boost::regex::flag_type REGEX_FLAGS =
    boost::regex::extended|boost::regex::icase;
const int MAX_SUB_MATCHES = 10;

RegExp::RegExp(const std::string& regExp,
	       const std::string& reply,
	       const std::locale& locale)
    : regExp_(regExp)
    , reply_(reply)
{
    try
    {
	pattern_.imbue(locale);
	pattern_.assign(regExp, REGEX_FLAGS);
    }
    catch ( boost::regex_error& e )
    {
	std::string errorMessage = "Could not compile regexp";
	if ( e.what() )
	{
	    errorMessage = e.what();
	}
	throw Exception(__FILE__, __LINE__, errorMessage);
    }
}

std::string RegExp::FindMatchAndReply(const std::string& message) const
{
    std::string result;

    if ( !pattern_.empty() )
    {
	boost::smatch matches;
	if ( boost::regex_search(message, matches, pattern_) )
	{
	    result = reply_;
	    for(unsigned int i = 0; i < matches.size(); ++i)
	    {
		std::stringstream ss;
		ss<<"\\"<<i;

		for(std::string::size_type pos = result.find(ss.str());
		    pos != std::string::npos;
		    pos = result.find(ss.str(), pos+1))
		{
		    if ( pos == 0 || result[pos-1] != '\\' )
		    {
			result.replace(pos, ss.str().size(), matches[i].str());
			pos += matches[i].str().size()-1;
		    }
		    else if ( pos > 0 && result[pos-1] == '\\' )
		    {
			result.erase(pos-1, 1);
		    }
		}
	    }
	}
    }
    return result;
}

const std::string& RegExp::GetRegExp() const
{
    return regExp_;
}

const std::string& RegExp::GetReply() const
{
    return reply_;
}

void RegExp::SetReply(const std::string& reply)
{
    reply_ = reply;
}
