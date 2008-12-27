#pragma once

#include <sys/types.h>
#include <regex.h>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

class RegExp
{
public:
    /**
     * @throw Exception if the regexp cannot be compiled
     */
    RegExp(const std::string& regExp, const std::string& reply);

    std::string FindMatchAndReply(const std::string& message) const;
    
    const std::string& GetRegExp() const;
    const std::string& GetReply() const;

    void SetReply(const std::string& reply);
private:
    class RegExpPattern : boost::noncopyable
    {
    public:
	RegExpPattern(regex_t pattern);
	virtual ~RegExpPattern();

	const regex_t* GetPattern() const;
    private:
	regex_t pattern_;
    };

    std::string regExp_, reply_;

    typedef boost::shared_ptr<RegExpPattern> RegExpPatternPtr;
    RegExpPatternPtr pattern_;
};
