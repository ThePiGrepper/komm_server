-- main.lua --

lastDump={}
lastReply={}
timeOut_Cnt = 90 -- in seconds

dst_addr = "172.168.1.15"
dst_port = 9000

-- Waiting for Connection
tmr.alarm(0, 1000, 1, function()
   if wifi.sta.getip() == nil then
      print("Connecting to AP...\n")
      timeOut_Cnt = timeOut_Cnt - 1
      if timeOut_Cnt <= 0 then
        node.restart()
      end
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
      --srv=net.createServer(net.TCP)
      --srv:listen(80,function(conn)
      conn=net.createConnection(net.TCP)
        conn:on("receive",function(conn,payload)
          lastDump = payload
          if(payload == "uart_on") then
            uart.setup(0,115200,8,uart.PARITY_NONE,uart.STOPBITS_1);
          elseif(payload == "wifi_rst") then
            print('forget')
            node.restore()
            node.restart()
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
              uart.write(0,">> "..crypto.toHex(payload).." | "..crypto.toHex(reply).." <<\n")
              conn:send(reply)
              --conn:send(">> "..payload.." | "..reply.." <<")
            elseif(ret == 1) then -- reset
              uart.write(0,">> "..crypto.toHex(payload).." | "..crypto.toHex(reply).." <<\n")
              conn:on("sent",function(conn)
                node.restart()
              end)
              conn:send(reply)
              --print("THIS!")
            else --error
              uart.write(0,"?>"..crypto.toHex(payload).." | "..crypto.tohex(reply).." <<\n")
            end
          end
        end)
        --conn:on("sent",function(conn) conn:close() end)
        conn:on("connection",function(conn)
          msg = komm.invite()
          print(crypto.toHex(msg))
          conn:send(msg)
          tmr.alarm(1, 500, 1, function()
            msg,len = komm.getUpdate()
            if len > 0 then
              print(crypto.toHex(msg))
              conn:send(msg)
            end
          end)
        end)
        conn:on("disconnection",function(conn)
          print("disconnected")
          node.restart()
        end)
        conn:connect(dst_port,dst_addr)
      --end)
      tmr.stop(0)
   end
 end)
