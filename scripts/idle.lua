
local function idle(server, fromNick, fromUser, fromHost, to, message)
   local pos, length, nick = string.find(message, "!idle ([^ ]*)")
   if pos == 1 and string.find(to, "#") == 1 and nick then
      local line, secondsAgo = GetLastLine(server, to, nick)
      if line then
	 return nick.." has been idle for "
	    ..FormatTimeDiff(secondsAgo)..". Last thing he said was \""
	    ..line.."\""
      end
      return "I don't know how long "..nick.." has been idle."
   end
end

RegisterBlockingCall("^!idle .+", idle)

