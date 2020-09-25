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

void relayCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (type == WebServer::POST) {
    char relayName[16], res[16];
    bool repeat;

    repeat = server.readPOSTparam(relayName, 16, res, 16);

    if (repeat) {
      Serial.println(relayName);
      Serial.println(res);
      if (strcmp(relayName, "pc1") == 0) {
        digitalWrite(relayPins[0], atoi(res));
      } else if (strcmp(relayName, "pc2") == 0) {
        digitalWrite(relayPins[1], atoi(res));
      } else if (strcmp(relayName, "fgt1") == 0) {
        digitalWrite(relayPins[2], atoi(res));
      } else if (strcmp(relayName, "fgt2") == 0) {
        digitalWrite(relayPins[3], atoi(res));
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
      /* store the HTML in program memory using the P macro */
      P(message) =
          "<!DOCTYPE html><html><head>"
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
          "<body>"
            "<div class=\"onoffswitch\">"
              "<input type=\"checkbox\" name=\"onoffswitch\" class=\"onoffswitch-checkbox\" id=\"pc1\" tabindex=\"0\" checked onchange=\"switchRelay(this)\">"
              "<label class=\"onoffswitch-label\" for=\"outlet1\">"
                "<span class=\"onoffswitch-inner\"></span>"
                "<span class=\"onoffswitch-switch\"></span>"
              "</label>"
            "</div>"
            "<div class=\"onoffswitch\">"
              "<input type=\"checkbox\" name=\"onoffswitch\" class=\"onoffswitch-checkbox\" id=\"pc2\" tabindex=\"0\" checked onchange=\"switchRelay(this)\">"
              "<label class=\"onoffswitch-label\" for=\"outlet2\">"
                "<span class=\"onoffswitch-inner\"></span>"
                "<span class=\"onoffswitch-switch\"></span>"
              "</label>"
            "</div>"
            "<div class=\"onoffswitch\">"
              "<input type=\"checkbox\" name=\"onoffswitch\" class=\"onoffswitch-checkbox\" id=\"fgt1\" tabindex=\"0\" checked onchange=\"switchRelay(this)\">"
              "<label class=\"onoffswitch-label\" for=\"outlet3\">"
                "<span class=\"onoffswitch-inner\"></span>"
                "<span class=\"onoffswitch-switch\"></span>"
              "</label>"
            "</div>"
            "<div class=\"onoffswitch\">"
              "<input type=\"checkbox\" name=\"onoffswitch\" class=\"onoffswitch-checkbox\" id=\"fgt2\" tabindex=\"0\" checked onchange=\"switchRelay(this)\">"
              "<label class=\"onoffswitch-label\" for=\"outlet4\">"
                "<span class=\"onoffswitch-inner\"></span>"
                "<span class=\"onoffswitch-switch\"></span>"
              "</label>"
            "</div>"
          "</body>"
          "</html>";

      server.printP(message);
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
    Ethernet.begin(mac, ip);
    webserver.setDefaultCommand(&relayCmd);
    webserver.begin();

    //-------------------------------------------------------------------------------------------------------
    // Init pins
    //-------------------------------------------------------------------------------------------------------
    pc1 = 1;
    pc2 = 1;
    fgt1 = 0;
    fgt2 = 0;
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
    Serial.println("LED Controller Test 1.0");
  }

  void loop() {
    // char buff[64];
    // int len = 64;

    /* process incoming connections one at a time forever */
    webserver.processConnection();
  }
