#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.
#include <ArduinoJson.h>

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
int soundCounter[2];
int lastMillis;

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
void handleRedirect() {
  TRACE("Redirect...");
  String url = "/hauntedforest";

  // if (!LittleFS.exists(url)) { url = "/$update.htm"; }

  server.sendHeader("Location", url, true);
  server.send(302);
}  // handleRedirect()

void handleUpdateSettings() {
  TRACE("index.htm ");
  String result;
  result += "<!DOCTYPE html><head>";
  result += cssScript;
  result += R"htmlHeader(
<meta http-equiv="refresh" content="6; URL='/'">
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
  Serial.println(result);
  delay(20);
  ESP.reset();

}  // handleUpdateSettings()

void handle_root() {
  TRACE("index.htm ");
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
<h1>Haunted Forest</h1>
)htmlHeader";

  result += "<form class=\"haunted-form\" action=\"/updateSettings\"><table>";

  result += "<tr><td>Loud</td><td colspan=2>";
  result += soundCounter[0];
  result += "</td></tr></br>";

  result += "<tr><td>Quiet</td><td colspan=2>";
  result += soundCounter[1];
  result += "</td></tr></br>";

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
    case 0: result += "Armed and Ready"; break;
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
  Serial.println(result);

}  // handle_root()

// This function is called when the WebServer was requested to list all existing files in the filesystem.
// a JSON array with file information is returned.
void handleListFiles() {
  Dir dir = LittleFS.openDir("/");
  String result;

  result += "[\n";
  while (dir.next()) {
    if (result.length() > 4) { result += ","; }
    result += "  {";
    result += " \"name\": \"" + dir.fileName() + "\", ";
    result += " \"size\": " + String(dir.fileSize()) + ", ";
    result += " \"time\": " + String(dir.fileTime());
    result += " }\n";
    // jc.addProperty("size", dir.fileSize());
  }  // while
  result += "]";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleListFiles()

// This function is called when the sysInfo service was requested.
void handleSysInfo() {
  String result;

  FSInfo fs_info;
  LittleFS.info(fs_info);

  result += "{\n";
  result += "  \"flashSize\": " + String(ESP.getFlashChipSize()) + ",\n";
  result += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
  result += "  \"fsTotalBytes\": " + String(fs_info.totalBytes) + ",\n";
  result += "  \"fsUsedBytes\": " + String(fs_info.usedBytes) + ",\n";
  result += "}";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleSysInfo()

// ===== Request Handler class used to answer more complex requests =====

// The FileServerHandler is registered to the web server to support DELETE and UPLOAD of files into the filesystem.
class FileServerHandler : public RequestHandler {
public:
  // @brief Construct a new File Server Handler object
  // @param fs The file system to be used.
  // @param path Path to the root folder in the file system that is used for serving static data down and upload.
  // @param cache_header Cache Header to be used in replies.
  FileServerHandler() {
    TRACE("FileServerHandler is registered\n");
  }


  // @brief check incoming request. Can handle POST for uploads and DELETE.
  // @param requestMethod method of the http request line.
  // @param requestUri request ressource from the http request line.
  // @return true when method can be handled.
  bool canHandle(HTTPMethod requestMethod, const String UNUSED &_uri) override {
    return ((requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE));
  }  // canHandle()


  bool canUpload(const String &uri) override {
    // only allow upload on root fs level.
    return (uri == "/");
  }  // canUpload()


  bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String &requestUri) override {
    // ensure that filename starts with '/'
    String fName = requestUri;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (requestMethod == HTTP_POST) {
      // all done in upload. no other forms.

    } else if (requestMethod == HTTP_DELETE) {
      if (LittleFS.exists(fName)) { LittleFS.remove(fName); }
    }  // if

    server.send(200);  // all done.
    return (true);
  }  // handle()


  // uploading process
  void upload(ESP8266WebServer UNUSED &server, const String UNUSED &_requestUri, HTTPUpload &upload) override {
    // ensure that filename starts with '/'
    String fName = upload.filename;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (upload.status == UPLOAD_FILE_START) {
      // Open the file
      if (LittleFS.exists(fName)) { LittleFS.remove(fName); }  // if
      _fsUploadFile = LittleFS.open(fName, "w");

    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // Write received bytes
      if (_fsUploadFile) { _fsUploadFile.write(upload.buf, upload.currentSize); }

    } else if (upload.status == UPLOAD_FILE_END) {
      // Close the file
      if (_fsUploadFile) { _fsUploadFile.close(); }
    }  // if
  }    // upload()

protected:
  File _fsUploadFile;
};

#define TRIGGER_PIN 0
#define RELAY_PIN D1        // GPIO5 D1
#define SOUNDSENSOR_PIN D4  // GPIO2 D4

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false;  // change to true to use non blocking

WiFiManager wm;                     // global wm instance
WiFiManagerParameter custom_field;  // global param ( for non blocking w params )

void setup() {
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(3000);
  Serial.println("\n Starting");

  pinMode(TRIGGER_PIN, INPUT);
  pinMode(SOUNDSENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // wm.resetSettings(); // wipe settings

  if (wm_nonblocking) wm.setConfigPortalBlocking(false);

  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

  // test custom html input type(checkbox)
  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

  // test custom html(radio)
  const char *custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  new (&custom_field) WiFiManagerParameter(custom_radio_str);  // custom html input

  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  // custom menu via array or vector
  //
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = { "wifi", "info", "param", "sep", "restart", "exit" };
  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");


  //set static ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
  // wm.setShowStaticFields(true); // force show static ip fields
  // wm.setShowDnsFields(true);    // force show dns field always

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(30);  // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  // allow to address the device by the given name e.g. http://webserver
  WiFi.setHostname(HOSTNAME);

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("aabbccddee", "aabbccddee");  // password protected ap

  if (!res) {
    Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }


  TRACE("Starting WebServer example...\n");

  TRACE("Mounting the filesystem...\n");
  if (!LittleFS.begin()) {
    TRACE("could not mount the filesystem...\n");
    delay(2000);
    ESP.restart();
  }

  soundCounter[0] = 0;
  soundCounter[1] = 0;
  int lastMillis = millis() + 10000;

  // Ask for the current time using NTP request builtin into ESP firmware.
  TRACE("Setup ntp...\n");
  configTime(TIMEZONE, "pool.ntp.org");

  TRACE("Register service handlers...\n");

  // register a redirect handler when only domain name is given.
  // server.on("/", HTTP_GET, handle_root);
  server.on("/hauntedforest", HTTP_GET, handle_root);
  server.on("/updateSettings", HTTP_GET, handleUpdateSettings);

  server.on("/", HTTP_GET, handleRedirect);

  // register some REST services
  server.on("/$list", HTTP_GET, handleListFiles);
  server.on("/$sysinfo", HTTP_GET, handleSysInfo);

  // UPLOAD and DELETE of files in the file system using a request handler.
  server.addHandler(new FileServerHandler());

  // enable CORS header in webserver results
  server.enableCORS(true);

  // enable ETAG header in webserver results from serveStatic handler
  server.enableETag(true);

  // serve all static files
  server.serveStatic("/", LittleFS, "/");

  // handle cases when file is not found
  server.onNotFound([]() {
    // standard not found in browser.
    server.send(404, "text/html", "404 Not Found");
  });

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
    server.handleClient();
    msRemaining = (s + (uint32_t)hauntedJsonObj["power_on_delay_ms"]) - millis();
  }
}

void checkButton() {
  // check for button press
  if (digitalRead(TRIGGER_PIN) == LOW) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000);  // reset delay hold
      if (digitalRead(TRIGGER_PIN) == LOW) {
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }

      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);

      if (!wm.startConfigPortal("OnDemandAP", "password")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}

