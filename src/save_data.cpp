#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>

const char* ssid     = "Hehe";
const char* password = "123heheh321";

#define SD_CS_PIN 21
WebServer server(80);

void handleRoot() {
  File root = SD.open("/");
  String html = "<html><head><title>XIAO S3 Storage</title></head><body>";
  html += "<h1>Files on SD Card</h1><hr><ul>";
  
  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    
    // --- HIDE SYSTEM FILES ---
    // Skips files starting with '.' or common system folders
    if (!fileName.startsWith(".") && 
        !fileName.equalsIgnoreCase("/System Volume Information") && 
        !fileName.equalsIgnoreCase("System Volume Information")) {
      
      String displayPath = fileName.startsWith("/") ? fileName.substring(1) : fileName;
      html += "<li><a href='/download?file=" + fileName + "'>" + displayPath + "</a>";
      html += " (" + String(file.size() / 1024) + " KB)</li>";
    }
    
    file = root.openNextFile();
  }
  
  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

void handleDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing filename");
    return;
  }
  
  String path = server.arg("file");
  if (!path.startsWith("/")) path = "/" + path;

  File dataFile = SD.open(path);
  if (dataFile && !dataFile.isDirectory()) {
    // --- FIX FILE NAME ON DOWNLOAD ---
    String downloadName = path;
    if (downloadName.startsWith("/")) downloadName = downloadName.substring(1);

    // This header forces the browser to use the actual filename
    server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
    server.sendHeader("Content-Length", String(dataFile.size()));
    server.streamFile(dataFile, "application/octet-stream");
    dataFile.close();
  } else {
    server.send(404, "text/plain", "File Not Found");
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial); 

  Serial.print("Connecting to SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while(1);
  }
  Serial.println("SD card OK.");

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}