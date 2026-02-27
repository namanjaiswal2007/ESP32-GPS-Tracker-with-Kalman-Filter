#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include<math.h>


const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

String serverName = "http://eu.thingsboard.cloud/api/v1/YOUR_ACCESS_TOKEN/telemetry";

String nmeaSentence = "";
float latitude = 0;
float longitude = 0;
float cleanLat = 0;
float cleanLon = 0;
float err_estimate_lat = 1.0;
float err_estimate_lon = 1.0; 
float err_measure = 3.0; 
float q = 0.00005;
float lastSendLat=0.0,lastSendLon=0.0;

unsigned long lastTime;

void setup() {
  Serial.begin(115200); 
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  delay(500);

  /*Serial2.println("$PMTK251,115200*1F");
  delay(100);

  Serial2.end();
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  delay(500);
  
  Serial2.println("$PMTK220,100*2F");
  Serial.println("GPS Initialized at 115200 Baud, 10Hz Frequency");

  */
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  digitalWrite(2,HIGH);
}

void loop() {

  if(WiFi.status() != WL_CONNECTED){ // Try to connect back to wifi if disconnect after booting
    WiFi.begin(ssid, password);
  }

  while (Serial2.available()) { // loop runs this every character is accessed
    char c = Serial2.read();

    if (c == '\n') {   // stops with line ends
      if (nmeaSentence.startsWith("$GNRMC")) { // checks where the line is one we needed
        parseGNRMC(nmeaSentence); // send sentence to function parseGNRMC
      }
      nmeaSentence = ""; // reset the sentence
    } 
    else {
      nmeaSentence += c; // add the character to line
    }
  }
  
}

void parseGNRMC(String sentence) {

  String data[15]; // date will be of 15 character preknown knowledge
  int index = 0;

  for (int i = 0; i < sentence.length(); i++) { // seperate data into diffenent indexes
    if (sentence[i] == ',') {
      index++;
    } else {
      data[index] += sentence[i];
    }
  }

  if (data[2] == "A") { // data[2] is either A or V A means data is acceptable and V means its corrupted

    // data 1 is time converting it inot readable form it will come like 123412
    String rawTime = data[1];
    String timeStr = rawTime.substring(0,2) + ":" + rawTime.substring(2,4) + ":" + rawTime.substring(4,6);


    // extrating latitude
    float rawLat = data[3].toFloat(); // converting the data into float
    int latDegree = int(rawLat / 100); // extrating intial part of latitude
    float latMinute = rawLat - (latDegree * 100); // converting minute into decimal
    latitude = latDegree + (latMinute / 60.0); // joint and making the latitude

    if (data[4] == "S") latitude = -latitude; // S mean the directino is opposite so adding negative sign

    // same as latitude
    float rawLon = data[5].toFloat();
    int lonDegree = int(rawLon / 100);
    float lonMinute = rawLon - (lonDegree * 100);
    longitude = lonDegree + (lonMinute / 60.0);
    if (data[6] == "W") longitude = -longitude;

    if (abs(latitude) < 0.0001 || abs(longitude) < 0.0001) { // if data is very close to 0,0 reject it
      return;
    }

    if (lastTime == 0) { // to prevent sending 0,0 when cleanLat and cleanLon is 0,0
      cleanLat = latitude;
      cleanLon = longitude;
      lastSendLat = cleanLat;
      lastSendLon = cleanLon;
      lastTime = millis();
    }


    float rawDist = distanceMeters(cleanLat, cleanLon, latitude, longitude);// the distance from the new data of gps and old known data

    if (rawDist > 50.0) { // if the new coordinates are drastically far reject it
      return;
    }

    if (rawDist < 2.0) { // if distance is very small most probably noise reject it
      return;   // freeze position
    }

    // Decide filtering strength
    bool isMoving = rawDist > 4.0; // cnecks if person is moving or not

    
    if(isMoving){ // No Kalman when moving L89 is pretty much accurate
      cleanLat = latitude;
      cleanLon = longitude;
      err_estimate_lat = 1.0;
      err_estimate_lon = 1.0;

    }
    else{ // Kalman at stationary
      float gain_lat = err_estimate_lat / (err_estimate_lat + err_measure);
      cleanLat = cleanLat + gain_lat * (latitude - cleanLat);
      err_estimate_lat = (1.0 - gain_lat) * err_estimate_lat + q;

      // Longitude
      float gain_lon = err_estimate_lon / (err_estimate_lon + err_measure);
      cleanLon = cleanLon + gain_lon * (longitude - cleanLon);
      err_estimate_lon = (1.0 - gain_lon) * err_estimate_lon + q;
    }

    Serial.print("CLEAN_LAT:"); Serial.print(cleanLat, 6);
    Serial.print(" | CLEAN_LON:"); Serial.print(cleanLon, 6);
    Serial.print(" | TIME:"); Serial.println(timeStr);
  }

  float dist = distanceMeters(lastSendLat, lastSendLon, cleanLat, cleanLon); // Finding Distance between last POint send and current point
  unsigned long currentTime = millis(); // to check the current Time

  
    // send data only when difference between current time and last send time is greater than 1 second wife is coonnected
    // distance is greater than 3 m and send data at interval of 30 second when stationary 
    if (WiFi.status() == WL_CONNECTED && cleanLat != 0.0 && cleanLon != 0.0 && ((currentTime - lastTime >= 1000 && dist > 3.0) || currentTime - lastTime >= 30000) ) {
      
      HTTPClient http;

      String jsonData = "{\"lat\":" + String(cleanLat,6) + ",\"lon\":" + String(cleanLon,6) + "}";
      
      
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);
      Serial.print("HTTP Response Code1: ");
      Serial.println(httpResponseCode);
      http.end();

      lastSendLat = cleanLat;
      lastSendLon = cleanLon;
      lastTime = millis();
    }
  
}
float distanceMeters(float lat1, float lon1, float lat2, float lon2) { // funtion to find distance betwwen two coordinates 

  const float R = 6371000;
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);

  float a = sin(dLat/2) * sin(dLat/2) +
            cos(radians(lat1)) * cos(radians(lat2)) *
            sin(dLon/2) * sin(dLon/2);

  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}
