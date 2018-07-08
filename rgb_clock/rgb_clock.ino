#include <cstdint>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <FS.h>

constexpr char ssid[]     = "****";
constexpr char password[] = "****";

constexpr unsigned int httpPort = 80;  // for configuration web page
constexpr unsigned int udpPort = 1337; // for NTP messaging

constexpr unsigned int serialBaudrate = 115200;

ESP8266WebServer server(httpPort);
WiFiUDP udp;

struct Ntp_packet
{
  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.
};

class Time
{
public:
  Time(uint32_t seconds)
  {
    this->hours = (seconds  % 86400L) / 3600;
    this->minutes = (seconds  % 3600) / 60;
    this->seconds = (seconds % 60); 
  }

  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

int32_t ntohl(const int32_t value)
{
    int32_t part0 = (value & 0x000000FF) << 24;
    int32_t part1 = (value & 0x0000FF00) << 8;
    int32_t part2 = (value & 0x00FF0000) >> 8;
    int32_t part3 = (value & 0xFF000000) >> 24;
    return part0 | part1 | part2 | part3;
}

Time get_current_time_from_NTP_server()
{
  Ntp_packet packet{};
  packet.li_vn_mode = 0b11100011;
  packet.stratum = 0x0;
  packet.poll = 0x6;
  packet.precision = 0xEC;

  IPAddress timeServerIP;
  WiFi.hostByName("1.pl.pool.ntp.org", timeServerIP); 
  udp.beginPacket(timeServerIP, 123);
  udp.write(reinterpret_cast<char*>(&packet), sizeof(Ntp_packet));
  udp.endPacket();

  delay(500); // some delay is needed while waiting for response

  int packetSize = udp.parsePacket();
  if (!packetSize)
  {
    Serial.print("No response....");
    return Time(0);
  }

  Serial.printf("Received %d bytes from %s, port %d\n", 
      packetSize, 
      udp.remoteIP().toString().c_str(), udp.remotePort());

  udp.read(reinterpret_cast<char*>(&packet), sizeof(Ntp_packet));
  uint32_t seconds_starting_at_1900 = ntohl(packet.txTm_s); // big to little conversion endian is necessary

  Serial.print("Seconds since Jan 1 1900 = " );
  Serial.println(seconds_starting_at_1900);
  Time t(seconds_starting_at_1900);
  Serial.printf("UTC: %02d:%02d:%02d\n", t.hours, t.minutes, t.seconds);
  return t;
}

void start_m_dns()
{
  Serial.print("Starting mDNS...");
  if (!MDNS.begin("rbgclock")) Serial.println("Error starting MDNS responder!");
  else Serial.println("mDNS started...");  
}

void handleRoot()
{
  server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

void start_webserver()
{
  server.on("/",handleRoot);
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();
}

void connect_to_wifi()
{
  Serial.print("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print(".");}
  Serial.println();

  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_serial()
{
  Serial.begin(serialBaudrate);
  Serial.println("Serial initialized...");  
}

void setup_filesystem()
{
  SPIFFS.begin();
  Serial.println("File System Initialized");
}

void start_udp()
{
  Serial.println("Starting UDP");
  udp.begin(udpPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());  
}

void setup()
{
  setup_serial();
  setup_filesystem();
  connect_to_wifi();
  start_m_dns();
  start_webserver();
  start_udp();
}

void loop() 
{
    get_current_time_from_NTP_server(); // this need to be moved on some long ticker loop
    server.handleClient();   // Listen for incoming clients
}

void handleWebRequests(){
  if(loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

bool loadFromSpiffs(String path)
{
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  // else if(path.endsWith(".js")) dataType = "application/javascript";
  // else if(path.endsWith(".png")) dataType = "image/png";
  // else if(path.endsWith(".gif")) dataType = "image/gif";
  // else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  // else if(path.endsWith(".xml")) dataType = "text/xml";
  // else if(path.endsWith(".pdf")) dataType = "application/pdf";
  // else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SPIFFS.open(path.c_str(), "r");
  if(!dataFile) return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {}

  dataFile.close();
  return true;
}