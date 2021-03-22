/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "Ethernet.h"
#include "millisDelay.h"
#include <SPI.h>
#include "AES.h"
#include "avr/wdt.h"
#include "base64.hpp"
#include "utility/w5100.h"
#include "secrets.h"

byte aesKey_in_new[N_BLOCK];// = "abcdefghijklmnop";
byte aesKey_in_old[N_BLOCK];// = "abcdefghijklmnop";
bool is_debug = true;

//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
int fail_count = 0;
millisDelay updateDelay;
int tmp = 0;
char inputChar;
char buf[128];
char msg[128];
byte iv[N_BLOCK], ivInit[N_BLOCK];
int relayPins[] = {6, 7, 8, 9};
int outletStates[] = {1, 1, 1, 1};
static uint8_t mac[] = {0xF9, 0x62, 0x30, 0x4C, 0x7D, 0xC5};
IPAddress gateway(192, 168, 1, 1);
IPAddress dns(8, 8, 8, 8);

EthernetClient updClient;
EthernetServer swServer(1234);
byte updSrvAddr[] = {192, 168, 0, 113};
static const char conn_string[] = "hosts.spduapi.azurewebsites.net";
static const char host[] = "spduapi.azurewebsites.net";
byte *conTarget;

int srvPort = 80;

AES aes128;


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
	byte *ptr = arr;

	Serial.print(F("arr with sz - "));
	Serial.println(length);
	for (i = 0; i < length; i++) {
		Serial.print(ptr[i]);
		Serial.print(' ');
	}
	Serial.println();

	return 0;
}
int getRandomBlock(byte *block) {
	if (!block)
		return -1;
	
	
	delay(1);
	memset(block, 0, 16); //setting 17 bytes overwrites msg
	for (int i = 0; i < 4; i++) {
		tmp = random(RAND_MAX);
		for (int j = 0; j < 4; j++) {
			block[i*4 + j] = ((byte*)&tmp)[j];
		}
	}
	Serial.print("random block of 16: ");
	for (int i = 0; i < 16; i++) {
		Serial.print(block[i]);
		Serial.print(' ');
		if (block[i] == 0)
			block[i] = 1;
		if (block[i] == '\\')
			block[i] = 1;
	}
	Serial.println();
	return 0;
}

int encrypt(byte *plain, byte *cipher, byte *key, byte *IV, int tlength) {
	Serial.println(F("Encrypt..."));
	int n_blocks;
	if (!cipher || !plain || !key || !IV || tlength <= N_BLOCK) {
		return -1;
	}
	// Serial.println("ivInit");
	// getRandomBlock(ivInit);
	// memcpy(iv, ivInit, N_BLOCK);
	aes128.clean();
	aes128.set_key(key, 16);
	Serial.print(F("message with length - "));
	Serial.println(tlength);
	Serial.write((char *)plain, tlength);
	n_blocks = tlength / 16;
	Serial.print(F("message with nlocks - "));
	Serial.println(n_blocks);
	aes128.cbc_encrypt(plain, cipher, n_blocks, iv);
	aes128.clean();
	// memcpy(iv, ivInit, N_BLOCK);
	Serial.write((uint8_t *)cipher, tlength);
	return 0;
}

int decrypt(byte *cipher, byte *plain, byte *key, byte* IV, int tlength) {
	Serial.println(F("Decrypt..."));
	int n_blocks;

	if (!cipher || !plain || !key || !IV || tlength <= N_BLOCK) {
		return -1;
	}
	aes128.clean();

	aes128.set_key(key, N_BLOCK);
	n_blocks = tlength / N_BLOCK;

	aes128.cbc_decrypt(cipher, plain, n_blocks, IV);
	aes128.clean();
	return 0;
}

int setMsg(byte *msg, int len, IPAddress ip) {
	int ilength = 0, plength = 0, tlength = 0;
	int b64_len = 0;
	memset(buf, 0, sizeof(buf));
	memcpy(aesKey_in_old, aesKey_in_new, N_BLOCK);
	Serial.println("aesKey_in_new generate:");
	getRandomBlock(aesKey_in_new);
	tmp = sprintf(buf, "{\"password\": \"%s\",\"ipAddress\": \"%d.%d.%d.%d\",\"key\": \"", password, ip[0], ip[1], ip[2], ip[3]);
	memset(msg, 0, len);
	b64_len = encode_base64((char*)aesKey_in_new, 16, msg);
	printArr(msg, 24);
	Serial.print(F("length of base64 encoded key: "));
	Serial.println(b64_len);
	Serial.println((char*)msg);

	memcpy(&buf[tmp], msg, b64_len);
	Serial.println((char*)aesKey_in_new);
	tmp = tmp + b64_len;
	memcpy(&buf[tmp], "\"}", 2);
	tmp = sizeof(buf) - 1;
	while(tmp && !buf[tmp]) {
		tmp--;
	}
	tmp += 1; //size of msg;
	ilength = tmp;
	Serial.print(F("size of message - "));
	Serial.println(ilength);
	if (((ilength) % 16) == 0) {
		Serial.print(F("message fits int number of blocks of 16 - "));
		Serial.println(ilength);
	} else {
		Serial.print(F("message will be padded to int number of blocks of 16 - "));
		plength = ((ilength/16) + 1)*16 - ilength;
	}
	tlength = ilength + plength;
	Serial.println(tlength);
	memset(msg, 0, tlength);
	memcpy(msg, buf, tlength);
	/*PKCS7 padding*/
	Serial.print(F("PKCS7 padding - fill with "));
	Serial.print(plength);
	Serial.println();
	for (int end = ilength; end < tlength; end++) {
		Serial.print(end);
		msg[end] = plength;
	}
	Serial.write(msg, tlength);
	Serial.println();
	return tlength;
}

