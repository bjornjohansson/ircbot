-- Split a compact irc nick/user/host string into components
-- Usage: nick, user, host = SplitUserString(userString)
function SplitUserString(userString)
   local nick, user, host

   local remaining = userString
   
   local pos = string.find(remaining,"!")
   if pos then
      nick = string.sub(remaining, 0, pos-1)
      remaining = string.sub(remaining, pos+1)
   end
   pos = string.find(remaining,"@")
   if pos then
      user = string.sub(remaining, 0, pos-1)
      host = string.sub(remaining, pos+1)
   end

   return nick, user, host
end

-- Format a number of seconds into a format of Weeks, days, hours, seconds
function FormatTimeDiff(diff)
   local idleTime = string.format("%d seconds",diff%60)
   if diff >= 60 then
      local minutes = math.floor(diff/60)
      idleTime = string.format("%d minutes, ",minutes%60)..idleTime
      if minutes >= 60 then
	 local hours = math.floor(minutes/60)
	 idleTime = string.format("%d hours, ",hours%24)..idleTime
	 if hours >= 24 then
	    local days = math.floor(hours/24)
	    idleTime = string.format("%d days, ",days%7)..idleTime
	    if days >= 7 then
	       local weeks = math.floor(days/7)
	       idleTime = string.format("%d weeks, ", weeks)..idleTime
	    end
	 end
      end
   end
   return idleTime
end

function IsDirectMessage(message, myNick)
   local pattern = myNick.."[,:] "
   local first, last  = string.find(message, pattern)
   if first == 1 then
      return true, string.sub(message, last+1)
   end
   return false, message
end

function JoinChannelCall(server, fromNick, fromUser, fromHost, to, message)
   local command, channel, key = select(3,
					string.find(message,
						    "([^ ]+) ([^ ]+) ?(.*)"))
   Join(server, channel, key)
end

RegisterBlockingCall("^join .+", JoinChannelCall, true)