#include "regexp.hpp"
#include "../exception.hpp"

#include <vector>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>

const int REGCOMP_FLAGS = REG_EXTENDED | REG_ICASE;
const int MAX_SUB_MATCHES = 10;

RegExp::RegExp(const std::string& regExp, const std::string& reply)
    : regExp_(regExp)
    , reply_(reply)
{
    regex_t pattern;
    int regerr = regcomp(&pattern, regExp_.c_str(), REGCOMP_FLAGS);

    if ( regerr != 0 )
    {
	size_t size = regerror(regerr, &pattern, 0, 0);
	std::string errorMessage = "Could not compile regexp";
	if ( size < 1024 )
	{
	    std::vector<char> buffer(size, 0);
	    size = regerror(regerr, &pattern, &buffer[0],
			    static_cast<size_t>(buffer.size()));
	    errorMessage = &buffer[0];
	}
	throw Exception(__FILE__, __LINE__, errorMessage);
    }

    pattern_.reset(new RegExpPattern(pattern));
}

std::string RegExp::FindMatchAndReply(const std::string& message) const
{
    std::string result;
    if ( pattern_ )
    {
	typedef std::vector<regmatch_t> MatchContainer;
	MatchContainer matches(MAX_SUB_MATCHES);
	if ( regexec(pattern_->GetPattern(), message.c_str(),
		     static_cast<size_t>(matches.size()), &matches[0], 0) == 0)
	{
	    result = reply_;
	    for(unsigned int i = 0;
		i < matches.size() && matches[i].rm_so != -1;
		++i)
	    {
		std::stringstream ss;
		ss<<"\\"<<i;
		regoff_t start = matches[i].rm_so;
		regoff_t size = matches[i].rm_eo - matches[i].rm_so;
		std::string replacement = message.substr(start, size);

		for(std::string::size_type pos = result.find(ss.str());
		    pos != std::string::npos;
		    pos = result.find(ss.str(), pos+1))
		{
		    if ( pos == 0 || result[pos-1] != '\\' )
		    {
			result.replace(pos, ss.str().size(), replacement);
			pos += size-1;
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


RegExp::RegExpPattern::RegExpPattern(regex_t pattern)
    : pattern_(pattern)
{

}

RegExp::RegExpPattern::~RegExpPattern()
{
    regfree(&pattern_);
}

const regex_t* RegExp::RegExpPattern::GetPattern() const
{
    return &pattern_;
}
