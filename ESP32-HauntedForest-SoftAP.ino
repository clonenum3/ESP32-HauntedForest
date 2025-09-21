#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.
#include <ArduinoJson.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 8, 8);
DNSServer dnsServer;
#define RELAY_PIN D1        // GPIO5 D1
#define SOUNDSENSOR_PIN D4  // GPIO2 D4

int soundCounter[2];
int lastMillis;
bool abortHaunt = false;

const char *cssScript = R"cssScript(
<style>
/* =========================================================
   BODY STYLING: Create the haunted forest background
   ========================================================= */
body {
    /* Set a dark, eerie background. A gradient with a repeating texture image works well. */
    background: #000 url('https://www.transparenttextures.com/patterns/dark-wood.png') repeat;
    background-size: 100%;
    
    /* Add a subtle, dark fog effect using a linear gradient overlay. */
    background-image: linear-gradient(rgba(0,0,0,0.8), rgba(0,0,0,0.8)), url('haunted-forest-background.jpg');
    background-position: center;
    background-repeat: no-repeat;
    background-attachment: fixed;
    background-size: cover;
    
    color: #b1a7a6; /* Eerie, muted text color */
    font-family: 'Creepster', cursive, sans-serif; /* A spooky font, or a fallback if not available */
    margin: 0;
    padding: 2em;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
}

/* Add a web font for a spooky feel */
@import url('https://fonts.googleapis.com/css2?family=Creepster&display=swap');

/* Optional: add a cinematic, foggy overlay with an animation */
body::before {
    content: '';
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: radial-gradient(circle at center, transparent 0%, rgba(0, 0, 0, 0.7) 100%);
    pointer-events: none;
    animation: fog-drift 60s infinite alternate ease-in-out;
}

@keyframes fog-drift {
    from {
        transform: scale(1);
        opacity: 0.8;
    }
    to {
        transform: scale(1.1);
        opacity: 0.9;
    }
}


/* =========================================================
   FORM CONTAINER STYLING: The form area itself
   ========================================================= */
.haunted-form-container {
    background-color: rgba(10, 5, 5, 0.8); /* Semi-transparent dark background for readability */
    border: 2px solid rgba(177, 167, 166, 0.4); /* Subtle, ghostly border */
    box-shadow: 0 0 20px rgba(0, 0, 0, 0.7), inset 0 0 10px rgba(177, 167, 166, 0.2);
    padding: 2em;
    border-radius: 10px;
    backdrop-filter: blur(5px); /* Optional: Adds a slightly blurred background behind the form */
    max-width: 600px;
    width: 100%;
    letter-spacing: 0.2em; /* Wider spacing for a more dramatic look */

}


/* =========================================================
   TABLE STYLING: For the form inputs
   ========================================================= */
.haunted-form-container table {
    width: 100%;
    border-collapse: collapse; /* Merges the borders of the table cells */
    color: inherit;
}

.haunted-form-container th,
.haunted-form-container td {
    padding: 15px;
    text-align: left;
    border-bottom: 1px solid rgba(177, 167, 166, 0.2);
}

.haunted-form-container thead th {
    background-color: rgba(50, 40, 40, 0.5);
    color: #e0e0e0;
    text-transform: uppercase;
    letter-spacing: 2px;
}

/* Zebra-striping for rows to improve readability */
.haunted-form-container tbody tr:nth-child(even) {
    background-color: rgba(20, 15, 15, 0.6);
}

.haunted-form-container tbody tr:nth-child(odd) {
    background-color: rgba(30, 25, 25, 0.6);
}

/* Hover effect for rows */
.haunted-form-container tbody tr:hover {
    background-color: rgba(60, 50, 50, 0.8);
    box-shadow: inset 0 0 10px rgba(255, 255, 255, 0.1);
    cursor: pointer;
}


/* =========================================================
   FORM ELEMENTS STYLING: Inputs, textareas, and buttons
   ========================================================= */
.haunted-form-container input[type="text"],
.haunted-form-container input[type="email"],
.haunted-form-container textarea {
    width: calc(100% - 20px); /* Adjust width for padding */
    padding: 10px;
    background-color: rgba(0, 0, 0, 0.6);
    border: 1px solid rgba(177, 167, 166, 0.4);
    color: #e0e0e0;
    font-family: inherit;
    border-radius: 5px;
    transition: all 0.3s ease;
}

