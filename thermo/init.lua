--init.lua
print("Setting up WIFI...")
wifi.setmode(wifi.STATION)
--modify according your wireless router settings

station_cfg={}
station_cfg.ssid="myAP"
station_cfg.pwd="myPassword"
station_cfg.save=false

thingspeak_api_key="nice_try"

wifi.sta.config(station_cfg)
wifi.sta.connect()

wifi_connected = false
dhtpin = 4
            
tmr.alarm(1, 2000, tmr.ALARM_AUTO, 
function() 
    if wifi.sta.getip()== nil then 
        print("IP unavaiable, Waiting at "..station_cfg.ssid.."...") 
    else 
        tmr.stop(1)
        print("Config done, IP is "..wifi.sta.getip())
        wifi_connected = true
    end 
end)

function postThingSpeak(level)
    if wifi_connected then
        connout = nil
        connout = net.createConnection(net.TCP, 0)
        connout:on("receive", function(connout, payloadout)
            if (string.find(payloadout, "Status: 200 OK") ~= nil) then
                print("Posted OK");
            end
        end)
     
        connout:on("connection", function(connout, payloadout)
            print ("Sending data...");

            voltage = adc.readvdd33(0);   
            status, temp, humi, temp_dec, humi_dec = dht.read11(dhtpin)
            print(status)
            if status == dht.OK then
            connout:send("GET /update?api_key="..thingspeak_api_key
            .. "&field1=" .. temp .. "." .. temp_dec
            .. "&field2=" .. humi .. "." .. humi_dec
            .. "&field3=" .. (voltage/1000) .. "." .. (voltage%1000)
            .. " HTTP/1.1\r\n"
            .. "Host: api.thingspeak.com\r\n"
            .. "Connection: close\r\n"
            .. "Accept: */*\r\n"
            .. "User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; nonWindows NT 5.1)\r\n"
            .. "\r\n")
            end
        end)

        connout:on("sent",function(connout)
                      print("Closing connection...")
                      connout:close()
                  end)
                  
        connout:on("disconnection", function(connout)
                      print("Got disconnection...")
                end)
        
        connout:connect(80,'api.thingspeak.com')
    else
        print("Not yet connected to WIFI, so can't post data to TS")
    end
end
 
tmr.alarm(0, 60000, 1, function() postThingSpeak(0) end)


