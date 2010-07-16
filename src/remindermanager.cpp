#include "remindermanager.hpp"
#include "exception.hpp"
#include "logging/logger.hpp"

#include <fstream>

#include <boost/regex/icu.hpp>

bool ReminderManager::Reminder::operator<(const Reminder& rhs) const
{
	return Timestamp < rhs.Timestamp;
}

ReminderManager::ReminderManager(const UnicodeString& remindersFile,
		ReminderManager::ReminderCallback callback) :
	remindersFile_(remindersFile), runTimer_(true), reminderCallback_(callback)
{
	ReadReminders();
	timerThread_.reset(new boost::thread(boost::bind(&ReminderManager::Timer,
			this)));
}

ReminderManager::~ReminderManager()
{
	runTimer_ = false;
	timerCondition_.notify_all();
	timerThread_->join();
}

void ReminderManager::CreateReminder(time_t inNumSeconds,
		const UnicodeString& server, const std::string& channel,
		const UnicodeString& message)
{
	Reminder reminder =
	{ time(0) + inNumSeconds, server, channel, message };

	time_t nextTime = std::numeric_limits<time_t>::max();
	try
	{
		const Reminder& nextReminder = GetNextReminder();
		nextTime = nextReminder.Timestamp;
	} catch (Exception&)
	{
		// No reminders, keep the timestamp at a silly high number
	}

	// Exclusive lock
	boost::lock_guard<boost::shared_mutex> lock(reminderMutex_);

	reminders_.insert(reminder);

	SaveReminders();

	if (reminder.Timestamp < nextTime)
	{
		// If the new reminder is going to fire earlier than the current
		// reminder we were waiting for we need to signal the timer thread
		// to pick up the new reminder instead.
		timerCondition_.notify_all();
	}
}

ReminderManager::ReminderIteratorRange ReminderManager::GetDueReminders(
		bool erase)
{
	typedef boost::shared_ptr<DueRemindersContainer> DueRemindersContainerPtr;
	DueRemindersContainerPtr dueReminders(new DueRemindersContainer());

	time_t currentTime = time(0);

	std::auto_ptr<boost::shared_lock<boost::shared_mutex> > sharedLock;
	std::auto_ptr<boost::lock_guard<boost::shared_mutex> > lockGuard;

	if (erase)
	{
		lockGuard.reset(new boost::lock_guard<boost::shared_mutex>(
				reminderMutex_));
	}
	else
	{
		sharedLock.reset(new boost::shared_lock<boost::shared_mutex>(
				reminderMutex_));
	}

	for (ReminderContainer::iterator reminder = reminders_.begin(); reminder
			!= reminders_.end();)
	{
		if (reminder->Timestamp <= currentTime)
		{
			dueReminders->push_back(*reminder);
			if (erase)
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

	if (erase)
	{
		SaveReminders();
	}

	return boost::make_shared_container_range(dueReminders);
}

ReminderManager::ReminderIteratorRange ReminderManager::FindReminders(
		const UnicodeString& server, const UnicodeString& channel,
		const UnicodeString& searchString) const
{
	typedef boost::shared_ptr<DueRemindersContainer> DueRemindersContainerPtr;
	DueRemindersContainerPtr foundReminders(new DueRemindersContainer());

	boost::shared_lock<boost::shared_mutex> lock(reminderMutex_);

	try
	{
		boost::u32regex regexp = boost::make_u32regex(searchString,
				boost::regex::extended | boost::regex::icase);

		for (ReminderContainer::iterator reminder = reminders_.begin(); reminder
				!= reminders_.end(); ++reminder)
		{
			if (boost::u32regex_search(reminder->Message, regexp))
			{
				foundReminders->push_back(*reminder);
			}
		}
	} catch (boost::regex_error& e)
	{
		throw Exception(__FILE__, __LINE__, e.what());
	}
	return boost::make_shared_container_range(foundReminders);
}

void ReminderManager::CleanDueReminders()
{
	time_t currentTime = time(0);

	boost::lock_guard<boost::shared_mutex> lock(reminderMutex_);

	for (ReminderContainer::iterator reminder = reminders_.begin(); reminder
			!= reminders_.end();)
	{
		if (reminder->Timestamp <= currentTime)
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
	boost::shared_lock<boost::shared_mutex> lock(reminderMutex_);

	if (reminders_.begin() != reminders_.end())
	{
		return *reminders_.begin();
	}
	throw Exception(__FILE__, __LINE__, "No upcoming reminder");
}

void ReminderManager::Timer()
{
	Log << LogLevel::Info << "Reminder thread " << boost::this_thread::get_id()
			<< " starting";
	while (runTimer_)
	{
		boost::unique_lock<boost::mutex> lock(timerMutex_);

		boost::posix_time::ptime remindTime(boost::posix_time::pos_infin);
		try
		{
			remindTime = boost::posix_time::from_time_t(
					GetNextReminder().Timestamp);
			if (remindTime < boost::get_system_time())
			{
				remindTime = boost::get_system_time();
			}
		} catch (Exception&)
		{
			// There are no remaining reminders, wait until notification
			// with no timeout
		}

		if (!timerCondition_.timed_wait(lock, remindTime))
		{
			if (runTimer_)
			{
				// The timer timed out and so we should trigger
				// the reminder

				ReminderIteratorRange iterators = GetDueReminders();
				for (ReminderIterator reminder = iterators.first; reminder
						!= iterators.second; ++reminder)
				{
					try
					{
						reminderCallback_(reminder->Message, reminder->Channel,
								reminder->Server);
					} catch (...)
					{
						// Catch all exceptions and ignore them
					}
				}
			}
		}
	}
	Log << LogLevel::Info << "Reminder thread " << boost::this_thread::get_id()
			<< " ending";
}

void ReminderManager::SaveReminders()
{
	std::ofstream fout(AsUtf8(remindersFile_).c_str());
	fout.imbue(std::locale::classic());

	for (ReminderContainer::iterator reminder = reminders_.begin(); reminder
			!= reminders_.end() && fout.good(); ++reminder)
	{
		fout << reminder->Timestamp << " " << AsUtf8(reminder->Server) << " "
				<< reminder->Channel << " " << AsUtf8(reminder->Message) << std::endl;
	}
}

void ReminderManager::ReadReminders()
{
	std::ifstream fin(AsUtf8(remindersFile_).c_str());
	fin.imbue(std::locale::classic());

	while (fin.good())
	{
		Reminder reminder;
		std::string server;
		fin >> reminder.Timestamp >> server >> reminder.Channel;
		reminder.Server = ConvertString(server);
		fin.get();
		std::string message;
		std::getline(fin, message);
		reminder.Message = ConvertString(message);
		if (reminder.Server.isEmpty() || reminder.Channel.empty()
				|| reminder.Message.isEmpty())
		{
			break;
		}
		reminders_.insert(reminder);
	}
}
