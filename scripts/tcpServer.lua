-- main.lua --

lastDump={}
lastReply={}
conn_timeout = 90 -- in seconds
reconn_timeout = 90 -- in tries

dst_addr = "172.168.1.15"
dst_port = 9000

-- Waiting for Connection
tmr.alarm(0, 1000, tmr.ALARM_AUTO, function()
   if wifi.sta.getip() == nil then
      print("Connecting to AP...\n")
      conn_timeout = conn_timeout - 1
      if conn_timeout <= 0 then
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
          reconn_timeout = 90 -- here set number of disconnection retries
          msg = komm.invite()
          print(crypto.toHex(msg))
          conn:send(msg)
          tmr.alarm(1, 500, tmr.ALARM_AUTO, function()
            msg,len = komm.getUpdate()
            if len > 0 then
              print(crypto.toHex(msg))
              conn:send(msg)
            end
          end)
        end)
        conn:on("disconnection",function(conn)
          if reconn_timeout > 0 then
            reconn_timeout= reconn_timeout - 1
            print("disconnected : try #"..reconn_timeout)
            tmr.alarm(2, 3000, tmr.ALARM_SINGLE, function()
              conn:connect(dst_port,dst_addr)
            end)
          else
            print("disconnected")
            node.restart()
          end
        end)
        conn:connect(dst_port,dst_addr)
      --end)
      tmr.stop(0)
   end
 end)
