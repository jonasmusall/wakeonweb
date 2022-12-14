#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
// define WIFI_SSID and WIFI_PASS, optionally SITE_USER, SITE_PASS, SSH_USER and SSH_PASS (not your actual SSH credentials!) in credentials.h
#include "credentials.h"
#include "web.h"

// UPDATE CACHE_ETAG EVERY TIME A CACHEABLE RESPONSE BODY IS MODIFIED OR ADDED!
// cacheable URIs: "/", "/main.css", "/favicon.svg", "/favicon.ico"
#define CACHE_ETAG "0004"
#define CACHE_CONTROL "max-age=604800, immutable" // one week

/* ---- String constants for HTTP Date format construction ---- */

const char dayName0[] PROGMEM = "Mon";
const char dayName1[] PROGMEM = "Tue";
const char dayName2[] PROGMEM = "Wed";
const char dayName3[] PROGMEM = "Thu";
const char dayName4[] PROGMEM = "Fri";
const char dayName5[] PROGMEM = "Sat";
const char dayName6[] PROGMEM = "Sun";
const char monthName00[] PROGMEM = "Jan";
const char monthName01[] PROGMEM = "Feb";
const char monthName02[] PROGMEM = "Mar";
const char monthName03[] PROGMEM = "Apr";
const char monthName04[] PROGMEM = "May";
const char monthName05[] PROGMEM = "Jun";
const char monthName06[] PROGMEM = "Jul";
const char monthName07[] PROGMEM = "Aug";
const char monthName08[] PROGMEM = "Sep";
const char monthName09[] PROGMEM = "Oct";
const char monthName10[] PROGMEM = "Nov";
const char monthName11[] PROGMEM = "Dec";
const char *const dayTable[] = {dayName0, dayName1, dayName2, dayName3, dayName4, dayName5, dayName6};
const char *const monthTable[] =
    {monthName00, monthName01, monthName02, monthName03, monthName04, monthName05, monthName06, monthName07, monthName08, monthName09, monthName10, monthName11};
char cacheDate[] = "DDD, dd MMM yyyy hh:mm:ss GMT";

/* ---- Runtime variables ---- */

ESP8266WebServer server(80);

boolean pwr = false;
boolean ssh = false;

/* ---- Setup functions ---- */

// Gets the current real time from the internet
void setupClock()
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Synchrozining time");
  time_t now = time(nullptr);
  while (now < 1)
  {
    delay(1000);
    now = time(nullptr);
  }
  Serial.println("Synchronized");

  // write GMT date and time to cacheDate in HTTP Date format
  // "<day-name>, <day> <month-name> <year> <hour>:<minute>:<second> GMT"
  struct tm timeInfo;
  gmtime_r(&now, &timeInfo);
  sprintf(
      cacheDate,
      "%s, %02d %s %04d %02d:%02d:%02d GMT",
      dayTable[timeInfo.tm_wday - 1],
      timeInfo.tm_mday,
      monthTable[timeInfo.tm_mon],
      timeInfo.tm_year + 1900,
      timeInfo.tm_hour,
      timeInfo.tm_min,
      timeInfo.tm_sec);
  Serial.println(cacheDate);
}

// Setup entry point
void setup()
{
  pinMode(D7, INPUT);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);

  Serial.begin(115200);

  // connect to WiFi
  Serial.println("Connecting");
  // put WIFI_SSID and WIFI_PASS definitions in "credentials.h"
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  Serial.println();
  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  // date and time is used to generate "Date" header a single time at startup
  setupClock();

  // server setup
  server.on("/", handleRoot);
  server.on("/main.css", handleMainStylesheet);
  server.on("/state.css", handleStateStylesheet);
  server.on("/favicon.svg", handleFaviconSvg);
  server.on("/favicon.ico", handleFaviconIco);
  server.on("/t", handleTrigger);
  server.on("/s", handleSshNotification);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
}

/* ---- Main program loop ---- */

// Main program loop
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

/* ---- Web server helper functions ---- */

// Validates the cache of a browser using the ETag
bool validateCache()
{
  // if the header is missing, server.header(name) will return an empty string
  if (server.header("If-None-Match") == CACHE_ETAG)
  {
    server.send(304, "*/*", "");
    return true;
  }
  return false;
}

// Sets the headers required to tell browsers to cache the next response
void enableCaching()
{
  server.sendHeader("Date", cacheDate, false);
  server.sendHeader("ETag", CACHE_ETAG, false);
  server.sendHeader("Cache-Control", CACHE_CONTROL, false);
}

