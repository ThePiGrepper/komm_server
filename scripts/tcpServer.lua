-- main.lua --

lastDump={}
lastReply={}

-- Connect 
--print('\nStarting Sensus Board\n')
tmr.alarm(0, 1000, 1, function()
   if wifi.sta.getip() == nil then
      print("Connecting to AP...\n")
   else
      ip, nm, gw=wifi.sta.getip()
      print("IP Info: \nIP Address: ",ip)
      print("Netmask: ",nm)
      print("Gateway Addr: ",gw,'\n')
      print(komm.info())
      tmr.stop(0)
   end
end)

tmr.alarm(1,0001,0,function()
  komm_loadState()
  print('Previous State Loaded')
   -- Start a simple http server
  srv=net.createServer(net.TCP)
  srv:listen(80,function(conn)
    conn:on("receive",function(conn,payload)
      --uart.write(0,payload)
      lastDump = payload
      ret,reply,mod = komm.digest(payload)
      print('modified?='..mod)
      if(mod == 1) then
        komm_saveState();
      end
      print("ret value:"..ret)
      lastReply = reply
      if(ret == 0) then
        --reply = payload
        uart.write(0,">> "..payload.." | "..reply.." <<\n")
        conn:send(reply)
        --conn:send(">> "..payload.." | "..reply.." <<")
      elseif(ret == 1) then -- reset
        uart.write(0,">> "..payload.." | "..reply.." <<\n")
        conn:send(reply)
        node.restart()
        print("THIS!")
      else --error
        uart.write(0,"?>"..payload.." | "..reply.." <<\n")
      end
    end)
    conn:on("sent",function(conn) conn:close() end)
  end)
end)
