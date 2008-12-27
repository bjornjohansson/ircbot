#include "remindermanager.hpp"
#include "exception.hpp"

#include <fstream>
#include <iostream>

#include <boost/regex.hpp>

bool ReminderManager::Reminder::operator<(const Reminder& rhs) const
{
    return Timestamp < rhs.Timestamp;
}

ReminderManager::ReminderManager(const std::string& remindersFile)
    : remindersFile_(remindersFile)
{
    ReadReminders();
}

void ReminderManager::CreateReminder(time_t inNumSeconds,
				     const std::string& server,
				     const std::string& channel,
				     const std::string& message)
{
    Reminder reminder = {time(0)+inNumSeconds, server, channel, message};

    reminders_.insert(reminder);

    SaveReminders();
}
    
ReminderManager::ReminderIteratorRange
ReminderManager::GetDueReminders(bool erase)
{
    typedef boost::shared_ptr<DueRemindersContainer> DueRemindersContainerPtr;
    DueRemindersContainerPtr dueReminders(new DueRemindersContainer());

    time_t currentTime = time(0);

    for(ReminderContainer::iterator reminder = reminders_.begin();
	reminder != reminders_.end(); )
    {
	if ( reminder->Timestamp <= currentTime )
	{
	    dueReminders->push_back(*reminder);
	    if ( erase )
	    {
		ReminderContainer::iterator oldReminderIterator = reminder;
		++reminder;
		reminders_.erase(oldReminderIterator);
	    }
	    else
	    {
		++reminder;
	    }
	}
	else
	{
	    break;
	}
    }

    if ( erase )
    {
	SaveReminders();
    }

    return boost::make_shared_container_range(dueReminders);
}

ReminderManager::ReminderIteratorRange
ReminderManager::FindReminders(const std::string& server, 
			       const std::string& channel,
			       const std::string& searchString) const
{
    typedef boost::shared_ptr<DueRemindersContainer> DueRemindersContainerPtr;
    DueRemindersContainerPtr foundReminders(new DueRemindersContainer());

    try
    {
	boost::regex regexp(searchString,
			    boost::regex::extended|boost::regex::icase);

	for(ReminderContainer::iterator reminder = reminders_.begin();
	    reminder != reminders_.end();
	    ++reminder)
	{
	    if ( boost::regex_search(reminder->Message, regexp) )
	    {
		foundReminders->push_back(*reminder);
	    }
	}
    }
    catch ( boost::regex_error& e )
    {
	throw Exception(__FILE__, __LINE__, e.what()
);
    }
    return boost::make_shared_container_range(foundReminders);
}

void ReminderManager::CleanDueReminders()
{
    time_t currentTime = time(0);

    for(ReminderContainer::iterator reminder = reminders_.begin();
	reminder != reminders_.end(); )
    {
	if ( reminder->Timestamp <= currentTime )
	{
	    ReminderContainer::iterator oldReminderIterator = reminder;
	    ++reminder;
	    reminders_.erase(oldReminderIterator);
	}
	else
	{
	    ++reminder;
	}
    }
    SaveReminders();
}

const ReminderManager::Reminder& ReminderManager::GetNextReminder() const
{
    time_t currentTime = time(0);

    for(ReminderContainer::iterator reminder = reminders_.begin();
	reminder != reminders_.end();
	++reminder)
    {
	if ( reminder->Timestamp > currentTime )
	{
	    return *reminder;
	}
    }
    throw Exception(__FILE__, __LINE__, "No upcoming reminder");
}


void ReminderManager::SaveReminders()
{
    std::ofstream fout(remindersFile_.c_str());

    for(ReminderContainer::iterator reminder = reminders_.begin();
	reminder != reminders_.end() && fout.good();
	++reminder)
    {
	fout<<reminder->Timestamp<<" "<<reminder->Server<<" "
	    <<reminder->Channel<<" "<<reminder->Message<<std::endl;
    }
}

void ReminderManager::ReadReminders()
{
    std::ifstream fin(remindersFile_.c_str());

    while ( fin.good() )
    {
	Reminder reminder;
	fin>>reminder.Timestamp>>reminder.Server>>reminder.Channel;
	fin.get();
	std::getline(fin, reminder.Message);
	if ( reminder.Server.empty() || reminder.Channel.empty() ||
	     reminder.Message.empty() )
	{
	    break;
	}
	reminders_.insert(reminder);
    }
}
