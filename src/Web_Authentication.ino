/* Remotely controllable relay with authentitication */

#include <Arduino.h>
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

//-------------------------------------------------------------------------------------------------------
// Init outlet relay pins and state
//-------------------------------------------------------------------------------------------------------
String readString;
char inputChar;
char buf[128];
int relayPins[] = {2, 3, 4, 6};
int outletStates[] = {0, 0, 0, 0};
static uint8_t mac[] = {0xF9, 0x62, 0x30, 0x4C, 0x7D, 0x5C};
// static uint8_t ip[] = {192, 168, 1, 210}; /* ip is set dinamically through serial */
IPAddress ipAddr(192, 168, 1, 210);
bool editing = false;

P(pageStart) = "<!DOCTYPE html><html><head>"
          "<title>Webduino AJAX RGB Example</title>"
          "<link href='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/themes/base/jquery-ui.css' rel=stylesheet />"
          "<script src='http://ajax.googleapis.com/ajax/libs/jquery/1.6.4/jquery.min.js'></script>"
          "<script src='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/jquery-ui.min.js'></script>"
          "<script type='text/javascript'>"
            "console.log('is this working?');"
            "function switchRelay(relaySwitch) {"
              "console.log(relaySwitch.id);"
               "$.post('', { [relaySwitch.id]: relaySwitch.checked ? 1 : 0 });"
            "}"
          "</script>"
          "<style>"
          ".onoffswitch {"
          "position: relative; width: 90px;"
          "margin: 20px;"
          "-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;"
          "}"
          ".onoffswitch-checkbox {"
          "position: absolute;"
          "opacity: 0;"
          "pointer-events: none;"
          "}"
          ".onoffswitch-label {"
          "display: block; overflow: hidden; cursor: pointer;"
          "border: 2px solid #999999; border-radius: 20px;"
          "}"
          ".onoffswitch-inner {"
          "display: block; width: 200%; margin-left: -100%;"
          "transition: margin 0.3s ease-in 0s;"
          "}"
          ".onoffswitch-inner:before, .onoffswitch-inner:after {"
          "display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;"
          "font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;"
          "box-sizing: border-box;"
          "}"
          ".onoffswitch-inner:before {"
          "content: \"ON\";"
          "padding-left: 10px;"
          "background-color: #34A7C1; color: #FFFFFF;"
          "}"
          ".onoffswitch-inner:after {"
          "content: \"OFF\";"
          "padding-right: 10px;"
          "background-color: #EEEEEE; color: #999999;"
          "text-align: right;"
          "}"
          ".onoffswitch-switch {"
          "display: block; width: 18px; margin: 6px;"
          "background: #FFFFFF;"
          "position: absolute; top: 0; bottom: 0;"
          "right: 56px;"
          "border: 2px solid #999999; border-radius: 20px;"
          "transition: all 0.3s ease-in 0s; "
          "}"
          ".onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {"
          "margin-left: 0;"
          "}"
          ".onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {"
          "right: 0px; "
          "}"
          "</style>"
          "</head>"
          "<body>";

/* store common switch html code parts on onboard memory to free up RAM */
P(switchHtmlStart) = "<div class=\"onoffswitch\">"
                      "<input type=\"checkbox\" name=\"onoffswitch\" class=\"onoffswitch-checkbox\" ";
P(switchHtmlEnd) = "<span class=\"onoffswitch-inner\"></span><span class=\"onoffswitch-switch\"></span>"
                    "</label></div>";
P(pageEnd) = "</body></html>";

/* get string with html for (un)checked switch based on relay state */
char* HtmlSwitchChecked(char* res, int outletIndex) {
  char name[5];
  switch (outletIndex) {
    case 0:
      sprintf(name, "pc1");
      break;
    case 1:
      sprintf(name, "pc2");
      break;
    case 2:
      sprintf(name, "fgt1");
      break;
    case 3:
      sprintf(name, "fgt2");
      break;
  }
  sprintf(res, "id=\"%s\" tabindex=\"0\" %s onchange=\"switchRelay(this)\">\
          <label class=\"onoffswitch-label\" for=\"%s\">",
          name, outletStates[outletIndex] ? "checked " : " ", name);
  Serial.println(res);
  return res;
}

