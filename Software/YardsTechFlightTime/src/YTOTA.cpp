#include <WiFi.h>
#include <Update.h> //https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/AWS_S3_OTA_Update/AWS_S3_OTA_Update.ino
#include "YTGlobals.h"


WiFiClient UpdClient;

// Variables to validate
// response from S3
long contentLength = 0;
bool isValidContentType = false;
String OTAHost = "";
int OTAPort = 20001;
String OTAPath = "";



// S3 Bucket Config
//String host = "bucket-name.s3.ap-south-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
//int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
//String bin = "/sketch-name.ino.bin"; // bin file name with a slash in front.


// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}


// OTA Logic 
void ExecuteOTA (String Host, int Port, String Path) {
  OTAUpdating = true;

  UpdClient.setTimeout (10);
  
  Serial.println("Connecting to: " + String(Host));
  // Connect to host
  if (UpdClient.connect(Host.c_str(), Port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching path: " + String(Path));

    // Get the contents of the bin file
    UpdClient.print(String("GET ") + Path + " HTTP/1.1\r\n" +
                 "Host: " + Host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (UpdClient.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        UpdClient.stop();
        OTAUpdating = false;
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}
    */
    while (UpdClient.available()) {
      // read line till /n
      String line = UpdClient.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();
      String lcline = line;
      lcline.toLowerCase();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (lcline.startsWith("http/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          OTAUpdating = false;
          return;
        }
      }

      // extract headers here
      // Start with content length
      if (lcline.startsWith("content-length: ")) {
        contentLength = atol((getHeaderValue(lcline, "content-length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (lcline.startsWith("content-type: ")) {
        String contentType = getHeaderValue(lcline, "content-type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to host failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(Host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength, U_FLASH, ACTIVITY_PIN, HIGH);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(UpdClient);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          RestartModule(true);
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      UpdClient.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    UpdClient.flush();
  }
  
  OTAUpdating = false;
}
