
local function StringSplit(str, delim)
   local fragments = {}

   local delimSize = #delim
   local lastIndex = 1-delimSize
   local index = string.find(str, delim)
   while index do
      local fragment = string.sub(str, lastIndex+delimSize, index-1)
      table.insert(fragments, fragment)
      lastIndex = index
      index = string.find(str, delim, index+delimSize)
   end
   if lastIndex+delimSize < #str then
      table.insert(fragments, string.sub(str, lastIndex+delimSize))
   end

   return fragments
end

function RegExp_RegisterCalls()
   RegisterBlockingCall("^add .+", RegExp_Administrative, true);
   RegisterBlockingCall("^del .+", RegExp_Administrative, true);
   RegisterBlockingCall("^match .+", RegExp_Administrative, true);
   RegisterBlockingCall("^find .+", RegExp_Administrative, true);
   RegisterBlockingCall("^delmatch .+", RegExp_Administrative, true);
   RegisterBlockingCall("^chreply .+", RegExp_Administrative, true);
   RegisterBlockingCall("^chregexp .+", RegExp_Administrative, true);
   RegisterBlockingCall("^up .+", RegExp_MoveUp, true);
   RegisterBlockingCall("^down .+", RegExp_MoveDown, true);
   RegisterBlockingCall("^upmatch .+", RegExp_UpMatch, true);
   RegisterBlockingCall("^downmatch .+", RegExp_DownMatch, true);
end

function RegExp_MoveUp(server, fromNick, fromUser, fromHost, to, message)
   local command, parameters = select(3,string.find(message, "([^ ]+) (.*)"))

   if RegExpMoveUp(parameters) then
      return "Ok, "..fromNick
   end
   
   return "No matching regexp."
end

function RegExp_MoveDown(server, fromNick, fromUser, fromHost, to, message)
   local command, parameters = select(3,string.find(message, "([^ ]+) (.*)"))

   if RegExpMoveDown(parameters) then
      return "Ok, "..fromNick
   end
   
   return "No matching regexp."
end

function RegExp_UpMatch(server, fromNick, fromUser, fromHost, to, message)
   local command, parameters = select(3,string.find(message, "([^ ]+) (.*)"))
   
   local direct, message = IsDirectMessage(parameters, GetMyNick(server))
   local matches = RegExpFindMatch(message)
   for i, v in ipairs(matches) do
      -- v[1] = regexp, v[2] = reply, v[3] = encodedRegexp
      if v[3] and ( direct or string.find(v[2], "\\D") ~= 1 ) then
	 if RegExpMoveUp(v[1]) then
	    return "Moved \""..v[1].."\" = \""..v[2].."\" up"
	 end
      end
   end
   return "No matches"
end

function RegExp_DownMatch(server, fromNick, fromUser, fromHost, to, message)
   local command, parameters = select(3,string.find(message, "([^ ]+) (.*)"))

      local direct, message = IsDirectMessage(parameters, GetMyNick(server))
      local matches = RegExpFindMatch(message)
      for i, v in ipairs(matches) do
	 -- v[1] = regexp, v[2] = reply, v[3] = encodedRegexp
	 if v[3] and ( direct or string.find(v[2], "\\D") ~= 1 ) then
	    if RegExpMoveDown(v[1]) then
	       return "Moved \""..v[1].."\" = \""..v[2].."\" down"
	    end
	 end
      end
      return "No matches"
end


function RegExp_Administrative(server, fromNick, fromUser, fromHost, to,
			       message)
   local command, parameters = select(3,string.find(message, "([^ ]+) (.*)"))

   if command == "add" then
      local regexp, reply = select(3,string.find(parameters,"([^ ]+) (.*)"))
      local success, message = RegExpAdd(regexp, reply)
      if success then
	 return "Ok, "..fromNick
      end
      return message
   elseif command == "del" then
      if RegExpDelete(parameters) then
	 return "Deleted \""..parameters.."\""
      end
      return "No matching regexp"
   elseif command == "match" then
      local direct, message = IsDirectMessage(parameters, GetMyNick(server))
      local matches = RegExpFindMatch(message)
      if #matches > 0 then
	 local found = {}
	 for i, v in ipairs(matches) do
	    -- Only match if the message is direct or not required to be direct
	    if direct or string.find(v[2], "\\D") ~= 1 then
	       table.insert(found, "Matches \""..v[1].."\" = \""..v[2].."\"")
	    end
	 end
	 if #found > 0 then
	    return found
	 end
      end
      return "No matches"
   elseif command == "find" then
      local matches = RegExpFindRegExp(parameters)
      if #matches > 0 then
	 local found = {}
	 for i, v in ipairs(matches) do
	    table.insert(found, "Found \""..v[1].."\" = \""..v[2].."\"")
	 end
	 return found
      else
	 return "Nothing found"
      end
   elseif command == "delmatch" then
      local direct, message = IsDirectMessage(parameters, GetMyNick(server))
      local matches = RegExpFindMatch(message)
      for i, v in ipairs(matches) do
	 -- v[1] = regexp, v[2] = reply, v[3] = encodedRegexp
	 if v[3] and ( direct or string.find(v[2], "\\D") ~= 1 ) then
	    if RegExpDelete(v[3]) then
	       return "Deleted \""..v[1].."\" = \""..v[2].."\""
	    end
	 end
      end
      return "No matches"
   elseif command == "chreply" then
      local regexp, reply = select(3,string.find(parameters,"([^ ]+) (.*)"))
      if RegExpChangeReply(regexp, reply) then
	 return "Ok, "..fromNick
      else
	 return "No matching regexp"
      end
   elseif command == "chregexp" then
      local regexp, newExp = select(3,string.find(parameters,"([^ ]+) (.*)"))
      local success, errorMessage = RegExpChangeRegExp(regexp, newExp)
      if success then
	 return "Ok, "..fromNick
      else
	 return errorMessage
      end
   end
