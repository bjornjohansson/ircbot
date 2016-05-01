#include "regexp.hpp"
#include "../exception.hpp"
#include "../logging/logger.hpp"

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
		const std::locale&) :
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
                  const UnicodeString& match,
                  boost::function<UnicodeString (const UnicodeString&)> modifier
                  = boost::function<UnicodeString (const UnicodeString&)>())
{
	for (int32_t pos = result.indexOf(index);
	     pos != -1;
	     pos = result.indexOf(index, pos + 1))
	{
		if (pos == 0 || result[pos - 1] != '\\')
		{
			UnicodeString replacement = match;
			if (modifier)
			{
				replacement = modifier(replacement);
			}
			result.replace(pos, index.length(), replacement);
			
			pos += replacement.length() - 1;
		}
		else if (pos > 0 && result[pos - 1] == '\\')
		{
			result.remove(pos - 1, 1);
		}
	}
}

void EscapeCharacter(UnicodeString& input,
                     UChar character,
                     const UnicodeString& quote)
{
	int32_t quoteLength = quote.length();
	for (int32_t pos = input.indexOf(character);
	     pos != -1;
	     pos = input.indexOf(character, pos))
	{
		input.insert(pos, quote);
		pos += quoteLength + 1;
	}
}

UnicodeString QuoteBashString(const UnicodeString& input)
{
	UnicodeString result = input;
	EscapeCharacter(result, L'\\', "\\");
	EscapeCharacter(result, L'"', "\\");
	EscapeCharacter(result, L'$', "\\");
	EscapeCharacter(result, L'`', "\\\\");

	return result;
}

UnicodeString RegExp::FindMatchAndReply(const UnicodeString& message) const
{
	UnicodeString result;

	if (!pattern_.empty())
	{
		boost::u16match matches;
        try
        {
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
                    UnicodeString match(matches[i].str().c_str());

                    ReplaceIndex(result, unquotedIndex, match);
                    ReplaceIndex(result, quotedIndex, match, &QuoteBashString);
                }
            }
        }
        catch (std::out_of_range& e)
        {
            // out_of_range is thrown when the message contains invalid
            // unicode code points. Don't crash when that happens, just log
            // it and move on.
            Log << LogLevel::Error << "Exception caught matching regexp '"
                << AsUtf8(regExp_) << "' against message '" << AsUtf8(message)
                << "': " << e.what();
            return UnicodeString();
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
