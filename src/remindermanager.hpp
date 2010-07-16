#pragma once

#include <string>
#include <vector>
#include <set>

#include <boost/shared_container_iterator.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

#include <unicode/unistr.h>

class ReminderManager
{
public:
	typedef boost::function<void(const UnicodeString&, const std::string&,
			const UnicodeString&)> ReminderCallback;

	struct Reminder
	{
		time_t Timestamp;
		UnicodeString Server;
		std::string Channel;
		UnicodeString Message;

		bool operator<(const Reminder& rhs) const;
	};

	ReminderManager(const UnicodeString& remindersFile,
			ReminderCallback callback);

	virtual ~ReminderManager();

	void CreateReminder(time_t inNumSeconds, const UnicodeString& server,
			const std::string& channel, const UnicodeString& message);

	typedef std::vector<Reminder> DueRemindersContainer;
	typedef boost::shared_container_iterator<DueRemindersContainer>
			ReminderIterator;
	typedef std::pair<ReminderIterator, ReminderIterator> ReminderIteratorRange;

	ReminderIteratorRange GetDueReminders(bool erase = true);

	/**
	 * @throw Exception if searchString is an invalid regular expression
	 */
	ReminderIteratorRange
			FindReminders(const UnicodeString& server,
					const UnicodeString& channel,
					const UnicodeString& searchString) const;

	void CleanDueReminders();

	/**
	 * @return The reminder that is closest in the future
	 * @throw Exception if there is no upcoming reminder
	 */
	const Reminder& GetNextReminder() const;

private:
	void SaveReminders();
	void ReadReminders();

	void Timer();

	typedef std::multiset<Reminder> ReminderContainer;
	ReminderContainer reminders_;
	UnicodeString remindersFile_;
	boost::mutex timerMutex_;
	mutable boost::shared_mutex reminderMutex_;
	boost::condition_variable timerCondition_;
	std::auto_ptr<boost::thread> timerThread_;
	bool runTimer_;
	ReminderCallback reminderCallback_;
};