// Sets the Cache-Control header to prevent caching of the next response
void disableCaching()
{
  server.sendHeader("Cache-Control", "no-store");
}

// Checks the authentication for a website request (if credentials were configured)
bool authenticateSite()
{
#if defined(SITE_USER) && defined(SITE_PASS)
  // put SITE_USER and SITE_PASS definitions in "credentials.h"
  if (!server.authenticate(SITE_USER, SITE_PASS))
  {
    server.requestAuthentication(BASIC_AUTH, (const char *)__null, "401 Unauthorized");
    return false;
  }
#endif
  return true;
}

// Checks the authentication for an 'SSH-ready' notification request (if credentials were configured)
bool authenticateSshNotification()
{
#if defined(SSH_USER) && defined(SSH_PASS)
  // put SSH_USER and SSH_PASS definitions in "credentials.h" (not your actual SSH credentials!)
  if (!server.authenticate(SSH_USER, SSH_PASS))
  {
    server.requestAuthentication(BASIC_AUTH, (const char *)__null, "401 Unauthorized");
    return false;
  }
#endif
  return true;
}

/* ---- Web server request handling functions ---- */

// Handles request for "/" (cacheable)
void handleRoot()
{
  if (validateCache())
  {
    return;
  }
  enableCaching();
  server.send(200, "text/html", bodyRoot);
}

// Handles requests for "/main.css" (cacheable)
void handleMainStylesheet()
{
  if (validateCache())
  {
    return;
  }
  enableCaching();
  server.send(200, "text/css", bodyMainCss);
}

// Handles requests for "/state.css" (not cacheable, auth required, carries all server state information)
void handleStateStylesheet()
{
  if (!authenticateSite())
  {
    return;
  }
  disableCaching();

  /*
   State display works by overwriting the "display" property of the elements we want to show or hide.
   Per default (in "/main.css"), elements with class "unknown" are shown while all others are hidden.
   With "/state.css" available, we want to hide all class="unknown" elements and show the ones that
   represent the current server state. Note that when pwr == true, we only hide class="state unknown"
   elements, because the disabled "Power on" button should still be shown.
   */
  if (pwr)
  {
    if (ssh)
    {
      server.send(200, "text/css", ".state.unknown{display:none!important;}.pwron{display:initial;}.sshyes{display:initial;}");
    }
    else
    {
      server.send(200, "text/css", ".state.unknown{display:none!important;}.pwron{display:initial;}.sshno{display:initial;}");
    }
  }
  else
  {
    if (ssh)
    {
      server.send(200, "text/css", ".unknown{display:none!important;}.pwroff{display:initial;}.sshyes{display:initial;}");
    }
    else
    {
      server.send(200, "text/css", ".unknown{display:none!important;}.pwroff{display:initial;}.sshno{display:initial;}");
    }
  }
}

// Handles requests for "/favicon.svg" (cacheable)
void handleFaviconSvg()
{
  if (validateCache())
  {
    return;
  }
  enableCaching();
  server.send(200, "image/svg+xml", bodyFaviconSvg);
}

// Handles requests for "/favicon.ico" (cacheable)
void handleFaviconIco()
{
  if (validateCache())
  {
    return;
  }
  enableCaching();
  // binary data needs to be sent with size explicitly specified
  server.send(200, "image/x-icon", bodyFaviconIco, sizeof(bodyFaviconIco));
}

// Handles requests for "/t" (not cacheable, auth required, triggers power-on)
void handleTrigger()
{
  if (!authenticateSite())
  {
    return;
  }
  disableCaching();
  if (pwr)
  {
    server.send(409, "text/plain", "409 Conflict\n\nServer is already powered on.");
    return;
  }
  powerOn();
  server.sendHeader("Location", "/", false);
  server.send(302, "text/plain", "302 Found\n\nServer powered on successfully.");
}

// Handles requests for "/s" (not cacheable, auth required, marks the SSH service as ready)
void handleSshNotification()
{
  if (!authenticateSshNotification())
  {
    return;
  }
  disableCaching();
  ssh = true;
  server.send(200, "text/plain", "200 OK\n\nSSH service marked as ready.");
}

// Handles requests to unknown URIs by replying with a 404 error
void handleNotFound()
{
  server.send(404, "text/plain", "404 Not Found");
}

/* ---- Actual power-on function ---- */

// Triggers the power button of the connected mainboard
void powerOn()
{
  digitalWrite(D8, HIGH);
  delay(50);
  digitalWrite(D8, LOW);
}
