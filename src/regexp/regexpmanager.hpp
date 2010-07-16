#pragma once

#include "regexp.fwd.hpp"

#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/shared_container_iterator.hpp>

#include <sys/types.h>
#include <regex.h>

#include <unicode/unistr.h>

class RegExpManager
{
public:
    struct RegExpResult
    {
	bool Success;
	UnicodeString ErrorMessage;
    };

    RegExpManager(const UnicodeString& regExpFile, const UnicodeString& locale);

    /**
     * Add regexp and save changes (no need to call SaveRegExps())
     */
    RegExpResult AddRegExp(const UnicodeString& regexp,
			   const UnicodeString& reply);
    /**
     * Remove regexp and save changes (no need to call SaveRegExps())
     */
    bool RemoveRegExp(const UnicodeString& regexp);

    bool SaveRegExps() const;

    /**
     * @param 1 reply
     * @param 2 original message
     * @param 3 matching regexp
     */
    typedef boost::function<UnicodeString (const UnicodeString&,
					 const UnicodeString&,
					 const RegExp&)> Operation;

    /**
     * Find a regexp that matches the message and create a reply.
     * The reply is then passed through the operation function.
     * If the operation functions returns a non-zero length string
     * then that string is returned and the matching is stopped.
     */
    UnicodeString FindMatchAndReply(const UnicodeString& message,
				  Operation operation) const;

    typedef std::vector<RegExp> RegExpContainer;
    typedef boost::shared_ptr<RegExpContainer> RegExpContainerPtr;
    typedef boost::shared_container_iterator<RegExpContainer> RegExpIterator;
    typedef std::pair<RegExpIterator,RegExpIterator> RegExpIteratorRange;

    RegExpIteratorRange FindMatches(const UnicodeString& message) const;
    RegExpIteratorRange FindRegExps(const UnicodeString& searchString) const;

    /**
     * Change reply and save changes (no need to call SaveRegExps())
     */
    bool ChangeReply(const UnicodeString& regexp, const UnicodeString& reply);
    /**
     * Change regexp and save changes (no need to call SaveRegExps())
     */
    RegExpResult ChangeRegExp(const UnicodeString& regexp,
			      const UnicodeString& newRegexp);

    bool MoveUp(const UnicodeString& regexp);
    bool MoveDown(const UnicodeString& regexp);

    // Decode regexp from its non-space syntax
    UnicodeString Decode(const UnicodeString& regexp) const;
    // Encode regexp into its non-space syntax
    UnicodeString Encode(const UnicodeString& regexp) const;

private:

    /**
     * Adds a regexp without updating the regexp file
     */
    RegExpResult AddRegExpImpl(const UnicodeString& regexp,
			       const UnicodeString& reply);

    std::locale locale_;
    UnicodeString regExpFile_;

    RegExpContainer regExps_;
};
