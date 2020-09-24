#include <RichHttpServer.h>
#include <StandardCplusplus.h>
//#include <functional>

using namespace::placeholders;

// Define shorthand for common types
using RichHttpConfig = RichHttp::Generics::Configs::EspressifBuiltin;
using RequestContext = RichHttpConfig::RequestContextType;

// Use the default auth provider
SimpleAuthProvider authProvider;

// Use the builtin server (ESP8266WebServer for ESP8266, WebServer for ESP32).
// Listen on port 80.
RichHttpServer<RichHttpConfig> server(80, authProvider);

// Handlers for normal routes follow the same structure
void handleGetAbout(RequestContext& request) {
  // Respond with JSON body:
  //   {"message":"hello world"}
  request.response.json["message"] = "hello world";
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handleGetThing(RequestContext& request) {
  char buffer[100];
  sprintf("Thing ID is: %s\n", buffer, request.pathVariables.get("thing_id"));

  // Respond with JSON body:
  //   {"message":"Thing ID is..."}
  request.response.json["message"] = buffer;
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handlePutThing(RequestContext& request) {
  JsonObject body = request.getJsonBody().as<JsonObject>();

  if (body.isNull()) {
    request.response.setCode(400);
    request.response.json["error"] = F("Invalid JSON.  Must be an object.");
    return;
  }

  char buffer[100];
  sprintf("Thing ID is: %s\nBody is: %s\n", request.pathVariables.get("thing_id"), request.getBody());

  request.response.json["message"] = buffer;
}

void setup() {
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
}

void loop() {
  server.handleClient();
}