String getParam(String name) {
  //read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}

void loop() {
  if (wm_nonblocking) wm.process();  // avoid delays() in loop when non-blocking and other long running code

  // 0 == Loud
  // 1 == Quiet
  pinMode(RELAY_PIN, OUTPUT);
  if (digitalRead(SOUNDSENSOR_PIN) == LOW) {
    // Loud
    soundCounter[0]++;
  } else {
    // Quiet
    soundCounter[1]++;
  }

  if (lastMillis + (uint32_t)hauntedJsonObj["listen_ms"] < millis()) {
    // Some set amount of time has past()
    int total = soundCounter[0] + soundCounter[1];
    if ((1000 * soundCounter[0]) / total > (uint32_t)hauntedJsonObj["listen_pct"]) {
      digitalWrite(RELAY_PIN, HIGH);
      int s;
      s = millis();
      while (millis() < (s + (uint32_t)hauntedJsonObj["haunt_time_ms"])) {
        server.handleClient();
        hauntState = 2;
        msRemaining = (s + (uint32_t)hauntedJsonObj["haunt_time_ms"]) - millis();
      }
      //delay((uint32_t)hauntedJsonObj["haunt_time_ms"]);
      digitalWrite(RELAY_PIN, LOW);
      s = millis();
      while (millis() < (s + (uint32_t)hauntedJsonObj["haunt_cooldown_ms"])) {
        server.handleClient();
        hauntState = 1;
        msRemaining = (s + (uint32_t)hauntedJsonObj["haunt_cooldown_ms"]) - millis();
      }
      //delay((uint32_t)hauntedJsonObj["haunt_cooldown_ms"]);
    } else {
      hauntState = 0;
    }
    soundCounter[0] = 0;
    soundCounter[1] = 0;
    lastMillis = millis();
  }

  checkButton();
  server.handleClient();
}
