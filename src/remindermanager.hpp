#pragma once

#include <string>
#include <vector>
#include <set>

#include <boost/shared_container_iterator.hpp>

class ReminderManager
{
public:
    struct Reminder
    {
	time_t Timestamp;
	std::string Server;
	std::string Channel;
	std::string Message;

	bool operator<(const Reminder& rhs) const;
    };

    ReminderManager(const std::string& remindersFile);

    void CreateReminder(time_t inNumSeconds,
			const std::string& server,
			const std::string& channel,
			const std::string& message);
    
    typedef std::vector<Reminder> DueRemindersContainer;
    typedef boost::shared_container_iterator<DueRemindersContainer> ReminderIterator;
    typedef std::pair<ReminderIterator,ReminderIterator> ReminderIteratorRange;

    ReminderIteratorRange GetDueReminders(bool erase = true);

    /**
     * @throw Exception if searchString is an invalid regular expression
     */
    ReminderIteratorRange FindReminders(const std::string& server, 
					const std::string& channel,
					const std::string& searchString) const;

    void CleanDueReminders();

    /**
     * @return The reminder that is closest in the future
     * @throw Exception if there is no upcoming reminder
     */
    const Reminder& GetNextReminder() const;

private:
    void SaveReminders();
    void ReadReminders();
    
    typedef std::multiset<Reminder> ReminderContainer;
    ReminderContainer reminders_;
    std::string remindersFile_;
};
