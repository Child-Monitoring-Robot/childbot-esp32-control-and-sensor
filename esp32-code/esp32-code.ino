#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "childbotnet";
const char* password = "robotnet";

// MPU6050 Accelerometer
Adafruit_MPU6050 mpu6050;
sensors_event_t accelerometer, gyroscope, unusedTemp;

// MQ2 Gas Sensor
const int MQ2_D0 = 16;
const int MQ2_A0 = 35;
int gasState = 0, gasValue = 0;

// DHT22 Temperature Humidity Sensor
#define DHTTYPE DHT22
const int DHT22_DATA_PIN = 4;
DHT dht22(DHT22_DATA_PIN, DHTTYPE);
int temperature, humidity;

// Motor control pins
const int RIGHT_FRONT_ONE = 25;
const int RIGHT_FRONT_TWO = 33;

const int RIGHT_BACK_ONE = 12;
const int RIGHT_BACK_TWO = 14;

const int LEFT_FRONT_ONE = 18;
const int LEFT_FRONT_TWO = 19;

const int LEFT_BACK_ONE = 15;
const int LEFT_BACK_TWO = 2;

// buzzer
const int BUZZER_PIN = 26;

bool states[8] = {false, false, false, false, false, false, false, false};

WiFiServer server(80); // Start a server on port 80

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Try to initialize mpu6050
  if (!mpu6050.begin()) {
    Serial.println("Failed to find MPU6050 chip");
  }
  Serial.println("MPU6050 Found!");

  mpu6050.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu6050.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu6050.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // mq2
  pinMode(MQ2_D0, INPUT);

  //dht22
  dht22.begin();
  pinMode(DHT22_DATA_PIN, INPUT_PULLUP);

  //buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // motor control pins
  pinMode(RIGHT_FRONT_ONE, OUTPUT);
  pinMode(RIGHT_FRONT_TWO, OUTPUT);
  pinMode(RIGHT_BACK_ONE, OUTPUT);
  pinMode(RIGHT_BACK_TWO, OUTPUT);
  pinMode(LEFT_FRONT_ONE, OUTPUT);
  pinMode(LEFT_FRONT_TWO, OUTPUT);
  pinMode(LEFT_BACK_ONE, OUTPUT);
  pinMode(LEFT_BACK_TWO, OUTPUT);

  initMotorStates();

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Print the ESP32 IP address
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
}

void loop() {
  // mpu6050 sensor data
  mpu6050.getEvent(&accelerometer, &gyroscope, &unusedTemp);

  // MQ2 gas sensor data
  gasState = digitalRead(MQ2_D0);
  gasValue = analogRead(MQ2_A0);

  // dht22 sensor
  temperature = dht22.readTemperature(); // read as celcius
  humidity = dht22.readHumidity();

  // buzzer
  digitalWrite(BUZZER_PIN, LOW);

  // Check for incoming client requests
  WiFiClient client = server.accept();
  if (!client) {
    return;
  }

  // Wait until the client sends data
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the HTTP request
  String request = client.readStringUntil('\r');
  client.clear();

  if (request.indexOf("/front") != -1) {
    moveFront();
  }

  if (request.indexOf("/back") != -1) {
    moveBack();
  }

  if (request.indexOf("/turnLeft") != -1) {
    turnLeft();
  }

  if (request.indexOf("/turnRight") != -1) {
    turnRight();
  }

  if (request.indexOf("/stop") != -1) {
    initMotorStates();
  }

  if (request.indexOf("/toggleBuzzer") != -1) {
    if(digitalRead(BUZZER_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, LOW);
    }
    else {
      digitalWrite(BUZZER_PIN, HIGH);
    }
  }

  // HTML content for the webpage
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 L298n Motor Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; margin: 0; padding: 20px; }";
  html += "h1 { color: #333; }";
  html += ".grid-container { display: grid; grid-template-columns: repeat(2, 200px); gap: 20px; margin-top: 30px; justify-content: center; }";
  html += ".button { font-size: 18px; padding: 15px 30px; background-color: #4CAF50; border: none; color: white; border-radius: 8px; cursor: pointer; width: 100%; box-shadow: 0 4px 6px rgba(0,0,0,0.1); transition: background-color 0.3s, box-shadow 0.3s; }";
  html += ".button:hover { background-color: #45a049; box-shadow: 0 6px 12px rgba(0,0,0,0.2); }";
  html += ".button:active { background-color: #3e8e41; }";
  html += ".button-container { display: flex; flex-direction: column; justify-content: center; align-items: center; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>ESP32 L298n Motor Control</h1>";

  html += "<div class='grid-container'>";
  // Buttons with states
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/front'\">Move Forward</button></div>";
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/back'\">Move Backward</button></div>";
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/turnLeft'\">Turn Left</button></div>";
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/turnRight'\">Turn Right</button></div>";
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/stop'\">Stop</button></div>";
  html += "<div class='button-container'><button class='button' onclick=\"location.href='/toggleBuzzer'\">Toggle Buzzer</button></div>";
  html += "</div>";

  html += "<div class='grid-container'>";
  html += "<div><h3>MQ2 Data</h3>";
  html += "<p>Gas State = " + String(gasState) + "<br>";
  html += "Gas Value = " + String(gasValue) + "</p></div>";
  html += "</div>";

  html += "<div class='grid-container'>";
  html += "<div><h3>DHT22 Data</h3>";
  html += "<p>Temperature = " + String(temperature) + " degree C<br>";
  html += "Humidity = " + String(humidity) + "%</p></div>";
  html += "</div>";

  html += "<div class='grid-container'>";
  html += "<div><h3>MPU-6050 Data</h3>";
  html += "<p>Acceleration X = " + String(accelerometer.acceleration.x) + " m/s^2 <br>";
  html += "Acceleration Y = " + String(accelerometer.acceleration.y) + " m/s^2<br>";
  html += "Acceleration Z = " + String(accelerometer.acceleration.z) + " m/s^2</p></div>";
  html += "</div>";


  html += "</body></html>";

  // Send the response to the client
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
  client.println();

  // Close the connection
  client.stop();
}