.haunted-form-container input[type="text"]:focus,
.haunted-form-container input[type="email"]:focus,
.haunted-form-container textarea:focus {
    outline: none;
    border-color: #920f0f; /* Sinister red glow on focus */
    box-shadow: 0 0 10px #920f0f, inset 0 0 5px #920f0f;
}

.haunted-form-container input::placeholder,
.haunted-form-container textarea::placeholder {
    color: rgba(177, 167, 166, 0.6);
}

.haunted-form-container button {
    display: block;
    width: 100%;
    padding: 15px;
    background-color: #920f0f; /* Dark red, like blood */
    color: #fff;
    border: none;
    border-radius: 5px;
    font-family: inherit;
    text-transform: uppercase;
    letter-spacing: 2px;
    cursor: pointer;
    transition: all 0.3s ease;
    box-shadow: 0 5px 15px rgba(0, 0, 0, 0.5);
}

.haunted-form-container button:hover {
    background-color: #c71e1e; /* Lighter red on hover */
    box-shadow: 0 8px 20px rgba(0, 0, 0, 0.7);
    transform: translateY(-2px);
}
</style>
)cssScript";

// mark parameters not used in example
#define UNUSED __attribute__((unused))

// TRACE output simplified, can be deactivated here
#define TRACE(...) Serial.printf(__VA_ARGS__)

// name of the server. You reach it using http://webserver
#define HOSTNAME "webserver"

// local time zone definition (Berlin)
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// need a WebServer for http access on port 80.
ESP8266WebServer server(80);

// JSON hauntedSettings Config START
struct hauntedConfigStruct {
  uint32_t listen_ms;          // = 500;
  uint32_t listen_pct;         // = 1;
  uint32_t haunt_time_ms;      // = 3000;
  uint32_t haunt_cooldown_ms;  // = 3000;
};
const char *hauntedSettingsFile = "/hauntedSettings.json";
hauntedConfigStruct hauntedConfig;
DynamicJsonDocument hauntedJSON(1024);
JsonObject hauntedJsonObj;
uint8_t hauntState = 3;
uint32_t msRemaining = 0;

// JSON hauntedSettings Config END

// This function is called when the WebServer was requested without giving a filename.
// This will redirect to the file index.htm when it is existing otherwise to the built-in $upload.htm page
// void handleRedirect() {
//   TRACE("Redirect...");
//   String url = "/hauntedforest";
//
//   // if (!LittleFS.exists(url)) { url = "/$update.htm"; }
//
//   server.sendHeader("Location", url, true);
//   server.send(302);
// }  // handleRedirect()

void handleUpdateHauntedSettings() {
  // TRACE("index.htm ");
  String result;
  result += "<!DOCTYPE html><head>";
  result += cssScript;
  result += R"htmlHeader(
<meta http-equiv="refresh" content="2; URL='/'">
</head>

<html>
<body>
<div class="haunted-form-container">
<h2><center>Updating Haunted Forest Settings</center</h2>
)htmlHeader";

  int numArgs = server.args();

  File file = LittleFS.open(hauntedSettingsFile, "w");
  if (file) {

    for (int i = 0; i < numArgs; i++) {
      result += "key:";
      result += server.argName(i);
      result += " value:";
      result += server.arg(i);

      if (hauntedJsonObj.containsKey(server.argName(i)) == false) {
        result += "Setting does not exist: ";
        result += server.argName(i);
        result += "</br>\n";
      } else {
        hauntedJsonObj[String(server.argName(i))] = String(server.arg(i));
      }

      //if (hauntedSettingsJSON.hasOwnProperty(server.argName(i).c_str())) {
      //  hauntedSettingsJSON[server.argName(i).c_str()] = server.arg(i).toInt();
      //  file.print(JSON.stringify(hauntedSettingsJSON));
      //  result += " Updating Value<br>\n";
      //} else {
      //  result += " No match for value, not updating...<br>\n";
      //}
    }
  } else {
    result += "Error opening settings file<br>\n";
  }

  String hauntedSettingsString = "";
  serializeJson(hauntedJsonObj, hauntedSettingsString);  // input , output

  file = LittleFS.open(hauntedSettingsFile, "w");
  if (file) {
    file.print(hauntedSettingsString);
    Serial.println(hauntedSettingsString);
    file.close();  // close WRITE hauntedSettingsFile
    Serial.println("JSON data written to hauntedSettingsFile");
  } else {
    Serial.println("Failed to write hauntedSettingsFile");
  }
  result += "</div></body></html>";

  server.send(200, "text/html", result);
  // Serial.println(result);
  // delay(20);
  // ESP.reset();
  abortHaunt = true;

}  // handleUpdateHauntedSettings()

