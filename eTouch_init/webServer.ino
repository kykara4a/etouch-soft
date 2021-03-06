String responseHTML = ""
                      "<!DOCTYPE html><html><head><title>Braille Display</title></head><body>"
                      "<center><h1>Braille Display</h1><br>"
                      "<form name='form' action='code' method='post'>"
                      "<p><b>Text:</b><br><input type='text' name='text' size='40'></p>"
                      "<p><input type='submit' value='Translate'></p></form>"
                      "<form action='upload' method='post' enctype='multipart/form-data'>"
                      "Select image to upload:<br>"
                      "<input type='file' name='name' id='fileToUpload'><br>"
                      "<input type='submit' value='Upload File' name='submit'></form>"
                      "</center></body></html>";

void webServerSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Braille Display");
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  SPIFFS.begin();                                          // Start the SPI Flash Files System

  webServer.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                   // send it if it exists
      webServer.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });
  webServer.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ webServer.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                       // Receive and save the file
  );
  webServer.onNotFound([]() {
    handleRoot();
    if (!handleFileRead(webServer.uri()))                  // send it if it exists
      webServer.send(404, "text/plain", "404: Not Found");
  });

  webServer.on("/code", codeTranslate);
  webServer.begin();
}

void handleRoot() {
  webServer.send(200, "text/html", responseHTML);
}

void codeTranslate() {
  text = webServer.arg("text");
  text.toUpperCase();
  
  executeLine(0);
  handleRoot();
}

/* Below: to be used for file handling. */
String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".txt")) return "text/plain";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = webServer.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = webServer.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      webServer.sendHeader("Location","/success.html");      // Redirect the client to the success page
      webServer.send(303);
    } else {
      webServer.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

