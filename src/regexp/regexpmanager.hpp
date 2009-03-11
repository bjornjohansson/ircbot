#pragma once

#include "regexp.fwd.hpp"

#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/shared_container_iterator.hpp>

#include <sys/types.h>
#include <regex.h>

class RegExpManager
{
public:
    struct RegExpResult
    {
	bool Success;
	std::string ErrorMessage;
    };

    RegExpManager(const std::string& regExpFile);

    /**
     * Add regexp and save changes (no need to call SaveRegExps())
     */
    RegExpResult AddRegExp(const std::string& regexp,
			   const std::string& reply);
    /**
     * Remove regexp and save changes (no need to call SaveRegExps())
     */
    bool RemoveRegExp(const std::string& regexp);

    bool SaveRegExps() const;

    /**
     * @param 1 reply
     * @param 2 original message
     * @param 3 matching regexp
     */
    typedef boost::function<std::string (const std::string&,
					 const std::string&,
					 const RegExp&)> Operation;

    /**
     * Find a regexp that matches the message and create a reply.
     * The reply is then passed through the operation function.
     * If the operation functions returns a non-zero length string
     * then that string is returned and the matching is stopped.
     */
    std::string FindMatchAndReply(const std::string& message,
				  Operation operation) const;

    typedef std::vector<RegExp> RegExpContainer;
    typedef boost::shared_ptr<RegExpContainer> RegExpContainerPtr;
    typedef boost::shared_container_iterator<RegExpContainer> RegExpIterator;
    typedef std::pair<RegExpIterator,RegExpIterator> RegExpIteratorRange;

    RegExpIteratorRange FindMatches(const std::string& message) const;
    RegExpIteratorRange FindRegExps(const std::string& searchString) const;

    /**
     * Change reply and save changes (no need to call SaveRegExps())
     */
    bool ChangeReply(const std::string& regexp, const std::string& reply);
    /**
     * Change regexp and save changes (no need to call SaveRegExps())
     */
    RegExpResult ChangeRegExp(const std::string& regexp,
			      const std::string& newRegexp);

    bool MoveUp(const std::string& regexp);
    bool MoveDown(const std::string& regexp);

    // Decode regexp from its non-space syntax
    std::string Decode(const std::string& regexp) const;
    // Encode regexp into its non-space syntax
    std::string Encode(const std::string& regexp) const;

private:

    /**
     * Adds a regexp without updating the regexp file
     */
    RegExpResult AddRegExpImpl(const std::string& regexp,
			       const std::string& reply);

    std::string regExpFile_;

    RegExpContainer regExps_;
};
