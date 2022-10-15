#include <ESP8266WiFi.h>
#include "credentials.h"

#define TIMEOUT_MS 2000
#define REQUEST_RECORD_LEN 6

WiFiServer server(80);
WiFiClient client;
int charCount;
char request[REQUEST_RECORD_LEN];

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
  server.begin();
}

void loop()
{
  // wait for clients while checking for power
  delay(50);
  if (digitalRead(D7) == LOW)
  {
    pwr = true;
  }
  else
  {
    pwr = false;
    ssh = false;
  }
  client = server.available();
  if (client)
  {
    // read request
    unsigned long t = millis();
    unsigned long t0 = t;
    charCount = 0;
    int newlineCount = 0;
    while (client.connected() && t - t0 <= TIMEOUT_MS && newlineCount < 4)
    {
      t = millis();

      if (client.available())
      {
        char c = client.read();
        if (charCount < REQUEST_RECORD_LEN)
        {
          request[charCount] = c;
          charCount++;
        }
        if (c == '\r' || c == '\n')
        {
          newlineCount++;
        }
        else
        {
          newlineCount = 0;
        }
      }
    }

    reply();
    client.stop();
  }
}

void reply()
{
  // crude request parsing: only check first few characters
  if (
      charCount < 6 ||
      request[0] != 'G' ||
      request[1] != 'E' ||
      request[2] != 'T' ||
      request[3] != ' ' ||
      request[4] != '/')
  {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>400 Bad Request</title></head><body>400 Bad Request</body></html>");
    client.println();
    return;
  }

  // space means end of URI, which is now "/"
  if (request[5] == ' ')
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html><head>");
    client.println("<meta charset=\"utf-8\">");
    client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<style>");
    client.println("html{min-height:100%;display:flex;justify-content:space-around;align-items:center;background:#0d0d0d;}");
    client.println("body{height:fit-content;margin:0px;padding:1.2em 1.5em 1.5em 1.5em;font:1em sans-serif;background:white;border-radius:1em;}");
    client.println("h1{margin-top:0px;}");
    client.println("</style>");
    client.println("<title>Server Control</title>");
    client.println("</head><body>");
    client.println("<h1>Server Control</h1>");
    if (pwr)
    {
      client.println("<p>Power state: ON</p>");
    }
    else
    {
      client.println("<p>Power state: OFF</p>");
    }
    if (ssh)
    {
      client.println("<p>SSH ready: YES</p>");
    }
    else
    {
      client.println("<p>SSH ready: NO</p>");
    }
    if (pwr)
    {
      client.println("<form action=\"/t\"><input type=\"submit\" value=\"Power on\" disabled></form>");
    }
    else
    {
      client.println("<form action=\"/t\"><input type=\"submit\" value=\"Power on\"></form>");
    }
    client.println("</body></html>");
    client.println();
    return;
  }

  // request URI = "/t" (call to trigger power button)
  if (request[5] == 't')
  {
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    if (pwr)
    {
      client.println("X-Message: already-on");
      client.println();
      client.println();
      return;
    }
    client.println();
    client.println();

    powerOn();

    return;
  }

  // request URI = "/s" (call to signal that SSH is ready)
  // TODO: implement security key?
  if (request[5] == 's')
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Location: /");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();
    client.println();

    ssh = true;

    return;
  }
  
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>404 Not Found</title></head><body>404 Not Found</body></html>");
  client.println();
}

void powerOn()
{
  digitalWrite(D8, HIGH);
  delay(50);
  digitalWrite(D8, LOW);
}