void handleHauntedForest() {
  // TRACE("index.htm ");
  String result;
  result += "<!DOCTYPE html><head>";
  //  This makes it difficult to adjust values when the screen is refreshing... need to make it so only the one value changes not a full refresh.
  //  if (hauntState > 0) {
  //    result += "<meta http-equiv=\"refresh\" content=\"5; URL='/'\">";
  //  }
  result += cssScript;
  result += R"htmlHeader(
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Journey into the Haunted Forest</title>
<link href="https://fonts.googleapis.com/css2?family=Creepster&display=swap" rel="stylesheet">
<link rel="stylesheet" href="style.css">
</head>
<html>
<body>
<div class="haunted-form-container">
<center>
<h1><a href="/resetRearm"><font color=green>RESET</font></a></h1>
<h1>Haunted Forest</h1>
<h1><a href="/hauntNow"><font color=red>HAUNT</font></a></h1>
)htmlHeader";

  result += "<form class=\"haunted-form\" action=\"/updateSettings\"><table>";

  result += "<tr><td>State</td><td colspan=2>";
  switch (hauntState) {
    case 3:
      result += "Boot Timer: ";
      result += msRemaining;
      break;
    case 2:
      result += "Haunting: ";
      result += msRemaining;
      break;
    case 1:
      result += "Re-Arming: ";
      result += msRemaining;
      break;
    case 0:
      result += "Armed and Ready";
      result += "</td></tr></br>\n";

      result += "<tr><td>Loud</td><td colspan=2>";
      result += soundCounter[0];
      result += "</td></tr></br>";

      result += "<tr><td>Quiet</td><td colspan=2>";
      result += soundCounter[1];
      break;
  }
  result += "</td></tr></br>\n";

  // listen_ms = Window of time to check for sound
  // listen_pct = Amount of time inside the window needed to trigger the event
  // haunt_time_ms = How long should the event last in ms?
  // haunt_cooldown_ms = How long should we wait after a trigger before we listen again?

  result += "<tr><td><label for=\"listen_ms\">Time to listen (ms):</label></td><td><input type=\"number\" min=\"1\" max=\"4294967295\" id=\"listen_ms\" name=\"listen_ms\" value=\"";
  result += hauntedJsonObj["listen_ms"].as<String>();
  result += "\"><td>Example: 500 = 0.5 Seconds</td><tr>\n";

  result += "<tr><td><label for=\"listen_pct\">Sensitivity:</label></td><td><input type=\"number\" min=\"1\" max=\"4294967295\" id=\"listen_pct\" name=\"listen_pct\" value=\"";
  result += hauntedJsonObj["listen_pct"].as<String>();
  result += "\"><td>Min: 1, Max: 999 (999 = the entire window of time)</td><tr>\n";

  result += "<tr><td><label for=\"haunt_time_ms\">Haunt Time (ms):</label></td><td><input type=\"number\" min=\"1\" max=\"4294967295\" id=\"haunt_time_ms\" name=\"haunt_time_ms\" value=\"";
  result += hauntedJsonObj["haunt_time_ms"].as<String>();
  result += "\"><td>Example: 120000 = 2 Minutes</td><tr>\n";

  result += "<tr><td><label for=\"haunt_cooldown_ms\">Reset Delay (ms):</label></td><td><input type=\"number\" min=\"1\" max=\"4294967295\" id=\"haunt_cooldown_ms\" name=\"haunt_cooldown_ms\" value=\"";
  result += hauntedJsonObj["haunt_cooldown_ms"].as<String>();
  result += "\"><td>Example: 300000 = 5min</td><tr>\n";

  result += "<tr><td><label for=\"power_on_delay_ms\">Power on Delay (ms):</label></td><td><input type=\"number\" min=\"1\" max=\"4294967295\" id=\"power_on_delay_ms\" name=\"power_on_delay_ms\" value=\"";
  result += hauntedJsonObj["power_on_delay_ms"].as<String>();
  result += "\"><td>Example: 60000 = 1min</td><tr>\n";

  result += "<tr><td colspan=3><center><input type=\"submit\" value=\"Submit\"></center></td></tr>";
  result += "</table></form>";

  // result += "<p>If you click the \"Submit\" button, the form-data will be sent to a page called \"/updateSettings\".</p>";
  result += "</center></div></body></html>";

  server.send(200, "text/html", result);
  // Serial.println(result);

}  // handleHauntedForest()

