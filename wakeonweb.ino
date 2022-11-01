#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "credentials.h"

#define TIMEOUT_MS 2000
#define REQUEST_RECORD_LEN 6

ESP8266WebServer server(80);
const char bodyRoot[] PROGMEM =
    "<!DOCTYPE html><html><head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"stylesheet\" href=\"/main.css\">"
    "<link rel=\"stylesheet\" href=\"/state.css\">"
    "<link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\" sizes=\"any\">"
    "<title>Server Control</title>"
    "</head><body>"
    "<h1>Server Control</h1>"
    "<p>Power state: <span class=\"state pwroff\">OFF</span><span class=\"state pwron\">ON</span></p>"
    "<p>SSH ready: <span class=\"state sshno\">NO</span><span class=\"state sshyes\">YES</span></p>"
    "<form action=\"/t\"><input class=\"pwroff\" type=\"submit\" value=\"Power on\"><input class=\"pwron\" type=\"submit\" value=\"Power on\" disabled></form>"
    "</body></html>";
const char bodyMainStylesheet[] PROGMEM =
    "html{min-height:100%;display:flex;justify-content:space-around;align-items:center;background:#0d0d0d;}"
    "body{height:fit-content;margin:0px;padding:1.2em 1.5em 1.5em 1.5em;font:1em sans-serif;background:white;border-radius:1em;}"
    "h1{margin-top:0px;}"
    ".state{padding:0.2em 0.8em;background:#6c6c6c;color:white;border-radius:1em;}"
    ".state.pwron,.state.sshyes{background:#008a3c;}.state.pwroff,.state.sshno{background:#b30000;}";
const char bodyFavicon[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<circle style=\"fill:#d7cb02;\" cx=\"12\" cy=\"12\" r=\"12\" />"
    "<path style=\"fill:#000000;\" d=\"M 16.763931,12 C 16.763662,10.895812 15.868608,10.000758 14.76442,10.000488 L 12,9.9999998 13,2 H 12 L 7.4487305,11.106445 C 7.3095717,11.383787 7.2368536,11.689704 7.2363281,12 c 2.694e-4,1.104188 0.8953234,1.999242 1.9995117,1.999512 L 12,14 11,22 h 1 l 4.551529,-9.106445 C 16.690688,12.616213 16.763406,12.310296 16.763931,12 Z\" />"
    "</svg>";

boolean pwr = false;
boolean ssh = false;

void setup()
{
  pinMode(D7, INPUT);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);

  Serial.begin(115200);

  Serial.println("Connecting");
  // WIFI_SSID and WIFI_PASS definitions in "credentials.h"
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  Serial.println();
  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  server.keepAlive(false);
  server.on("/", handleRoot);
  server.on("/main.css", handleMainStylesheet);
  server.on("/state.css", handleStateStylesheet);
  server.on("/favicon.svg", handleFavicon);
  server.on("/t", handleTrigger);
  server.on("/s", handleSshNotification);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
}

void loop()
{
  // wait for clients while checking for power
  if (digitalRead(D7) == LOW)
  {
    pwr = true;
  }
  else
  {
    pwr = false;
    ssh = false;
  }
  server.handleClient();
}

void handleRoot()
{
  // TODO: enable caching
  server.send(200, "text/html", bodyRoot);
}

void handleMainStylesheet()
{
  // TODO: enable caching
  server.send(200, "text/css", bodyMainStylesheet);
}

void handleStateStylesheet()
{
  // TODO: authentication
  if (pwr)
  {
    if (ssh)
    {
      server.send(200, "text/css", ".pwroff{display:none;}.sshno{display:none;}");
    }
    else
    {
      server.send(200, "text/css", ".pwroff{display:none;}.sshyes{display:none;}");
    }
  }
  else
  {
    if (ssh)
    {
      server.send(200, "text/css", ".pwron{display:none;}.sshno{display:none;}");
    }
    else
    {
      server.send(200, "text/css", ".pwron{display:none;}.sshyes{display:none;}");
    }
  }
}

void handleFavicon()
{
  server.send(200, "image/svg+xml", bodyFavicon);
}

void handleTrigger()
{
  // TODO: authentication
  if (pwr)
  {
    server.send(409, "text/plain", "409 Conflict\n\nServer is already powered on.");
    return;
  }
  powerOn();
  server.sendHeader("Location", "/", false);
  server.send(302, "text/plain", "302 Found\n\nServer powered on successfully.");
}

void handleSshNotification()
{
  // TODO: authentication
  ssh = true;
  server.send(200, "text/plain", "200 OK\n\nSSH service marked as ready.");
}

void handleNotFound()
{
  server.send(404, "text/plain", "404 Not Found");
}

void powerOn()
{
  digitalWrite(D8, HIGH);
  delay(50);
  digitalWrite(D8, LOW);
}
