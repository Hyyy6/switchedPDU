/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "Ethernet.h"
#include "WebServer.h"
// #include "ArduinoJson.h"
#include "millisDelay.h"
#include <SPI.h>
// #include "Crypto.h"
#include "AES.h"
// #include "sercerts.h"

static const char aesKey_in[] = "abcdefghijklmnop";
static const char aesKey_out[] = "abcdefghijklmnop";

//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
String readString;
millisDelay updateDelay;
int tmp = 0;
char inputChar;
char buf[256];
// byte aesBuf[256];
int relayPins[] = {2, 3, 4, 6};
int outletStates[] = {1, 1, 1, 1};
static uint8_t mac[] = {0xF9, 0x62, 0x30, 0x4C, 0x7D, 0xC5};
IPAddress ipAddr(10, 0, 1, 12);
IPAddress gateway(192, 168, 1, 1);
IPAddress dns(8, 8, 8, 8);

EthernetClient updClient;
EthernetServer swServer(1234);
byte updSrvAddr[] = {192, 168, 0, 113};
int srvPort = 7071;

AES aes128;

void encrypt() {

}


void sendUpdate(IPAddress ip) {
	// webserver.reset();
	// IPAddress *localip;
	byte *msg;
	int ilength = 0, plength = 0, tlength = 0;
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "{\"password\": \"secpass123\",\"name\": \"ard\",\"reason_to_hate\": \"padding size\",\"payload\": {\"deviceName\": \"arduino\",\"ipAddress\": \"%d.%d.%d.%d\",}}", ip[0], ip[1], ip[2], ip[3]);

	tmp = sizeof(buf) - 1;
	while(tmp && !buf[tmp]) {
		tmp--;
	}
	tmp += 1; //size of msg;
	ilength = tmp;
	// int end = tmp;
	Serial.println(F("Start sendUpdate"));
	Serial.print(F("size of message - "));
	Serial.println(ilength);
	if (((ilength) % 16) == 0) {
		Serial.print(F("message fits int number of blocks of 16 - "));
		Serial.println(ilength);
		msg = (byte*)malloc(ilength);
	} else {
		Serial.print(F("message will be padded to int number of blocks of 16 - "));
		plength = ((ilength/16) + 1)*16 - ilength;
		// Serial.println();
		msg = (byte*)malloc(ilength + plength);
	}
	tlength = ilength + plength;
	if (!msg) {
		Serial.println("Not enough memory.\n");
		return;
	}
	Serial.println(tlength);
	memset(msg, 0, tlength);
	memcpy(msg, buf, tlength);
	/*PKCS7 padding*/
	for (int end = ilength; end < tlength; end++) {
		msg[end] = plength;
	}
	// Serial.printf("n-2 = %x; n-1 = %x\n", buf[sizeof(buf) - 2], buf[sizeof(buf) - 1]);
	Serial.write(msg, tlength);
	Serial.println();

	const uint8_t *key = "abcdefghijklmnop";
	byte iv[N_BLOCK];
	memcpy(iv, key, N_BLOCK);
	aes128.clean();
	aes128.set_key((byte *)key, 16);
	// aes128.encryptBlock(test_buf, (uint8_t *)test.c_str());
	// aes128.~BlockCipher
	// Serial.println(test);
	// printArr(test_buf);
	Serial.println(F("...encrypt..."));

	// aes128.decryptBlock(test_input, test_buf);
	// printArr(test_input);
	// printArr(test_buf);
	// test.getBytes(test_buf, sizeof(test_buf));
	// printArr(test_buf);
	
	// Serial.write(msg, tlength);

	// memcpy(aesBuf, buf, sizeof(buf));
	// Serial.println((char *)aesBuf);
	// memset(aesBuf, 0, sizeof(aesBuf));
	aes128.cbc_encrypt(msg, (byte *)buf, 16, iv);
	Serial.write((char *)buf, tlength);
	Serial.println();
	
	if (updClient.connect(updSrvAddr, srvPort)) {
		if (updClient.connected())
			Serial.println(F("connected"));

		updClient.flush();
		if (!updClient.availableForWrite()) {
			Serial.println(F("Client err."));
			return;
		}
		Serial.println(F("sending addr upd"));

		updClient.println("PUT /api/spduAPI HTTP/1.1");
		updClient.print("Host: ");
		updClient.println("192.168.0.114:7071");
		updClient.println("User-Agent: Arduino/1.0");
		updClient.println("Connection: close");
		updClient.println("Content-Type: application/x-www-form-urlencoded;");
		updClient.print("Content-Length: ");

		Serial.print("msg length - ");
		Serial.println(tlength);

		updClient.println(tlength);
		updClient.println();
		// updClient.println((char *)aesBuf);
		updClient.write(buf, tlength);
		// updClient.println();
		delay(10);
	} else {
		Serial.println(F("could not connect to update server"));
	}
	tmp = 0;
	updClient.stop();
	// updClient.
	// webserver.begin();
	
	memcpy(iv, key, N_BLOCK);
	aes128.clean();
	aes128.set_key((byte *)key, 16);
	Serial.println(F("...decrypt..."));
	aes128.cbc_decrypt((byte*)buf,  msg, 16, iv);
	Serial.write(msg, tlength);
	Serial.println();
	free(msg);
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
				delay(5);
				client.readBytes(buf, sizeof(buf));
				// client.find("Content-Length:");
				// client.find("\r\n\r\n");
				// client.find("\"name\":\"");
				// int outlet = client.parseInt();
				// client.find("\"state\":\"");
				// int state = client.parseInt();


				// // sprintf(buf, "name - %d\nstate - %d", outlet, state);
				// // Serial.println(buf);


				// delay(5);
				// client.println("HTTP/1.1 200 OK");
				// client.println("Content-Type: text/html");
				// client.println("Connection: close");
				// client.println();
				// // client.println();
				// snprintf(buf, sizeof(buf), "Changed outlet #%d to \"%s\"", outlet, state ? "On" : "Off");
				// client.println(buf);

				delay(5);
				client.stop();
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

void printArr(uint8_t *arr) {
	int i;
	int size = sizeof(arr);
	// Serial.print("size of array: ");
	Serial.println(size);
	for (i = 0; i < size; i++) {
		Serial.print(arr[i]);
	}
	Serial.print("\n");
}

void setup()
{
	int j;
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
	Serial.println(F("Initializing with DHCP..."));
	// W5100.getMACAddress((uint8_t*)&tmp);
	// Serial.println(tmp);
	if (Ethernet.begin(mac) == 0) {
		Serial.println(F("Failed to configure with DHCP, using defined parameters."));
		Ethernet.begin(mac, ipAddr, dns, gateway);

		if (Ethernet.hardwareStatus() == EthernetNoHardware)
			Serial.println(F("Hardware issue."));
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
		// sprintf(buf, "pin - %d, state - %d", relayPins[pin], outletStates[pin]);
		// Serial.println(buf);
	}
	//-------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------
	// gdbstub_init();
	Serial.print(F("Server is at "));
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
		Serial.println(F("Delay"));
		sendUpdate(Ethernet.localIP());
		updateDelay.start(15000);
		// tmp = 0;
	}
	// }
	/* process incoming connections one at a time forever */
	procSwitchReq();
}
