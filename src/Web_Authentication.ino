/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
String readString;
int relayPins[] = {2, 3, 4, 6};
int pc1 = 0;
int pc2 = 0;
int fgt1 = 0;
int fgt2 = 0;

/* CHANGE THIS TO YOUR OWN UNIQUE VALUE.  The MAC number should be
 * different from any other devices on your network or you'll have
 * problems receiving packets. */
static uint8_t mac[] = {0xF9, 0x62, 0x30, 0x4C, 0x7D, 0x5C};

/* CHANGE THIS TO MATCH YOUR HOST NETWORK.  Most home networks are in
 * the 192.168.0.XXX or 192.168.1.XXX subrange.  Pick an address
 * that's not in use and isn't going to be automatically allocated by
 * DHCP from your router. */
static uint8_t ip[] = {192, 168, 1, 210};

/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
#define PREFIX ""
WebServer webserver(PREFIX, 80);

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  Serial.println("default");
  server.httpSuccess();
  if (type != WebServer::HEAD)
  {
    P(helloMsg) = "<h1>Hello, World!</h1><a href=\"private.html\">Private page</a>";
    server.printP(helloMsg);
  }
}

void privateCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  /* if the user has requested this page using the following credentials
   * username = user
   * password = user
   * display a page saying "Hello User"
   *
   * the credentials have to be concatenated with a colon like
   * username:password
   * and encoded using Base64 - this should be done outside of your Arduino
   * to be easy on your resources
   *
   * in other words: "dXNlcjp1c2Vy" is the Base64 representation of "user:user"
   *
   * if you need to change the username/password dynamically please search
   * the web for a Base64 library */
  if (server.checkCredentials("dXNlcjp1c2Vy"))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
      P(helloMsg) = "<h1>Hello User</h1>";
      server.printP(helloMsg);
    }
  }
  /* if the user has requested this page using the following credentials
   * username = admin
   * password = admin
   * display a page saying "Hello Admin"
   *
   * in other words: "YWRtaW46YWRtaW4=" is the Base64 representation of "admin:admin" */
  else if (server.checkCredentials("YWRtaW46YWRtaW4="))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
      P(helloMsg) =
          "<HTML>\
            <HEAD>\
              <TITLE>Home Automation</TITLE>\
              <center>\
            </HEAD>\
            <BODY>\
              <H1>Home Automation</H1>\
              <hr />\
              <center>\
              <a href=\"?pc1on\"\">PC1 on</a>\
              <br />\
              <a href=\"?pc1off\"\">PC1 off</a><br />\
              <br />\
              <br />\
              <a href=\"?pc2on\"\">PC2 on</a>\
              <br />\
              <a href=\"?pc2off\"\">PC2 off</a><br />\
              <br />\
              <br />\
              <a href=\"?fgt1on\"\">FGT1 on</a>\
              <br />\
              <a href=\"?fgt1off\"\">FGT1 off</a><br />\
              <br />\
              <br />\
              <a href=\"?fgt2on\"\">FGT2 on</a>\
              <br />\
              <a href=\"?fgt2off\"\">FGT2 off</a><br />\
              <br />\
              <br />\
            </BODY>\
          </HTML>";

      //P(helloMsg) = page;
      server.printP(helloMsg);
    }
  }
  else
  {
    /* send a 401 error back causing the web browser to prompt the user for credentials */
    server.httpUnauthorized();
  }

  EthernetClient client = server.available();
  if (client)
  {
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100)

        {

          //store characters to string
          readString += c;
          //Serial.print(c);

          Serial.write(c);
          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so you can send a reply
          //if HTTP request has ended
          if (c == '\n')
          {
            Serial.println(readString); //print to serial monitor for debuging

            if (readString.indexOf("?pc1on") > 0) //checks for on
            {
              digitalWrite(2, HIGH); // set pin 2 high
              Serial.println("PC1 on");
            }
            else if (readString.indexOf("?pc1off") > 0) //checks for off
            {
              digitalWrite(2, LOW); // set pin 2 low
              Serial.println("PC1 off");
            }

            else if (readString.indexOf("?pc2on") > 0) //checks for on
            {
              digitalWrite(3, HIGH); // set pin 3 high
              Serial.println("PC2 on");
            }
            else if (readString.indexOf("?pc2off") > 0) //checks for off
            {
              digitalWrite(3, LOW); // set pin 3 low
              Serial.println("PC2 off");
            }

            else if (readString.indexOf("?fgt1on") > 0) //checks for on
            {
              digitalWrite(4, HIGH); // set pin 4 high
              Serial.println("FGT1 on");
            }
            else if (readString.indexOf("?fgt1off") > 0) //checks for off
            {
              digitalWrite(4, LOW); // set pin 4 low
              Serial.println("FGT1 off");
            }

            else if (readString.indexOf("?fgt2on") > 0) //checks for on
            {
              digitalWrite(6, HIGH); // set pin 5 high
              Serial.println("FGT2 on");
            }
            else if (readString.indexOf("?fgt2off") > 0) //checks for off
            {
              digitalWrite(6, LOW); // set pin 5 low
              Serial.println("FGT2 off");
            }

            //clearing string for next read
            readString = "";

            // give the web browser time to receive the data
            delay(1);
          }
        }
      }
    }
  }
}

void setup()
{
  //-------------------------------------------------------------------------------------------------------
  // Init webserver
  //-------------------------------------------------------------------------------------------------------
  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index.html", &defaultCmd);
  webserver.addCommand("private.html", &privateCmd);
  webserver.begin();

  //-------------------------------------------------------------------------------------------------------
  // Init pins
  //-------------------------------------------------------------------------------------------------------
  pc1 = 1;
  pc2 = 1;
  fgt1 = 0;
  fgt2 = 0;
  // Extra Set up code:
  for (unsigned pin = 0; pin < sizeof(relayPins) / sizeof(int); pin++)
  {
    pinMode(relayPins[pin], OUTPUT); //pin selected to control
    // Serial.print("nit pin %d ", relayPins[pin]);
  }
  //-------------------------------------------------------------------------------------------------------
  //-------------------------------------------------------------------------------------------------------
  //enable serial data print
  Serial.begin(9600);

  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  Serial.println("LED Controller Test 1.0");
}

void loop()
{
  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
}
