// Present a "Will be back soon web page", as stand-in webserver.
// 2011-01-30 <jc@wippler.nl>
//
// License: GPLv2

#include <EtherCard.h>
#include <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define STATIC 1  // set to 1 to disable DHCP (adjust myip/gwip values below)

#if STATIC
// ethernet interface ip address
static byte myip[] = { 192,168,2,22 };
// gateway ip address
static byte gwip[] = { 192,168,2,1 };
#endif

// ethernet mac address - must be unique on your network
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x32 };

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer

#define ONE_WIRE_BUS 7
int relay = 8;
float temperature = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup(){
  Timer1.initialize(5000000);
  Timer1.attachInterrupt(callback);   
  
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  
  Serial.begin(57600);
  sensors.begin();
  Serial.println("\n[backSoon]");

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println( "Failed to access Ethernet controller");
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
#endif

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
}

void callback()
{
  sensors.requestTemperatures(); 
  temperature = sensors.getTempCByIndex(0);
  Serial.println(temperature);
}

void loop () {

  String temperature_str(temperature, 1);

  String page =
  "HTTP/1.0 200 OK\r\n"
  "\r\n"
  "<html>"
    "<head>"
    "<title>Arduino Test</title>"
    "<link rel=\"icon\" href=\"data:;base64,iVBORw0KGgo=\">"
    "<style type=\"text/css\">"
    "a.button {"
      "color: red"
    "}"
    "</style>"
    "</head>"
    "<body>"
      "<h3>Light switch</h3>"
      "<a href=\"/?ON\" class=\"button\">"
        "<input type=\"button\" value=\"ON\" />"
      "</a>"
      "<a href=\"/?OFF\" class=\"button\">"
        "<input type=\"button\" value=\"OFF\" />"
      "</a>"
      "<h3>Room temperature</h3>"
  ;
  page += temperature_str;
  page += 
    "</body>"
    "</html>"
  ;

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos) {

    // IF RELAY ON
    if(strstr((char *)Ethernet::buffer + pos, "GET /?ON") != 0) {
      Serial.println("Received ON command");
      digitalWrite(relay, HIGH);
    }

    // IF RELAY OFF  
    if(strstr((char *)Ethernet::buffer + pos, "GET /?OFF") != 0) {
      Serial.println("Received OFF command");
      digitalWrite(relay, LOW);
    }
    
    Serial.println(page);
    page.toCharArray((char*)ether.tcpOffset(), 500);
    ether.httpServerReply(page.length());
  }
}
