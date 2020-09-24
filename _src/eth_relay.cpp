#include <Arduino.h>

#include <RichHttpServer.h>
//#include <StandardCplusplus.h>
#include <functional>

using namespace ::placeholders;

// Define shorthand for common types
using RichHttpConfig = RichHttp::Generics::Configs::EspressifBuiltin;
using RequestContext = RichHttpConfig::RequestContextType;

// Use the default auth provider
SimpleAuthProvider authProvider;

// Use the builtin server (ESP8266WebServer for ESP8266, WebServer for ESP32).
// Listen on port 80.
RichHttpServer<RichHttpConfig> server(80, authProvider);

// Handlers for normal routes follow the same structure
void handleGetAbout(RequestContext &request)
{
  // Respond with JSON body:
  //   {"message":"hello world"}
  request.response.json["message"] = "hello world";
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handleGetThing(RequestContext &request)
{
  char buffer[100];
  sprintf("Thing ID is: %s\n", buffer, request.pathVariables.get("thing_id"));

  // Respond with JSON body:
  //   {"message":"Thing ID is..."}
  request.response.json["message"] = buffer;
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handlePutThing(RequestContext &request)
{
  JsonObject body = request.getJsonBody().as<JsonObject>();

  if (body.isNull())
  {
    request.response.setCode(400);
    request.response.json["error"] = F("Invalid JSON.  Must be an object.");
    return;
  }

  char buffer[100];
  sprintf("Thing ID is: %s\nBody is: %s\n", request.pathVariables.get("thing_id"), request.getBody());

  request.response.json["message"] = buffer;
}

int relayPins[] = {2, 3, 4, 6};
int pc1;
int pc2;
int fgt1;
int fgt2;

void setup()
{
  //-------------------------------------------------------------------------------------------------------
  // Init webserver
  //-------------------------------------------------------------------------------------------------------
  // Create some routes
  server
      .buildHandler("/about")
      .on(HTTP_GET, handleGetAbout);

  server
      .buildHandler("/things/:thing_id")
      .on(HTTP_GET, std::bind(handleGetThing, _1))
      .on(HTTP_PUT, std::bind(handlePutThing, _1));

  // Builders are cached and should be cleared after we're finished
  server.clearBuilders();

  server.begin();

  //-------------------------------------------------------------------------------------------------------
  // Init pins
  //-------------------------------------------------------------------------------------------------------
  // relayPins = {2, 3, 4, 5}
  pc1 = 1;
  pc2 = 1;
  fgt1 = 0;
  fgt2 = 0;
  // Extra Set up code:
  for (int pin = 0; pin < sizeof(relayPins) / sizeof(int); pin++)
  {
    pinMode(relayPins[pin], OUTPUT); //pin selected to control
    // Serial.print("nit pin %d ", relayPins[pin]);
  }
  //-------------------------------------------------------------------------------------------------------
  //-------------------------------------------------------------------------------------------------------
  //enable serial data print
  Serial.begin(9600);

  //start Ethernet
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  Serial.println("LED Controller Test 1.0");
}
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
}
void loop()
{
  server.handleClient();
}