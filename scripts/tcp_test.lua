-- init.lua --

-- Global Variables (Modify for your network)
ssid = "NODO2_EXT2"
pass = "AA123456"

-- Configure Wireless Internet
wifi.setmode(wifi.STATION)
print('set mode=STATION (mode='..wifi.getmode()..')\n')
print('MAC Address: ',wifi.sta.getmac())
print('Chip ID: ',node.chipid())
print('Heap Size: ',node.heap(),'\n')
-- wifi config start
wifi.sta.config(ssid,pass)
-- wifi config end

lastDump={}
lastReply={}

mconn = {}
tmr.alarm(0, 1000, 1, function()
  if wifi.sta.getip() == nil then
    print("Connecting to AP...\n")
  else
    print('Network OK')
    print(wifi.sta.getip())
    srv=net.createServer(net.TCP)
    srv:listen(80,function(conn)
      mconn = conn
      conn:on("receive",function(conn,payload)
        lastDump = payload
        print(payload)
        --uart.write(0,">> "..payload.." | "..reply.." <<\n")
        conn:send("reply>"..payload)
      end)
      --conn:on("sent",function(conn) conn:close() end)
    end)
    tmr.stop(0)
  end
end)
