#include "regexpmanager.hpp"
#include "regexp.hpp"
#include "../exception.hpp"
#include "../logging/logger.hpp"

#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <regex.h>

RegExpManager::RegExpManager(const UnicodeString& regExpFile,
		const UnicodeString& locale) :
	locale_(AsUtf8(locale).c_str()), regExpFile_(regExpFile)
{
	std::ifstream fin(AsUtf8(regExpFile).c_str());

	std::string line;
	while (std::getline(fin, line))
	{
		std::istringstream ss(line);
		std::string regexp, reply;
		std::getline(ss, regexp, ' ');
		std::getline(ss, reply);
		RegExpResult result = AddRegExpImpl(ConvertString(regexp), ConvertString(reply));
		if (!result.Success)
		{
			Log << LogLevel::Error << "'" << regexp << "': "
					<< result.ErrorMessage;
		}
	}
}

RegExpManager::RegExpResult RegExpManager::AddRegExp(
		const UnicodeString& regexp, const UnicodeString& reply)
{
	RegExpResult result = AddRegExpImpl(regexp, reply);

	if (result.Success)
	{
		SaveRegExps();
	}
	return result;
}

bool RegExpManager::RemoveRegExp(const UnicodeString& regexp)
{
	const UnicodeString decodedRegExp = Decode(regexp);
	for (RegExpContainer::iterator i = regExps_.begin(); i != regExps_.end(); ++i)
	{
		if (i->GetRegExp() == decodedRegExp)
		{
			regExps_.erase(i);
			SaveRegExps();
			return true;
		}
	}
	return false;
}

bool RegExpManager::SaveRegExps() const
{
	std::ofstream fout(AsUtf8(regExpFile_).c_str());

	if (fout.good())
	{
		for (RegExpContainer::const_iterator i = regExps_.begin(); i
				!= regExps_.end() && fout.good(); ++i)
		{
			fout << AsUtf8(Encode(i->GetRegExp())) << " " << AsUtf8(i->GetReply()) << std::endl;
		}
		return fout.good();
	}
	return false;
}

UnicodeString RegExpManager::FindMatchAndReply(const UnicodeString& message,
		Operation operation) const
{
	UnicodeString result = "";
	for (RegExpContainer::const_iterator i = regExps_.begin(); i
			!= regExps_.end(); ++i)
	{
		result = i->FindMatchAndReply(message);
		if (result.length() > 0)
		{
			result = operation(result, message, *i);
			if (result.length() > 0)
			{
				break;
			}
		}
	}
	return result;
}

RegExpManager::RegExpIteratorRange RegExpManager::FindMatches(
		const UnicodeString& message) const
{
	RegExpContainerPtr matches(new RegExpContainer());

	for (RegExpContainer::const_iterator i = regExps_.begin(); i
			!= regExps_.end(); ++i)
	{
		UnicodeString result = i->FindMatchAndReply(message);
		if (result.length() > 0)
		{
			matches->push_back(*i);
		}
	}
	return boost::make_shared_container_range(matches);
}

RegExpManager::RegExpIteratorRange RegExpManager::FindRegExps(
		const UnicodeString& searchString) const
{
	RegExpContainerPtr matches(new RegExpContainer());

	// Use the regexp class to find regexps, bit of a hack but saves coding
	RegExp regExp(searchString, "found", locale_);

	for (RegExpContainer::const_iterator i = regExps_.begin(); i
			!= regExps_.end(); ++i)
	{
		if (regExp.FindMatchAndReply(i->GetRegExp()).length() > 0
				|| regExp.FindMatchAndReply(i->GetReply()).length() > 0)
		{
			matches->push_back(*i);
		}
	}
	return boost::make_shared_container_range(matches);
}

bool RegExpManager::ChangeReply(const UnicodeString& regexp,
		const UnicodeString& reply)
{
	UnicodeString decodedRegExp = Decode(regexp);
	for (RegExpContainer::iterator i = regExps_.begin(); i != regExps_.end(); ++i)
	{
		if (i->GetRegExp() == decodedRegExp)
		{
			i->SetReply(reply);
			SaveRegExps();
			return true;
		}
	}
	return false;
}

RegExpManager::RegExpResult RegExpManager::ChangeRegExp(
		const UnicodeString& regexp, const UnicodeString& newRegexp)
{
	UnicodeString decodedRegExp = Decode(regexp);
	UnicodeString decodedNewRegExp = Decode(newRegexp);
	RegExpResult result;
	result.Success = false;
	result.ErrorMessage = "No matching regexp";
	for (RegExpContainer::iterator i = regExps_.begin(); i != regExps_.end(); ++i)
	{
		if (i->GetRegExp() == decodedRegExp)
		{
			try
			{
				*i = RegExp(decodedNewRegExp, i->GetReply(), locale_);
				SaveRegExps();
				result.Success = true;
				break;
			} catch (Exception& e)
			{
				result.ErrorMessage = e.GetMessage();
				result.Success = false;
			}
		}
	}
	return result;

}

bool RegExpManager::MoveUp(const UnicodeString& regexp)
{
	for (RegExpContainer::iterator i = regExps_.begin(); i != regExps_.end(); ++i)
	{
		if (i->GetRegExp() == regexp)
		{
			RegExp r = *i;
			regExps_.erase(i);
			regExps_.insert(regExps_.begin(), r);
			SaveRegExps();
			return true;
		}
	}
	return false;
}

bool RegExpManager::MoveDown(const UnicodeString& regexp)
{
	for (RegExpContainer::iterator i = regExps_.begin(); i != regExps_.end(); ++i)
	{
		if (i->GetRegExp() == regexp)
		{
			RegExp r = *i;
			regExps_.erase(i);
			regExps_.push_back(r);
			SaveRegExps();
			return true;
		}
	}
	return false;
}

UnicodeString RegExpManager::Decode(const UnicodeString& regexp) const
{
	UnicodeString result = regexp;
	int32_t pos = 0;

	while ((pos = result.indexOf('+', pos)) != -1)
	{
		// If the + is not preceeded by a \ we replace it with a ' '
		// Otherwise we remove the \ and leave the +
		if (pos == 0 || result[pos - 1] != '\\')
		{
			result.setCharAt(pos, L' ');
		}
		else
		{
			result.remove(pos - 1, 1);
		}
		++pos;
	}
	return result;
}

UnicodeString RegExpManager::Encode(const UnicodeString& regexp) const
{
	UnicodeString result = regexp;
	int32_t pos = 0;

	// Replace + with \+
	while ((pos = result.indexOf('+', pos)) != -1)
	{
		result.insert(pos, '\\');
		pos += 2;
	}
	// Replace ' ' with +
	while ((pos = result.indexOf(' ')) != -1)
	{
		result.setCharAt(pos, '+');
	}
	return result;
}

RegExpManager::RegExpResult RegExpManager::AddRegExpImpl(
		const UnicodeString& regexp, const UnicodeString& reply)
{
	RegExpResult result;

	try
	{
		RegExp r(Decode(regexp), reply, locale_);
		regExps_.push_back(r);
		result.Success = true;
	} catch (Exception& e)
	{
		result.Success = false;
		result.ErrorMessage = e.GetMessage();
	}

	return result;
}
