#include "regexp.hpp"
#include "../exception.hpp"

#include <vector>
#include <sstream>
#include <iostream>

#include <boost/algorithm/string/replace.hpp>

#include <converter.hpp>

const boost::regex::flag_type REGEX_FLAGS = boost::regex::extended
		| boost::regex::icase;
const int MAX_SUB_MATCHES = 10;

RegExp::RegExp(const UnicodeString& regExp, const UnicodeString& reply,
		const std::locale& locale) :
	regExp_(regExp), reply_(reply)
{
	try
	{
		// pattern_.imbue(locale);
		pattern_ = boost::make_u32regex(regExp, REGEX_FLAGS);
	} catch (boost::regex_error& e)
	{
		UnicodeString errorMessage = "Could not compile regexp";
		if (e.what())
		{
			errorMessage = e.what();
		}
		throw Exception(__FILE__, __LINE__, errorMessage);
	}
}

UnicodeString RegExp::FindMatchAndReply(const UnicodeString& message) const
{
	UnicodeString result;

	if (!pattern_.empty())
	{
		boost::u16match matches;
		if (boost::u32regex_search(message, matches, pattern_))
		{
			result = reply_;
			for (unsigned int i = 0; i < matches.size(); ++i)
			{
				std::stringstream ss;
				ss << '\\' << i;
				UnicodeString index = AsUnicode(ss.str());

				for (int32_t pos = result.indexOf(index);
				     pos != -1; pos = result.indexOf(index, pos + 1))
				{
					if (pos == 0 || result[pos - 1] != '\\')
					{
						result.replace(pos, index.length(), matches[i].str().c_str());
						pos += matches[i].str().size() - 1;
					}
					else if (pos > 0 && result[pos - 1] == '\\')
					{
						result.remove(pos - 1, 1);
					}
				}
			}
		}
	}
	return result;
}

const UnicodeString& RegExp::GetRegExp() const
{
	return regExp_;
}

const UnicodeString& RegExp::GetReply() const
{
	return reply_;
}

void RegExp::SetReply(const UnicodeString& reply)
{
	reply_ = reply;
}