// motor control functions
void initMotorStates() {
  digitalWrite(RIGHT_FRONT_ONE, LOW);
  digitalWrite(RIGHT_FRONT_TWO, LOW);
  digitalWrite(RIGHT_BACK_ONE, LOW);
  digitalWrite(RIGHT_BACK_TWO, LOW);
  digitalWrite(LEFT_FRONT_ONE, LOW);
  digitalWrite(LEFT_FRONT_TWO, LOW);
  digitalWrite(LEFT_BACK_ONE, LOW);
  digitalWrite(LEFT_BACK_TWO, LOW);
}

void moveFront() {
  digitalWrite(RIGHT_FRONT_ONE, HIGH);
  digitalWrite(RIGHT_FRONT_TWO, LOW);

  digitalWrite(RIGHT_BACK_ONE, HIGH);
  digitalWrite(RIGHT_BACK_TWO, LOW);

  digitalWrite(LEFT_FRONT_ONE, HIGH);
  digitalWrite(LEFT_FRONT_TWO, LOW);

  digitalWrite(LEFT_BACK_ONE, HIGH);
  digitalWrite(LEFT_BACK_TWO, LOW);
}

void moveBack() {
  digitalWrite(RIGHT_FRONT_ONE, LOW);
  digitalWrite(RIGHT_FRONT_TWO, HIGH);

  digitalWrite(RIGHT_BACK_ONE, LOW);
  digitalWrite(RIGHT_BACK_TWO, HIGH);

  digitalWrite(LEFT_FRONT_ONE, LOW);
  digitalWrite(LEFT_FRONT_TWO, HIGH);
  
  digitalWrite(LEFT_BACK_ONE, LOW);
  digitalWrite(LEFT_BACK_TWO, HIGH);
}

void turnLeft() {
  // Left motors move backward
  digitalWrite(LEFT_FRONT_ONE, LOW);
  digitalWrite(LEFT_FRONT_TWO, HIGH);
  digitalWrite(LEFT_BACK_ONE, LOW);
  digitalWrite(LEFT_BACK_TWO, HIGH);
  
  // Right motors move forward
  digitalWrite(RIGHT_FRONT_ONE, HIGH);
  digitalWrite(RIGHT_FRONT_TWO, LOW);
  digitalWrite(RIGHT_BACK_ONE, HIGH);
  digitalWrite(RIGHT_BACK_TWO, LOW);
}

void turnRight() {
  // Left motors move forward
  digitalWrite(LEFT_FRONT_ONE, HIGH);
  digitalWrite(LEFT_FRONT_TWO, LOW);
  digitalWrite(LEFT_BACK_ONE, HIGH);
  digitalWrite(LEFT_BACK_TWO, LOW);
  
  // Right motors move backward
  digitalWrite(RIGHT_FRONT_ONE, LOW);
  digitalWrite(RIGHT_FRONT_TWO, HIGH);
  digitalWrite(RIGHT_BACK_ONE, LOW);
  digitalWrite(RIGHT_BACK_TWO, HIGH);
}