void setup() {
  // WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(3000);
  Serial.println("\n Starting");

  pinMode(SOUNDSENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  WiFi.setHostname(HOSTNAME);

  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP("aabbccddee", "aabbccddee");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  dnsServer.start(DNS_PORT, "*", apIP);

  TRACE("Mounting the filesystem...\n");
  if (!LittleFS.begin()) {
    TRACE("could not mount the filesystem...\n");
    delay(2000);
    ESP.restart();
  }

  soundCounter[0] = 0;
  soundCounter[1] = 0;
  lastMillis = millis() + 10000;

  TRACE("Register service handlers...\n");

  // register a redirect handler when only domain name is given.
  server.on("/", HTTP_GET, handleHauntedForest);
  server.on("/hauntedforest", HTTP_GET, handleHauntedForest);
  server.on("/updateSettings", HTTP_GET, handleUpdateHauntedSettings);
  server.on("/resetRearm", []() {
    digitalWrite(RELAY_PIN, LOW);
    abortHaunt = true;
    server.send(200, "text/html", "<head><meta http-equiv=\"refresh\" content=\"0; URL='/'\"></head><body><h1>Resetting and Re-Arming...</h1></body></html>");
  });
  server.on("/hauntNow", []() {
    soundCounter[0] = 9999;
    soundCounter[1] = 0;
    lastMillis = 0;
    msRemaining = 0;
    hauntState = 0;
    server.send(200, "text/html", "<head><meta http-equiv=\"refresh\" content=\"0; URL='/'\"></head><body><h1>HAUNTING...</h1></body></html>");
  });
  server.onNotFound([]() {
    server.send(404, "text/html", "404 Not Found");
  });

  // enable CORS header in webserver results
  server.enableCORS(true);

  // enable ETAG header in webserver results from serveStatic handler
  server.enableETag(true);

  // serve all static files
  server.serveStatic("/", LittleFS, "/");

  server.begin();
  TRACE("hostname=%s\n", WiFi.getHostname());

  // bool success = LittleFS.remove(hauntedSettingsFile); // DELETE Settings file

  // Load Settings JSON file to String
  String hauntedSettingsString = "";

  File file = LittleFS.open(hauntedSettingsFile, "r");
  if (file) {
    hauntedSettingsString = file.readString();
    Serial.println("READ hauntedSettingsFile");
  } else {
    Serial.println("FAILED to read hauntedSettingsFile");
  }
  file.close();  // close READ hauntedSettingsFile

  deserializeJson(hauntedJSON, hauntedSettingsString);
  hauntedJsonObj = hauntedJSON.as<JsonObject>();

  if (hauntedJsonObj.containsKey(String("listen_ms")) == false) {
    Serial.println("Setting: listen_ms");
    hauntedSettingsString = "{\"listen_ms\":\"500\"}";

    File file = LittleFS.open(hauntedSettingsFile, "w");
    file.print(hauntedSettingsString);
    file.close();

    deserializeJson(hauntedJSON, hauntedSettingsString);
    hauntedJsonObj = hauntedJSON.as<JsonObject>();
  } else {
    Serial.print("Already set: listen_ms = ");
    Serial.println(hauntedJsonObj[String("listen_ms")].as<String>());
  }

  Serial.println("=== Read hauntedSettingsFile === START");
  Serial.println(hauntedSettingsString);
  Serial.println("=== Read hauntedSettingsFile === END");

  uint32_t listen_ms;          // = 500;
  uint32_t listen_pct;         // = 1;
  uint32_t haunt_time_ms;      // = 3000;
  uint32_t haunt_cooldown_ms;  // = 3000;
  uint32_t power_on_delay_ms;  // = 60000;

  if (hauntedJsonObj.containsKey("listen_ms") == false) { hauntedJsonObj[String("listen_ms")] = 500; }
  if (hauntedJsonObj.containsKey("listen_pct") == false) { hauntedJsonObj[String("listen_pct")] = 1; }
  if (hauntedJsonObj.containsKey("haunt_time_ms") == false) { hauntedJsonObj[String("haunt_time_ms")] = 3000; }
  if (hauntedJsonObj.containsKey("haunt_cooldown_ms") == false) { hauntedJsonObj[String("haunt_cooldown_ms")] = 3000; }
  if (hauntedJsonObj.containsKey("power_on_delay_ms") == false) { hauntedJsonObj[String("power_on_delay_ms")] = 60000; }

  serializeJson(hauntedJsonObj, hauntedSettingsString);

  file = LittleFS.open(hauntedSettingsFile, "w");
  if (file) {
    file.print(hauntedSettingsString);
    file.close();  // close WRITE hauntedSettingsFile
    Serial.println("JSON data written to hauntedSettingsFile");
  } else {
    Serial.println("Failed to write hauntedSettingsFile");
  }

  Serial.println("=== hauntedSettingsString === FINAL START");
  Serial.println(hauntedSettingsString);
  Serial.println("=== hauntedSettingsString === FINAL END");

  //Power On Delay
  int s = millis();
  while (millis() < (s + (uint32_t)hauntedJsonObj["power_on_delay_ms"])) {
    dnsServer.processNextRequest();
    server.handleClient();
    msRemaining = (s + (uint32_t)hauntedJsonObj["power_on_delay_ms"]) - millis();
    if (millis() % 1000 == 0) {
      Serial.printf("=== hauntedSettingsString === Delay %d \n", msRemaining);
    }
  }
}

void loop() {

  // 0 == Loud
  // 1 == Quiet
  if (digitalRead(SOUNDSENSOR_PIN) == LOW) {
    // Loud
    soundCounter[0]++;
  } else {
    // Quiet
    soundCounter[1]++;
  }

  if (lastMillis + (uint32_t)hauntedJsonObj["listen_ms"] < millis()) {
    // Serial.println("Should we Haunt?");
    // Some set amount of time has past()
    int total = soundCounter[0] + soundCounter[1];
    if ((1000 * soundCounter[0]) / total > (uint32_t)hauntedJsonObj["listen_pct"]) {
      Serial.println("HAUNTING!!!!");
      digitalWrite(RELAY_PIN, HIGH);
      int s;
      s = millis();
      while (millis() < (s + (uint32_t)hauntedJsonObj["haunt_time_ms"])) {
        dnsServer.processNextRequest();
        server.handleClient();
        hauntState = 2;
        msRemaining = (s + (uint32_t)hauntedJsonObj["haunt_time_ms"]) - millis();
        if (abortHaunt == true) { break; }
      }
      //delay((uint32_t)hauntedJsonObj["haunt_time_ms"]);
      digitalWrite(RELAY_PIN, LOW);
      s = millis();
      while (millis() < (s + (uint32_t)hauntedJsonObj["haunt_cooldown_ms"])) {
        dnsServer.processNextRequest();
        server.handleClient();
        hauntState = 1;
        msRemaining = (s + (uint32_t)hauntedJsonObj["haunt_cooldown_ms"]) - millis();
        if (abortHaunt == true) { break; }
      }
      //delay((uint32_t)hauntedJsonObj["haunt_cooldown_ms"]);
      dnsServer.processNextRequest();
      server.handleClient();
    } else {
      hauntState = 0;
    }
    soundCounter[0] = 0;
    soundCounter[1] = 0;
    lastMillis = millis();
  }

  dnsServer.processNextRequest();
  server.handleClient();
  if (abortHaunt == true) {
    abortHaunt = false;
    soundCounter[0] = 0;
    soundCounter[1] = 0;
    lastMillis = 0;
  }

  if (millis() % 100000 == 0) {
    Serial.println("I'm still here... ");
  }
}
