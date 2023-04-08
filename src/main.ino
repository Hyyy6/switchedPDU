/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "Ethernet.h"
#include "millisDelay.h"
#include <SPI.h>
#include "avr/wdt.h"
#include "utility/w5100.h"
byte psk_srv = 179;
byte psk_client = 55;
#define DEBUG

#define START_MEM_ADDR 0
#define OUTLET_COUNT 4
#define EXPAND_OUTLET_ARRAY(array)	array[0], array[1], array[2], array[3]
//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
int fail_count = 0;
millisDelay updateDelay;
int tmp = 0;
char buf[80];
int relayPins[] = {6, 7, 8, 9};
int outletStates[] = {1, 1, 1, 1};
static uint8_t mac[] = {0xF8, 0x62, 0x30, 0x4C, 0x7D, 0xC5};

EthernetClient updClient;
EthernetServer swServer(1234);
#ifndef DEBUG
// static const char conn_string[] = "hosts.spduapi.azurewebsites.net";
// static const char host[] = "spduapi.azurewebsites.net";
#define conn_string "hosts.spduapi.azurewebsites.net"
#define host "spduapi.azurewebsites.net"
#define updDelay 6000 
#else
// static const char conn_string[] = "10.10.0.156";
// static const char host[] = "10.10.0.156:6969";
#define conn_string "10.10.0.156"
#define host "10.10.0.156:6969"
#define updDelay 60000 
#endif
// const char *conTarget;

int srvPort = 6969;
// int srvPort = 7071;



int setOutletState(uint8_t outlet, uint8_t state) {
	byte *ptr = START_MEM_ADDR;
	if (outlet > 3)
		return -1;

	switch (state) {
		case 0:
		case 1:
			outletStates[outlet] = state;
			eeprom_update_byte(ptr + outlet, state);
			digitalWrite(relayPins[outlet], state);
			break;
		default:
			return -1;
	}
	return 0;
}

int updateFromLocal() {
	byte *addr;
	byte value = 0;
	for(int i = 0; i < OUTLET_COUNT; i++) {
		addr = (byte *)START_MEM_ADDR + i;
		value = eeprom_read_byte(addr);
		if (value > 1) {
			value = 1;
			eeprom_update_byte(addr, value);
		}
		setOutletState(i, value);
	}
	
	return 0;

}
int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval); 
}

void ShowSockStatus() {
  for (int i = 0; i < MAX_SOCK_NUM; i++) {
    Serial.print(F("Socket#"));
    Serial.print(i);
    uint8_t s = W5100.readSnSR(i);
    Serial.print(F(":0x"));
    Serial.print(s,16);
    Serial.print(F(" "));
    Serial.print(W5100.readSnPORT(i));
    Serial.print(F(" D:"));
    uint8_t dip[4];
    W5100.readSnDIPR(i, dip);
    for (int j=0; j<4; j++) {
      Serial.print(dip[j],10);
      if (j<3) Serial.print(".");
    }
    Serial.print(F("("));
    Serial.print(W5100.readSnDPORT(i));
    Serial.println(F(")"));
  }
}

int printArr(void *arr, int length) {
	if (arr == NULL) {
		return -1;
	}
	int i = 0;
	byte *ptr = (byte *)arr;

	Serial.print(F("arr with sz - "));
	Serial.println(length);
	for (i = 0; i < length; i++) {
		Serial.print(ptr[i]);
		Serial.print(' ');
	}
	Serial.println();

	return 0;
}

int setMsg(char *msg, int len, IPAddress ip) {
	byte rand_byte = 0;
	byte crypt;
	memset(msg, 0, sizeof(len));
	rand_byte = random(UINT8_MAX);
	crypt = rand_byte ^ psk_srv;
	tmp = sprintf(msg, "{\"key\": %d,\"ipAddress\": [%d,%d,%d,%d]", crypt, ip[0], ip[1], ip[2], ip[3]);
	sprintf(&msg[tmp], ",\"state\": [%d, %d, %d, %d]}", EXPAND_OUTLET_ARRAY(outletStates));
	tmp = len - 1;
	while(tmp && !msg[tmp]) {
		tmp--;
	}
	tmp += 1;
	return tmp;
}

