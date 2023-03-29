/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "Ethernet.h"
#include "millisDelay.h"
#include <SPI.h>
// #include "AES.h"
#include "avr/wdt.h"
// #include "base64.hpp"
#include "utility/w5100.h"
// #include "secrets.h"
// #include "EEPROM.h"

// byte aesKey_in_new[N_BLOCK];// = "abcdefghijklmnop";
// byte aesKey_in_old[N_BLOCK];// = "abcdefghijklmnop";
byte psk_srv = 'S';
byte psk_client = 'C';
byte rand_byte = 0;
byte crypt = 0;
// bool is_debug = true;
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
int tmp1 = 0;
char inputChar;
char buf[80];
// char msg[80];
char big_buf[300];
// byte iv[N_BLOCK], ivInit[N_BLOCK];
int relayPins[] = {6, 7, 8, 9};
int outletStates[] = {1, 1, 1, 1};
// static uint8_t mac[] = {0x2c,0xf0,0x5d,0x8a,0xda,0x89};
static uint8_t mac[] = {0xF8, 0x62, 0x30, 0x4C, 0x7D, 0xC5};
// {0xF8, 0x62, 0x30, 0x4C, 0x7D, 0xC5};
// {0x2c,0xf0,0x5d,0x8a,0xda,0x89};
// IPAddress gateway(192, 168, 1, 1);
// IPAddress dns(8, 8, 8, 8);

EthernetClient updClient;
EthernetServer swServer(1234);
// byte updSrvAddr[] = {192, 168, 0, 113};
// IPAddress updSrvAddr(192, 168, 0, 113);
#ifndef DEBUG
static const char conn_string[] = "hosts.spduapi.azurewebsites.net";
static const char host[] = "spduapi.azurewebsites.net";
#else
static const char conn_string[] = "192.168.137.140";
static const char host[] = "192.168.137.140:6969";
#endif
const char *conTarget;

int srvPort = 6969;
// int srvPort = 7071;

// AES aes128;


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
		// Serial.printf(F("value - "));
		// Serial.println(value);
		if (value > 1) {
			value = 1;
			eeprom_update_byte(addr, value);
		}
		
		// Serial.printf(F("set outlet "));Serial.print(i);Serial.printf(F("to value "));Serial.println(value);
		// outletStates[i] = value;
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
// int getRandomBlock(byte *block) {
// 	if (!block)
// 		return -1;
	
	
// 	delay(1);
// 	memset(block, 0, 16); //setting 17 bytes overwrites msg
// 	for (int i = 0; i < 4; i++) {
// 		tmp = random(RAND_MAX);
// 		for (int j = 0; j < 4; j++) {
// 			block[i*4 + j] = ((byte*)&tmp)[j];
// 		}
// 	}
// 	Serial.print("random block of 16: ");
// 	for (int i = 0; i < 16; i++) {
// 		Serial.print(block[i]);
// 		Serial.print(' ');
// 		if (block[i] == 0)
// 			block[i] = 1;
// 		if (block[i] == '\\')
// 			block[i] = 1;
// 	}
// 	Serial.println();
// 	return 0;
// }

// int encrypt(byte *plain, byte *cipher, byte *key, byte *IV, int tlength) {
// 	Serial.println(F("Encrypt..."));
// 	int n_blocks;
// 	if (!cipher || !plain || !key || !IV || tlength <= N_BLOCK) {
// 		return -1;
// 	}
// 	// Serial.println("ivInit");
// 	// getRandomBlock(ivInit);
// 	// memcpy(iv, ivInit, N_BLOCK);
// 	aes128.clean();
// 	aes128.set_key(key, 16);
// 	Serial.print(F("message with length - "));
// 	Serial.println(tlength);
// 	Serial.write((char *)plain, tlength);
// 	n_blocks = tlength / 16;
// 	Serial.print(F("message with nlocks - "));
// 	Serial.println(n_blocks);
// 	aes128.cbc_encrypt(plain, cipher, n_blocks, iv);
// 	aes128.clean();
// 	// memcpy(iv, ivInit, N_BLOCK);
// 	Serial.write((uint8_t *)cipher, tlength);
// 	return 0;
// }