int sendUpdate(IPAddress ip) {
	Serial.println(F("Start sendUpdate"));
	int tlength = setMsg((byte *)msg, sizeof(msg), ip);
	if (tlength <= 0) {
		Serial.println(F("Error compositing update message."));
		return -1;
	}
	Serial.write(msg, tlength);

	getRandomBlock(ivInit);
	memcpy(iv, ivInit, N_BLOCK);
	encrypt((byte *)msg, (byte*)buf, (byte*)aesKey_out, iv, tlength);
	// memcpy(iv, i)
	updClient.flush();
	delay(100);
	tmp = 0;
	while (tmp < 10) {
		if (is_debug) {
			conTarget = updSrvAddr;
		} else {
			conTarget = (byte *)conn_string;
		}
		conTarget = (byte *)conn_string;
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
		memset(msg, 0, sizeof(msg));
		delay(100);
		Serial.println(F("connected"));

		updClient.flush();
		if (!updClient.availableForWrite()) {
			Serial.println(F("Client err."));
			return -1;
		}
		Serial.println(F("sending addr upd"));

		updClient.println("PUT /api/spduAPI HTTP/1.1");
		updClient.print("Host: ");
		updClient.write(host);
		updClient.println();
		updClient.println("User-Agent: Arduino/1.0");
		updClient.println("Connection: close");
		updClient.println("Content-Type: application/x-www-form-urlencoded;");
		updClient.print("Content-Length: ");

		Serial.print(F("msg length - "));
		Serial.println(tlength);

		updClient.println(tlength + N_BLOCK);
		updClient.println();
		updClient.write(buf, tlength);
		updClient.write(ivInit, N_BLOCK);
		delay(100);

		Serial.println(F("server response"));
		char c;
		while (updClient.available()) {
			c = updClient.read();
			Serial.print(c);
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

void procSwitchReq(void) 
{
	int outlet = -1;
	int state = -1;
	tmp = -1;
	char *ptr;
	int content_length = 0;
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
				content_length = content_length - 16;
				memcpy(iv, &buf[content_length], 16);
				printArr(iv, N_BLOCK);
				memset(msg, 0, sizeof(msg));

				memcpy(ivInit, iv, N_BLOCK);
				decrypt((byte*)buf, (byte*)msg, aesKey_in_new, iv, content_length);
				Serial.println(msg);
				if (!strstr(msg, "name") || !strstr(msg, "state")) {
					memset(msg, 0, sizeof(msg));
					memcpy(iv, ivInit, N_BLOCK);
					decrypt((byte*)buf, (byte*)msg, aesKey_in_old, iv, content_length);
				}
				ptr = strstr(msg, name_parse);
				if (ptr) {
					Serial.println(&(ptr[sizeof(name_parse) - 1]));
					outlet = atoi(&(ptr[sizeof(name_parse) - 1])); //name_parse len is +1 because of null-terminator

				} else {
					Serial.println(F("invalid outlet number"));
					Serial.println(ptr - msg);
					Serial.println(ptr);
					goto out;
				}

				ptr = strstr(msg, state_parse);
				if (ptr) {
					Serial.println(&(ptr[sizeof(state_parse) - 1]));
					state = atoi(&(ptr[sizeof(state_parse) - 1])); //state_parse is also +1
				} else {
					Serial.println(F("invalid state"));
					Serial.println(ptr - msg);
					Serial.println(ptr);
					goto out;
				}

				memset(buf, 0, sizeof(buf));
				sprintf(buf, "outlet - %d\nstate - %d", outlet, state);
				Serial.printf("%s\n", buf);

				if ((outlet > -1 && outlet < 4) && (state == 0 || state == 1)) {
					tmp = 0;
					outletStates[outlet] = state;
					digitalWrite(relayPins[outlet], state);
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
	memset(msg, 0, sizeof(msg));
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
	begin:
	fail_count = 0;

	//-------------------------------------------------------------------------------------------------------
	// Init webserver
	//-------------------------------------------------------------------------------------------------------
	Serial.printf(F("Initializing with DHCP...\n"));
	if (Ethernet.begin(mac) == 0) {
		Serial.printf(F("Failed to configure with DHCP, using defined parameters.\n"));
		// Ethernet.begin(mac, ipAddr, dns, gateway);

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
	Serial.printf(F("Server is at "));
	Serial.println(Ethernet.localIP());
	if (Ethernet.localIP()[0] == 192) {
		Serial.println(F("Debug mode."));
		is_debug = true;
	} else {
		Serial.println(F("Deployment mode"));
		is_debug = false;
	}
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
		swServer.flush();
		Serial.println(F("Delay"));
		tmp = sendUpdate(Ethernet.localIP());
		Serial.println(tmp);
		if (tmp == -2 && fail_count > 1) {
			Serial.println("too many fails.. restart..");
			resetFunc();
		}
		
		updateDelay.start(600000);
		Serial.println(updateDelay.remaining());
		swServer.flush();
	}
	/* process incoming connections */
	procSwitchReq();
}
