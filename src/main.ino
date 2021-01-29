/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "Ethernet.h"
#include "WebServer.h"
#include "ArduinoJson.h"
#include "millisDelay.h"
#include <SPI.h>

//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
String readString;
millisDelay updateDelay;
int tmp = 0;
char inputChar;
char buf[256];
int relayPins[] = {2, 3, 4, 6};
int outletStates[] = {1, 1, 1, 1};
static uint8_t mac[] = {0xF9, 0x62, 0x30, 0x4C, 0x7D, 0xC5};
IPAddress ipAddr(10, 0, 1, 12);
IPAddress gateway(192, 168, 1, 1);
IPAddress dns(8, 8, 8, 8);

EthernetClient updClient;
EthernetServer swServer(80);
byte updSrvAddr[] = {192, 168, 0, 114};
int srvPort = 7071;

void sendUpdate(IPAddress ip) {
	// webserver.reset();
	// IPAddress *localip;
	sprintf(buf, "{\"password\": \"secpass123\",\"name\": \"ard\",\"payload\": {\"deviceName\": \"arduino\",\"ipAddress\": \"%d.%d.%d.%d\",}}\0", ip[0], ip[1], ip[2], ip[3]);
	Serial.println(buf);
	if (updClient.connect(updSrvAddr, srvPort)) {
		if (updClient.connected())
			Serial.println("connected");

		Serial.println("sending addr upd");

		updClient.println("PUT /api/spduAPI HTTP/1.1");
		updClient.print("Host: ");
		updClient.println("192.168.0.114:7071");
		updClient.println("User-Agent: Arduino/1.0");
		updClient.println("Connection: close");
		updClient.println("Content-Type: application/x-www-form-urlencoded;");
		updClient.print("Content-Length: ");
		updClient.println(strlen(buf));
		updClient.println();
		updClient.println(buf);
	} else {
		Serial.println("could not connect to update server");
	}
	tmp = 0;
	updClient.stop();
	// updClient.
	// webserver.begin();
	return;
}

void procSwitchReq(void) 
{
	EthernetClient client = swServer.available();
	String req;
	memset(buf, 0, sizeof(buf));
	if (client) {
		while (client.connected()) {
			if (client.available()) {
				client.readBytes(buf, sizeof(buf));
				// client.find("Content-Length:");
				// client.find("\r\n\r\n");
				// client.find("\"name\":\"");
				// int outlet = client.parseInt();
				// client.find("\"state\":\"");
				// int state = client.parseInt();


				// sprintf(buf, "name - %d\nstate - %d", outlet, state);
				Serial.println(buf);


				delay(1);
				client.println("HTTP/1.1 200 OK");
				client.println("Content-Type: text/html");
				client.println("Connection: close");
				// client.println("Connection: Keep-Alive");
				// client.println("Keep-Alive: timeout=5, max=5");
				// client.println("\nOK");
			}
		}
	}
	delay(1);
	client.stop();
	delay(1);
	return;
}

void setup()
{
	//enable serial data print
	tmp = 0;
	Serial.begin(9600);
	while (!Serial); // wait for serial port
begin:
	// if (!SD.begin(4)) {
	// 	Serial.println("Failed to initialize SD card.");
	// 	return;
	// }
	// debug_init();
	//-------------------------------------------------------------------------------------------------------
	// Init webserver
	//-------------------------------------------------------------------------------------------------------
	Serial.println("Initializing with DHCP...");
	// W5100.getMACAddress((uint8_t*)&tmp);
	// Serial.println(tmp);
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure with DHCP, using defined parameters.");
		Ethernet.begin(mac, ipAddr, dns, gateway);

		if (Ethernet.hardwareStatus() == EthernetNoHardware)
			Serial.println("Hardware issue.");
			delay(5000);
			goto begin;
	}
	// webserver.setDefaultCommand(&relayCmd);
	// webserver.begin();

	//-------------------------------------------------------------------------------------------------------
	// Init pins
	//-------------------------------------------------------------------------------------------------------
	// Extra Set up code:
	for (unsigned pin = 0; pin < sizeof(relayPins) / sizeof(int); pin++)
	{
		pinMode(relayPins[pin], OUTPUT); //pin selected to control
		digitalWrite(relayPins[pin], outletStates[pin] == 1 ? HIGH : LOW);
		sprintf(buf, "pin - %d, state - %d", relayPins[pin], outletStates[pin]);
		Serial.println(buf);
	}
	//-------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------
	// gdbstub_init();
	Serial.print("Server is at ");
	Serial.println(Ethernet.localIP());

	updateDelay.start(15000);
	Serial.println(updateDelay.remaining());
	swServer.begin();
}

void loop()
{
	// Serial.println("loop");
	// readCommands();
	// Serial.println(res);
	// Serial.println(updateDelay.remaining());
	if (updateDelay.justFinished()) {
		swServer.flush();
		// tmp = 1;
		Serial.println("Delay");
		sendUpdate(Ethernet.localIP());
		updateDelay.start(15000);
		// tmp = 0;
	}
	// }
	/* process incoming connections one at a time forever */
	procSwitchReq();
}
