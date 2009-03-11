#include "regexpmanager.hpp"
#include "regexp.hpp"
#include "../exception.hpp"

#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <regex.h>

#include <iostream>

RegExpManager::RegExpManager(const std::string& regExpFile)
    : regExpFile_(regExpFile)
{
    std::ifstream fin(regExpFile.c_str());

    std::string line;
    while ( std::getline(fin, line) )
    {
	std::istringstream ss(line);
	std::string regexp, reply;
	std::getline(ss, regexp, ' ');
	std::getline(ss, reply);
	RegExpResult result = AddRegExpImpl(regexp, reply);
	if ( !result.Success )
	{
	    std::cerr<<"'"<<regexp<<"': "<<result.ErrorMessage<<std::endl;
	}
    }
}

RegExpManager::RegExpResult RegExpManager::AddRegExp(const std::string& regexp,
						     const std::string& reply)
{
    RegExpResult result = AddRegExpImpl(regexp, reply);

    if ( result.Success )
    {
	SaveRegExps();
    }
    return result;
}

bool RegExpManager::RemoveRegExp(const std::string& regexp)
{
    const std::string decodedRegExp = Decode(regexp);
    for(RegExpContainer::iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if ( i->GetRegExp() == decodedRegExp )
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
    std::ofstream fout(regExpFile_.c_str());

    if ( fout.good() )
    {
	for(RegExpContainer::const_iterator i = regExps_.begin();
	    i != regExps_.end() && fout.good();
	    ++i)
	{
	    fout<<Encode(i->GetRegExp())<<" "<<i->GetReply()<<std::endl;
	}
	return fout.good();
    }
    return false;
}

std::string RegExpManager::FindMatchAndReply(const std::string& message,
					     Operation operation) const
{
    std::string result = "";
    for(RegExpContainer::const_iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	result = i->FindMatchAndReply(message);
	if ( result.size() > 0 )
	{
	    result = operation(result, message, *i);
	    if ( result.size() > 0 )
	    {
		break;
	    }
	}
    }
    return result;
}

RegExpManager::RegExpIteratorRange
RegExpManager::FindMatches(const std::string& message) const
{
    RegExpContainerPtr matches(new RegExpContainer());

    for(RegExpContainer::const_iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	std::string result = i->FindMatchAndReply(message);
	if ( result.size() > 0 )
	{
	    matches->push_back(*i);
	}
    }
    return boost::make_shared_container_range(matches);
}

RegExpManager::RegExpIteratorRange
RegExpManager::FindRegExps(const std::string& searchString) const
{
    RegExpContainerPtr matches(new RegExpContainer());

    // Use the regexp class to find regexps, bit of a hack but saves coding
    RegExp regExp(searchString, "found");

    for(RegExpContainer::const_iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if (regExp.FindMatchAndReply(i->GetRegExp()).size() > 0 ||
	    regExp.FindMatchAndReply(i->GetReply()).size() > 0 )
	{
	    matches->push_back(*i);
	}
    }
    return boost::make_shared_container_range(matches);
}

bool RegExpManager::ChangeReply(const std::string& regexp,
				const std::string& reply)
{
    std::string decodedRegExp = Decode(regexp);
    for(RegExpContainer::iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if ( i->GetRegExp() == decodedRegExp )
	{
	    i->SetReply(reply);
	    SaveRegExps();
	    return true;
	}
    }
    return false;
}

RegExpManager::RegExpResult
RegExpManager::ChangeRegExp(const std::string& regexp,
			    const std::string& newRegexp)
{
    std::string decodedRegExp = Decode(regexp);
    std::string decodedNewRegExp = Decode(newRegexp);
    RegExpResult result;
    result.Success = false;
    result.ErrorMessage = "No matching regexp";
    for(RegExpContainer::iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if ( i->GetRegExp() == decodedRegExp )
	{
	    try
	    {
		*i = RegExp(decodedNewRegExp, i->GetReply());
		SaveRegExps();
		result.Success = true;
		break;
	    }
	    catch ( Exception& e )
	    {
		result.ErrorMessage = e.GetMessage();
		result.Success = false;
	    }
	}
    }
    return result;

}

bool RegExpManager::MoveUp(const std::string& regexp)
{
    for(RegExpContainer::iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if ( i->GetRegExp() == regexp )
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

bool RegExpManager::MoveDown(const std::string& regexp)
{
    for(RegExpContainer::iterator i = regExps_.begin();
	i != regExps_.end();
	++i)
    {
	if ( i->GetRegExp() == regexp )
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

std::string RegExpManager::Decode(const std::string& regexp) const
{
    std::string result = regexp;
    std::string::size_type pos = 0;
 
    while ( (pos = result.find('+', pos)) != std::string::npos )
    {
	// If the + is not preceeded by a \ we replace it with a ' '
	// Otherwise we remove the \ and leave the +
	if (pos == 0 || result[pos-1] != '\\')
	{
            result[pos] =  ' ';
	}
	else
	{
            result.erase(pos-1,1);
	}
	++pos;
    }
    return result;
}

std::string RegExpManager::Encode(const std::string& regexp) const
{
    std::string result = regexp;
    std::string::size_type pos = 0;

    // Replace + with \+
    while ( (pos = result.find('+',pos)) != std::string::npos )
    {
	result.insert(pos,1,'\\');
	pos += 2;
    }
    // Replace ' ' with +
    while ( (pos = result.find(' ')) != std::string::npos )
    {
        result[pos] = '+';
    }
    return result;
}

RegExpManager::RegExpResult
RegExpManager::AddRegExpImpl(const std::string& regexp,
			     const std::string& reply)
{
    RegExpResult result;

    try
    {
	RegExp r(Decode(regexp), reply);
	regExps_.push_back(r);
	result.Success = true;
    }
    catch ( Exception& e )
    {
	result.Success = false;
	result.ErrorMessage = e.GetMessage();
    }

    return result;
}