int sendUpdate(IPAddress ip) {
	int fail = 0;
begin:
	Serial.println(F("Start sendUpdate"));
	int tlength = setMsg(buf, sizeof(buf), ip);
	if (tlength <= 0) {
		Serial.println(F("Error compositing update message."));
		return -1;
	}
	Serial.printf("set msg, bytes - %d\n================\n%s\n", tlength, buf);
	tmp = 0;
	while (tmp < 10) {
			// conTarget = (char *)conn_string;
			if (!updClient.connect(conn_string, srvPort)) {
				delay(100);
				tmp++;
				Serial.print(F("retry "));
				Serial.println(tmp);
			} else {
				break;
			}
	}
	if (updClient.connected()) {
		Serial.println(F("\nconnected"));

		updClient.flush();
		if (!updClient.availableForWrite()) {
			Serial.println(F("Client err."));
			return -1;
		}
		Serial.print(F("sending addr upd"));
		String data = "PUT /api/upd HTTP/1.1\r\n";
		data += "User-Agent: Arduino/1.0\r\n";
		data += "Accept: */*\r\n";
		data +="Host: ";
		data += host;
		data += "\r\nConnection: close\r\n";
				data += "Content-Type: text/html; charset=utf-8;\r\n";
				data += "Content-Length: ";//%d\n\n", host, (tlength+1));
				data += tlength;
				data += "\r\n\r\n";
		// updClient.write(data.begin(), data.length());

		data += buf;
		updClient.write(data.begin(), data.length());


		char c;
		tmp = 0;
		data = "";
		while (!updClient.available()) {/*Serial.print("delay");*/delay(1);}

		while (updClient.available()) {
			c = updClient.read();
			// Serial.print(c);
			// data += updClient.read();
			data += c;
			tmp++;
		}
		Serial.printf("server response, bytes read - %d, len - %d\n", tmp, data.length());
		// for (tmp = 0; tmp < data.length(); tmp++) {
			Serial.print(data);
		// }
		Serial.printf(F("end server response\n"));
		if (!strstr(data.c_str(), "HTTP/1.1 200 OK") || !strstr(data.c_str(), "Success")) {
			Serial.printf("Bad response. Retry #%d\n", fail);
			delay(10000);
			fail++;
			goto begin;
		} else {
			Serial.println("Update sent.");
		}
	} else {

	}
	tmp = 0;
	updClient.flush();
	updClient.stop();
	ShowSockStatus();
	return 0;
}

static const char name_parse[] = "\"name\": ";
static const char state_parse[] = "\"state\": ";
static const char code_parse[] = "\"code\": ";

void procSwitchReq(void) 
{
	int outlet = -1;
	int state = -1;
	tmp = -1;
	char *ptr;
	int content_length = 0;
	byte code = 0;
	EthernetClient client = swServer.available();
	if (client) {
		memset(buf, 0, sizeof(buf));
		Serial.printf(F("client\n"));
		while (client.connected()) {
			Serial.printf(F("connected\n"));
			if (client.available()) {
				Serial.printf(F("incoming request\n"));
				// Serial.write()
				delay(5);
				// client.readBytes(buf, sizeof(buf));
				client.find("Content-Length:");
				content_length = client.parseInt();

				client.find("\r\n\r\n");
				
				tmp = client.readBytesUntil(0, buf, sizeof(buf));
				Serial.print(F("size of incoming msg without header: "));
				Serial.println(tmp);
				if (tmp != content_length) {
					Serial.println(F("Invalid request."));
					return;
				}
				// content_length = content_length - 16;
				// memcpy(iv, &buf[content_length], 16);
				// printArr(iv, N_BLOCK);
				// memset(msg, 0, sizeof(msg));
				// memcpy(msg, buf, sizeof(buf));

				// memcpy(ivInit, iv, N_BLOCK);
				// decrypt((byte*)buf, (byte*)msg, aesKey_in_new, iv, content_length);
				Serial.println(buf);
				// if (!strstr(msg, "name") || !strstr(msg, "state")) {
				// 	memset(msg, 0, sizeof(msg));
				// 	memcpy(iv, ivInit, N_BLOCK);
				// 	decrypt((byte*)buf, (byte*)msg, aesKey_in_old, iv, content_length);
				// }
				ptr = strstr(buf, name_parse);
				if (ptr) {
					Serial.println(&(ptr[sizeof(name_parse) - 1]));
					outlet = atoi(&(ptr[sizeof(name_parse) - 1])); //name_parse len is +1 because of null-terminator

				} else {
					Serial.println(F("invalid outlet number"));
					Serial.println(ptr - buf);
					Serial.println(ptr);
					goto out;
				}

				ptr = strstr(buf, state_parse);
				if (ptr) {
					Serial.println(&(ptr[sizeof(state_parse) - 1]));
					state = atoi(&(ptr[sizeof(state_parse) - 1])); //state_parse is also +1
				} else {
					Serial.println(F("invalid state"));
					Serial.println(ptr - buf);
					Serial.println(ptr);
					goto out;
				}
				ptr = strstr(buf, code_parse);
				if (ptr) {
					Serial.println(&(ptr[sizeof(code_parse) - 1]));
					code = atoi(&(ptr[sizeof(code_parse) - 1])); //code_parse is also +1
				} else {
					Serial.println(F("invalid code"));
					Serial.println(ptr - buf);
					Serial.println(ptr);
					goto out;
				}

				memset(buf, 0, sizeof(buf));
				sprintf(buf, "outlet - %d\nstate - %d", outlet, state);
				Serial.printf("%s\n", buf);

				if ((outlet > -1 && outlet < 4) && (state == 0 || state == 1)) {
					tmp = 0;
					setOutletState(outlet, state);
					// outletStates[outlet] = state;
					// digitalWrite(relayPins[outlet], state);
				}

				delay(5);
				if (!tmp)
					client.println("HTTP/1.1 200 OK");
				else
					client.println("HTTP/1.1 400 Bad Request");
				client.println("Content-Type: text/html");
				client.println("Connection: close");
				client.println();
				if (!tmp) {
					client.print("successfully updated ");
				} else {
					client.print("failed to update ");
				}
				client.write(buf);
				client.println();

				delay(5);
				client.stop();
				updateDelay.finish();
			}
		}
	}
out:
	delay(1);
	client.stop();
	delay(1);
	memset(buf, 0, sizeof(buf));
	// memset(msg, 0, sizeof(msg));
	return;
}