/* set ip dynamically */
int readCommands() {
  if (Serial.available() > 0) {
    inputChar = Serial.read();
    editing = true;
    
    Serial.println("help or setip");
    delay(3000);
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.print(input.length());
    Serial.println(input);
    
    if (input == "help" || input == "h") {
      Serial.println("setip to set ip");
    } else if (input == "setip") {
      delay(3000);
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() == 16) {
        ipAddr.fromString(input);
      } else {
        Serial.println("Wrong ip");
        return -1;
      }
      Serial.println(ipAddr);
      Ethernet.begin(mac, ipAddr);
      return 0;
    } else {
      return -1;
    }
  }
  return -1;
}

/* This creates an instance of the webserver.  By specifying a prefix
 * of(r?) "", all pages will be at the root of the server. */
#define PREFIX ""
WebServer webserver(PREFIX, 80);

void relayCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (type == WebServer::POST) {
    char relayName[16], res[16];
    int numRes;
    bool repeat;

    repeat = server.readPOSTparam(relayName, 16, res, 16);
    numRes = atoi(res);
    if (repeat) {
      Serial.println(relayName);
      Serial.println(numRes);
      if (strcmp(relayName, "pc1") == 0) {
        outletStates[0] = numRes;
        digitalWrite(relayPins[0], outletStates[0] == 1 ? HIGH : LOW);
      } else if (strcmp(relayName, "pc2") == 0) {
        outletStates[1] = numRes;
        digitalWrite(relayPins[1], outletStates[1] == 1 ? HIGH : LOW);
      } else if (strcmp(relayName, "fgt1") == 0) {
        outletStates[2] = numRes;
        digitalWrite(relayPins[2], outletStates[2] == 1 ? HIGH : LOW);
      } else if (strcmp(relayName, "fgt2") == 0) {
        outletStates[3] = numRes;
        digitalWrite(relayPins[3], outletStates[3] == 1 ? HIGH : LOW);
      }
    }
    server.httpSeeOther(PREFIX);
    return;

  }
  if (type == WebServer::GET) {

    if (server.checkCredentials("dXNlcjp1c2Vy")) {
      server.httpSuccess();
      if (type != WebServer::HEAD) {
        P(helloMsg) = "<h1>Hello User</h1>";
        server.printP(helloMsg);
      }
    }
  /* if the user has requested this page using the following credentials
   * username = admin
   * password = admin
   * in other words: "YWRtaW46YWRtaW4=" is the Base64 representation of "admin:admin" */
    else if (server.checkCredentials("YWRtaW46YWRtaW4=")) {
      server.httpSuccess();
      server.printP(pageStart);

      for (int i = 0; i < 4; i++) {
        char *switchParams = HtmlSwitchChecked(buf, i);
        Serial.println(switchParams);


        server.printP(switchHtmlStart);
        server.print(switchParams);
        server.printP(switchHtmlEnd);
      }
      server.printP(pageEnd);


    } else {
      /* send a 401 error back causing the web browser to prompt the user for credentials */
      server.httpUnauthorized();
      }
    }
  }

  void setup() {
    //-------------------------------------------------------------------------------------------------------
    // Init webserver
    //-------------------------------------------------------------------------------------------------------
    Ethernet.begin(mac, ipAddr);
    webserver.setDefaultCommand(&relayCmd);
    webserver.begin();

    //-------------------------------------------------------------------------------------------------------
    // Init pins
    //-------------------------------------------------------------------------------------------------------
    // void *a = memset(outletStates, 0, sizeof(outletStates));
    // memset(outletStates, 0, sizeof(outletStates));
    
    // Extra Set up code:
    for (unsigned pin = 0; pin < sizeof(relayPins) / sizeof(int); pin++) {
      pinMode(relayPins[pin], OUTPUT); //pin selected to control
      // Serial.print("nit pin %d ", relayPins[pin]);
    }
    //-------------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------------
    //enable serial data print
    Serial.begin(9600);

    Serial.print("Server is at ");
    Serial.println(Ethernet.localIP());
  }

  void loop() {
    // Serial.println("loop");
    int res = readCommands();
    // Serial.println(res);

    /* process incoming connections one at a time forever */
    webserver.processConnection();
  }
