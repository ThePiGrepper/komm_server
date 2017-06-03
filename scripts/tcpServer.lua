-- main.lua --

lastDump={}
lastReply={}

-- Waiting for Connection
tmr.alarm(0, 1000, 1, function()
   if wifi.sta.getip() == nil then
      print("Connecting to AP...\n")
   else
      enduser_setup.stop()
      ip, nm, gw=wifi.sta.getip()
      print("IP Info: \nIP Address: ",ip)
      print("Netmask: ",nm)
      print("Gateway Addr: ",gw,'\n')
      print(komm.info())
      if(komm_loadState() == 0) then
        print('Previous State Loaded')
      end
       -- Start a simple http server
      srv=net.createServer(net.TCP)
      srv:listen(80,function(conn)
        conn:on("receive",function(conn,payload)
          lastDump = payload
          if(payload == "uart_on") then
            uart.setup(0,115200,8,uart.PARITY_NONE,uart.STOPBITS_1);
          elseif(payload == "wifi_rst") then
            node.restore()
            print('forget')
          elseif (string.byte(payload:sub(1,1)) == 0) or (string.byte(payload:sub(1,1)) == 2) then
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
              --print("THIS!")
            else --error
              uart.write(0,"?>"..payload.." | "..reply.." <<\n")
            end
          end
        end)
        --conn:on("sent",function(conn) conn:close() end)
      end)
      tmr.stop(0)
   end
 end)
