#pragma once

#include <locale>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex/icu.hpp>

#include <unicode/unistr.h>

class RegExp
{
public:
    /**
     * @throw Exception if the regexp cannot be compiled
     */
    RegExp(const UnicodeString& regExp,
	   const UnicodeString& reply,
	   const std::locale& locale = std::locale());

    UnicodeString FindMatchAndReply(const UnicodeString& message) const;
    
    const UnicodeString& GetRegExp() const;
    const UnicodeString& GetReply() const;

    void SetReply(const UnicodeString& reply);
private:
    UnicodeString regExp_, reply_;

    boost::u32regex pattern_;
};