void resetFunc() {
	Serial.printf(F("reset...\n"));
	wdt_enable(WDTO_15MS);
	while(1);
}

void setup()
{
	MCUSR = 0;
	tmp = 0;
	Serial.begin(9600);
	while (!Serial); // wait for serial port
	updateFromLocal();
begin:
	fail_count = 0;

	//-------------------------------------------------------------------------------------------------------
	// Init webserver
	//-------------------------------------------------------------------------------------------------------
	Serial.printf(F("Initializing with DHCP...\n"));
	printArr(mac, sizeof(mac));
	if (Ethernet.begin(mac, 5000, 5000) == 0) {
		Serial.printf(F("Failed to configure with DHCP, using defined parameters.\n"));
		// Ethernet.begin(mac, ipAddr, dns, gateway);
			Ethernet.MACAddress(mac);
			printArr(mac, sizeof(mac));
		if (Ethernet.hardwareStatus() == EthernetNoHardware)
			Serial.printf(F("Hardware issue.\n"));

		delay(5000);
		goto begin;
	}

	//-------------------------------------------------------------------------------------------------------
	// Init pins
	//-------------------------------------------------------------------------------------------------------
	// Extra Set up code:
	for (unsigned pin = 0; pin < sizeof(relayPins) / sizeof(int); pin++)
	{
		pinMode(relayPins[pin], OUTPUT); //pin selected to control
		digitalWrite(relayPins[pin], outletStates[pin] == 1 ? HIGH : LOW);
	}
	//-------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------
	// gdbstub_init();
	// Serial.printf(F("Server is at "));
	Ethernet.MACAddress(mac);
	printArr(mac, 6);
	Serial.println(Ethernet.localIP());
	// if (Ethernet.localIP()[0] == 200) {
	// 	Serial.println(F("Debug mode."));
	// 	// is_debug = true;
	// } else {
	// 	Serial.println(F("Deployment mode"));
	// 	// is_debug = false;
	// }
	updateDelay.start(5000);
	Serial.println(updateDelay.remaining());
	swServer.begin();
	updClient.setConnectionTimeout(2000);
	updClient.setTimeout(2000);
	randomSeed(updateDelay.remaining());
}

void loop()
{
	if (updateDelay.justFinished()) {
		Serial.print(F("ram available: "));
		Serial.println(freeRam());
		// swServer.flush();
		// Serial.println(F("Delay"));
		Ethernet.MACAddress(mac);
		printArr(mac, 6);
		tmp = sendUpdate(Ethernet.localIP());
		Serial.println(tmp);
		if (tmp == -2 && fail_count > 1) {
			Serial.println("too many fails.. restart..");
			resetFunc();
		}
		
		updateDelay.start(updDelay);
		Serial.println(updateDelay.remaining());
		// swServer.flush();
	}
	/* process incoming connections */
	procSwitchReq();
}
