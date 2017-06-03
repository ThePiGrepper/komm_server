-- init.lua --

--'forget network' button
forget_pin = 4
gpio.mode(forget_pin,gpio.INT)
gpio.trig(forget_pin, "down", function()
  node.restore()
  print('forget!!')
end)

wifi.setmode(wifi.STATIONAP)
wifi.ap.config({ssid="SensusSetup", auth=wifi.OPEN})
enduser_setup.manual(true)
enduser_setup.start(
  function()
    print("Connected to wifi as:" .. wifi.sta.getip())
  end,
  function(err, str)
    print("enduser_setup: Err #" .. err .. ": " .. str)
  end
)
-- Configure Wireless Internet
print('\nBooting Sensus Board\n')
--wifi.setmode(wifi.STATION)
print('set mode=STATION (mode='..wifi.getmode()..')\n')
print('MAC Address: ',wifi.sta.getmac())
print('Chip ID: ',node.chipid())
print('Heap Size: ',node.heap(),'\n')
-- wifi config start
--wifi.sta.config(ssid,pass)
-- wifi config end

-- Run the main file
dofile("komm_server.lua")
dofile("tcpServer.lua")
--end