// int decrypt(byte *cipher, byte *plain, byte *key, byte* IV, int tlength) {
// 	Serial.println(F("Decrypt..."));
// 	int n_blocks;

// 	if (!cipher || !plain || !key || !IV || tlength <= N_BLOCK) {
// 		return -1;
// 	}
// 	aes128.clean();

// 	aes128.set_key(key, N_BLOCK);
// 	n_blocks = tlength / N_BLOCK;

// 	aes128.cbc_decrypt(cipher, plain, n_blocks, IV);
// 	aes128.clean();
// 	return 0;
// }

int setMsg(char *msg, int len, IPAddress ip) {
	// int ilength = 0, plength = 0, tlength = 0;
	// int b64_len = 0;
	memset(msg, 0, sizeof(len));
	// memcpy(aesKey_in_old, aesKey_in_new, N_BLOCK);
	// Serial.println("aesKey_in_new generate:");
	// getRandomBlock(aesKey_in_new);
	rand_byte = random(UINT8_MAX);
	crypt = rand_byte ^ psk_srv;
	tmp = sprintf(msg, "{\"key\": %d,\"ipAddress\": [%d,%d,%d,%d]\"", crypt, ip[0], ip[1], ip[2], ip[3]);
	// memset(msg, 0, len);
	// b64_len = encode_base64((char*)aesKey_in_new, 16, msg);
	// printArr(msg, 24);
	// Serial.print(F("length of base64 encoded key: "));
	// Serial.println(b64_len);

	// memcpy(&buf[tmp], msg, b64_len);
	// tmp += sprintf(&buf[tmp], "")
	// Serial.println((char*)aesKey_in_new);
	// tmp = tmp + b64_len;
	// Serial.printf("+++ size - %d +++\n%s\n", tmp, msg);
	tmp1 = sprintf(&msg[tmp], ",\"state\": [%d, %d, %d, %d]}", EXPAND_OUTLET_ARRAY(outletStates));
	// Serial.printf("[%d, %d, %d, %d]\n", outletStates[0], outletStates[1], outletStates[2], outletStates[3]);
	// Serial.printf("[%d, %d, %d, %d]\n", EXPAND_OUTLET_ARRAY(outletStates));
	// memcpy(&buf[tmp], "\"}", 2);
	// Serial.printf("added %d bytes\n", tmp1);
	tmp += tmp1;
	// Serial.printf("+++ size - %d +++\n%s\n", tmp, msg);
	tmp = len - 1;
	while(tmp && !msg[tmp]) {
		tmp--;
	}
	tmp += 1; //size of msg;
	// ilength = tmp;
	// Serial.print(F("size of message - "));
	// Serial.println(tmp);
	// if (((ilength) % 16) == 0) {
	// 	// Serial.print(F("message fits int number of blocks of 16 - "));
	// 	// Serial.println(ilength);
	// } else {
	// 	// Serial.print(F("message will be padded to int number of blocks of 16 - "));
	// 	plength = ((ilength/16) + 1)*16 - ilength;
	// }
	// tlength = ilength + plength;
	// Serial.println(tlength);
	// memset(msg, 0, tlength);
	// memcpy(msg, buf, tmp);
	/*PKCS7 padding*/
	// Serial.print(F("PKCS7 padding - fill with "));
	// Serial.print(plength);
	// Serial.println();
	// for (int end = ilength; end < tlength; end++) {
	// 	Serial.print(end);
	// 	msg[end] = plength;
	// }
	// Serial.write(msg, tlength);
	// Serial.println();
	// Ethernet.MACAddress(mac);
	// printArr(mac, 6);
	return tmp;
}