end

RegExp_Server = ""
RegExp_FromNick = ""
RegExp_FromUser = ""
RegExp_FromHost = ""
RegExp_To = ""
RegExp_Channel = ""
RegExp_Direct = false

function RegExp_RunCode(startblob, code, endblob)
   local reply = ""
   local f, error = loadstring(code)
   if f then
      reply = tostring(f() or "") or ""
      local endline = string.find(reply, '\n')
      if endline then reply = string.sub(reply, 1, endline-1) end
      if reply == nil then return "" end
      return reply
   else
      reply = error
   end
   return reply
end

local function RegExp_CheckCommands(message)

   local index = string.find(message, "`")
   while index do
      if index == 1 or message[index-1] ~= "\\" then
	 local endQuote = string.find(message, "[^\\]`", index+1)
	 if endQuote and index+1 <= endQuote then
	    local command = string.sub(message, index+1, endQuote)
	    command = string.gsub(command, "\\`", "`")
	    command = string.gsub(command, "\\\\", "\\")
	    local result = Execute(command)
	    local endline = string.find(result, '\n')
	    if endline then result = string.sub(result, 1, endline-1) end
	    message = string.sub(message, 1, index-1)..result
	       ..string.sub(message, endQuote+2)
	 end
      end
      index = string.find(message, "`", index+1)
   end
   return message
end

local function RegExp_ChooseRandom(openBracket, content, closeBracket)
   local outcomes = StringSplit(content, "|")

   local outcome = math.random(#outcomes)

   return outcomes[outcome]
end

local function RegExp_ReplaceEscapeCode(message, code, replacement)
   local reply = message
   local pos = reply:find(code)
   while pos do
      if pos == 1 or reply:sub(pos-1, pos-1) ~= "\\" then
	 reply = reply:sub(1, pos-1)..replacement..reply:sub(pos+2)
      elseif reply:sub(pos-1, pos-1) == "\\" then
	 reply = reply:sub(1, pos-1)..reply:sub(pos+1)
	 pos = pos + 2
      end
      pos = reply:find(code, pos)
   end
   return reply
end

local function RegExp_ModifyReply(subbedReply, message, regexp, orgReply)
   local reply = subbedReply
   local matches = 1
   local loops = 0
   

   if string.find(reply, "\\N") or 0 > 0 then
      local nicks = GetChannelNicks(RegExp_Channel, RegExp_Server)
      reply = RegExp_ReplaceEscapeCode(reply, "\\N", table.concat(nicks, "|"))
   end

 
   if (reply:find("\\e") or 0) ~= 1 and (reply:find("\\E") or 0 ) ~= 1 then
      while matches ~= 0 and loops < 10 do
	 matches = 0
	 reply, matches = string.gsub(reply, "({)([^{}]*)(})",
				      RegExp_ChooseRandom)
	 loops = loops + 1
      end
   end

   reply = RegExp_ReplaceEscapeCode(reply, "\\n", RegExp_FromNick)

   reply = RegExp_ReplaceEscapeCode(reply, "\\b", GetMyNick(RegExp_Server))
   reply = RegExp_ReplaceEscapeCode(reply, "\\s", RegExp_Channel)
   local logname = string.gsub(GetLogName(RegExp_Channel, RegExp_Server),
			       "#", "\\#")
   reply = RegExp_ReplaceEscapeCode(reply, "\\l", logname)
   reply = RegExp_ReplaceEscapeCode(reply, "\\c", "\1")

   if RegExp_Direct then
      reply = RegExp_ReplaceEscapeCode(reply, "\\D", "")
   elseif string.find(reply, "\\D") == 1 then
      return ""
   end

   reply = string.gsub(reply, "(¤)([^¤]*)(¤)", RegExp_RunCode)
   reply = RegExp_CheckCommands(reply)
   if string.find(reply, "\\e") == 1 or string.find(reply, "\\E") == 1 then
      reply = reply:gsub("\\\\", "\\")
      reply = Execute(string.sub(reply, 3))
   end
   local pos = string.find(reply, "\\r")
   if pos then
      local prefix = ""
      if pos > 1 then
	 prefix = string.sub(reply, 1, pos-1)
      end
      reply = prefix..RecurseMessage(string.sub(reply, pos+2), RegExp_To,
                                     RegExp_FromNick, RegExp_FromUser,
                                     RegExp_FromHost, RegExp_Server)
   end

   if reply:find("\\k") == 1 then
      Kick(RegExp_FromNick, reply:sub(3))
      return ""
   end

   return reply
end

function RegExp_OnMessage(server, fromNick, fromUser, fromHost, to, message)
   local myNick = GetMyNick(server)

   RegExp_Server = server
   RegExp_FromNick = fromNick
   RegExp_FromUser = fromUser
   RegExp_FromHost = fromHost
   RegExp_To = to
   RegExp_Channel = to
   RegExp_Direct, message = IsDirectMessage(message, myNick)
   if to == myNick then
      RegExp_Channel = fromNick
      RegExp_Direct = true
   end

   local reply = RegExpMatchAndReply(message, RegExp_ModifyReply)
   if #reply > 0 then
      local rows = StringSplit(reply, "\n")
      if #rows >= 1 then
	 return unpack(rows)
      end
      return reply
   end
end


--RegisterForEvent("ON_MESSAGE", rxp)

RegExp_RegisterCalls()

RegisterForEvent("ON_MESSAGE", RegExp_OnMessage)

