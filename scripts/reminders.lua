
function Reminders_RegisterCalls()
   RegisterBlockingCall("^!remind [^ ]+ .+", Reminders_Call, true)
   RegisterBlockingCall("^!reminders .+", Reminders_Call, false)
end

function Reminders_Call(server, fromNick, fromUser,
			     fromHost, to, message)
   local myNick = GetMyNick(server)
   local channel = to
   if to == myNick then
      channel = fromNick
   end

   if string.find(message, "!remind ") == 1 then
      local command, parameters = select(3,string.find(message,
						       "([^ ]+) (.*)"))
      local seconds, reminder = select(3, string.find(parameters, 
						      "^([0-9]+) (.*)"))
      if seconds and reminder then
	 ReminderAdd(tonumber(seconds), server, channel, reminder)
	 return "Ok, "..fromNick
      else
	 return "Syntax: "..GetMyNick(server)..": !remind <seconds> reminder"
      end
   elseif string.find(message, "!reminders") == 1 then
      local command, parameters = select(3,string.find(message,
						       "([^ ]+) (.*)"))
      local reminders = ReminderFind(server, channel, parameters)

      if #reminders > 0 then
	 local matches = {}
	 for i,v in ipairs(reminders) do
	    table.insert(matches, FormatTimeDiff(v[1])..": "..v[2])
	 end
	 return matches
      else
	 return "Nothing found"
      end
   end
end

Reminders_RegisterCalls()