int sendUpdate(IPAddress ip) {
	Serial.println(F("Start sendUpdate"));
	int tlength = setMsg(buf, sizeof(buf), ip);
	if (tlength <= 0) {
		Serial.println(F("Error compositing update message."));
		return -1;
	}
	Serial.printf("set msg, bytes - %d\n================\n%s\n", tlength, buf);
	// Ethernet.MACAddress(mac);
	// printArr(mac, 6);
	// getRandomBlock(ivInit);
	// memcpy(iv, ivInit, N_BLOCK);
	// encrypt((byte *)msg, (byte*)buf, (byte*)aesKey_out, iv, tlength);
	// memcpy(iv, i)
	updClient.flush();
	// delay(100);
	tmp = 0;
	while (tmp < 10) {
			conTarget = (char *)conn_string;
			if (!updClient.connect(conTarget, srvPort)) {
				delay(100);
				tmp++;
				Serial.print(F("retry "));
				Serial.println(tmp);
			} else {
				break;
			}
		// conTarget = (byte *)conn_string;
		
	}
	if (updClient.connected()) {
		// memset(msg, 0, sizeof(msg));
		// delay(100);
		Serial.println(F("\nconnected"));

		updClient.flush();
		if (!updClient.availableForWrite()) {
			Serial.println(F("Client err."));
			return -1;
		}
		Serial.print(F("sending addr upd"));
		memset(big_buf, 0, sizeof(big_buf));
		tmp = snprintf(big_buf, sizeof(big_buf), "POST /api/upd HTTP/1.1\n"
				"User-Agent: Arduino/1.0\n"
				"Accept: */*\n"
				"Host: %s\n"
				"Connection: keep-alive\n"
				"Content-Type: text/html; charset=utf-8;\n"
				"Content-Length: %d\n\n", host, (tlength+1));
		// updClient.println("PUT /api/spdu HTTP/2.0");
		// updClient.print("Host: ");
		// updClient.write(host);
		// updClient.println();
		// updClient.println("User-Agent: Arduino/1.0");
		// updClient.println("Connection: close");
		// updClient.println("Content-Type: application/x-www-form-urlencoded;");
		// updClient.print("Content-Length: ");

		// Serial.print(F("msg length - "));
		// Serial.println(tlength);

		// updClient.println(tlength);
		// Ethernet.MACAddress(mac);
		// printArr(mac, 6);
		Serial.printf("+++ header length - %d +++\n%s\n", tmp, big_buf);
		tmp1 = snprintf(&big_buf[tmp], sizeof(big_buf) - tmp, "%s\n", buf);
		Serial.printf("+++ added bytes - %d\n===============\n%s +++\n%s\n", tmp1, buf, big_buf);
		updClient.write(big_buf, tmp + tmp1);
		// updClient.write(buf, tlength);
		// updClient.write(ivInit, N_BLOCK);
		updClient.flush();
		delay(500);

		Serial.println(F("server response"));
		char c;
		memset(big_buf, 0, sizeof(big_buf));
		while (updClient.available()) {
			c = updClient.read((byte *)big_buf, sizeof(big_buf));
			Serial.printf("read %d bytes", (int )c); Serial.print(big_buf);
			Serial.print(big_buf);
		}
		Serial.printf(F("end server response\n"));
	} else {
		if (fail_count > 2)
			return -2;

		Serial.printf(F("could not connect to update server\n"));
		Serial.printf("%d\n", fail_count);
		fail_count++;
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
		Serial.println(F("Delay"));
		Ethernet.MACAddress(mac);
		printArr(mac, 6);
		tmp = sendUpdate(Ethernet.localIP());
		Serial.println(tmp);
		if (tmp == -2 && fail_count > 1) {
			Serial.println("too many fails.. restart..");
			resetFunc();
		}
		
		updateDelay.start(6000);
		Serial.println(updateDelay.remaining());
		// swServer.flush();
	}
	/* process incoming connections */
	procSwitchReq();
}
