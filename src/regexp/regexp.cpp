#include "regexp.hpp"
#include "../exception.hpp"

#include <vector>
#include <sstream>
#include <iostream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/function.hpp>

#include <converter.hpp>

typedef std::basic_string<uint16_t> ustring;

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

void ReplaceIndex(UnicodeString& result,
                  const UnicodeString& index,
                  const ustring& match,
                  boost::function<ustring (const ustring&)> modifier
                  = boost::function<ustring (const ustring&)>())
{
	for (int32_t pos = result.indexOf(index);
	     pos != -1;
	     pos = result.indexOf(index, pos + 1))
	{
		if (pos == 0 || result[pos - 1] != '\\')
		{
			ustring replacement = match;
			if (modifier)
			{
				replacement = modifier(replacement);
			}
			result.replace(pos,
			               index.length(),
			               reinterpret_cast<const uint16_t*>(replacement.c_str()));
			
			pos += replacement.size() - 1;
		}
		else if (pos > 0 && result[pos - 1] == '\\')
		{
			result.remove(pos - 1, 1);
		}
	}
}

void EscapeCharacter(ustring& input, uint16_t character)
{
	size_t pos = 0;
	while ((pos=input.find(character, pos)) != ustring::npos)
	{
		input.insert(pos, 1, L'\\');
		pos += 2;
	}
	
}

ustring QuoteBashString(const ustring& input)
{
	ustring result = input;
	EscapeCharacter(result, L'\\');
	EscapeCharacter(result, L'"');
	EscapeCharacter(result, L'$');
	EscapeCharacter(result, L'`');
	
	return result;
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
				std::stringstream unquoted, quoted;
				unquoted << '\\' << i;
				quoted << '@' << i;
				UnicodeString unquotedIndex = AsUnicode(unquoted.str());
				UnicodeString quotedIndex = AsUnicode(quoted.str());

				ReplaceIndex(result, unquotedIndex, matches[i].str());
				ReplaceIndex(result, quotedIndex, matches[i].str(), &QuoteBashString);
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
