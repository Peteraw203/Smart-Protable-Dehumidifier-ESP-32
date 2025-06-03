#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// WiFi
#define WIFI_SSID "//Your Wifi name/SSID here"
#define WIFI_PASSWORD "Your password wifi"

// Firebase
#define FIREBASE_API_KEY "//Your firebase api key"
#define FIREBASE_URL "Firebase real time database url"
//Create a user in your Firebase dashboard
#define USER_EMAIL "Firebase user email"
#define USER_PASSWORD "Firebase user password"

// Firebase init
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pin
#define RELAY_PIN 27 
#define TOUCH_PIN 4
#define TOUCH_THRESHOLD 40
#define WATER_SENSOR_PIN 32
#define DHT_PIN 16
#define DHT_TYPE DHT22
#define POWER_WATER_LEVEL 12

DHT dht(DHT_PIN, DHT_TYPE);

// Global Variabel 
bool relayState = false;
int lastTouchState = 0;
bool otomatis = false;
int water_level_threshold = 1160; 
float humidity = 0, temperature = 0;
int waterLevel = 0, normalize_water_level = 0;
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 100;

// RTOS Tasks
TaskHandle_t TaskSensor, TaskTouch, TaskControl, TaskFirebase;

void connectWiFiAndFirebase() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("\nWiFi Connected!");
  WiFi.setSleep(true);

  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  while (!Firebase.ready()) {
    Serial.println("Wait for Firebase...");
    delay(500);
  }
  Serial.println("Firebase Connected!");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(POWER_WATER_LEVEL, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(POWER_WATER_LEVEL, HIGH);
  dht.begin();

  connectWiFiAndFirebase();

  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 6144, NULL, 1, &TaskSensor, 1);
  xTaskCreatePinnedToCore(touchTask, "TouchTask", 8192, NULL, 2, &TaskTouch, 1);
  xTaskCreatePinnedToCore(controlTask, "ControlTask", 8192, NULL, 1, &TaskControl, 1);
  xTaskCreatePinnedToCore(firebaseTask, "FirebaseTask", 8192, NULL, 2, &TaskFirebase, 1);
}

void loop() {
  
}

void sensorTask(void *pvParameters) {
  while (1) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    waterLevel = analogRead(WATER_SENSOR_PIN);
    
    //Change the global variable "water_level_threshold" depends on your water level sesnsor sensitivity 
    normalize_water_level = (waterLevel / water_level_threshold) * 100;

    if (!isnan(humidity) && !isnan(temperature)) {
      Serial.printf("Sensor -> H: %.2f%%, T: %.2f°C, WL: %d (%d%%)\n", humidity, temperature, waterLevel, normalize_water_level);
    } else {
      Serial.println("DHT nya error");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void touchTask(void *pvParameters) {
  while (1) {
    int touchValue = touchRead(TOUCH_PIN);
    bool isTouched = (touchValue < TOUCH_THRESHOLD);

    if (isTouched && (millis() - lastTouchTime > debounceDelay)) {
      lastTouchTime = millis();
      lastTouchState = (lastTouchState + 1) % 3;
      otomatis = (lastTouchState == 2);

      if (lastTouchState == 0) {
        relayState = false;
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Mode: OFF, Relay OFF");
      } else if (lastTouchState == 1) {
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
        Serial.println(relayState ? "Mode: Manual, Relay ON" : "Mode: Manual, Relay OFF");
      } else if (lastTouchState == 2) {
        Serial.println("Mode: Otomatis Aktif");
      }

      Firebase.RTDB.setInt(&fbdo, "/mode", lastTouchState);
    }

    vTaskDelay(700 / portTICK_PERIOD_MS);
  }
}

void controlTask(void *pvParameters) {
  while (1) {
    // Baca mode dari Firebase
    if (Firebase.RTDB.getInt(&fbdo, "/mode")) {
      int modeValue = fbdo.intData();
      if (modeValue >= 0 && modeValue <= 2) {
        lastTouchState = modeValue;
        otomatis = (lastTouchState == 2);

        if (lastTouchState == 0) {
          relayState = false;
          digitalWrite(RELAY_PIN, LOW);
        } else if (lastTouchState == 1) {
          relayState = true;
          digitalWrite(RELAY_PIN, HIGH);
        }
      }
    }

    if (normalize_water_level > 90) {
      relayState = false;
      digitalWrite(RELAY_PIN, LOW);
      Firebase.RTDB.setInt(&fbdo, "/mode", 0);
      Serial.println("Water level tinggi, Relay OFF");
    }

    if (otomatis) {
      if (humidity > 60.0) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("Auto Mode: Humidity tinggi, relay ON");
      } else {
        relayState = false;
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Auto Mode: Humidity rendah, relay OFF");
      } 
    }

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}

void firebaseTask(void *pvParameters) {
  while (1) {
    if (Firebase.ready()) {
      Firebase.RTDB.setDouble(&fbdo, "/humidity", humidity);
      Firebase.RTDB.setDouble(&fbdo, "/temperature", temperature);
      Firebase.RTDB.setInt(&fbdo, "/waterLevel", normalize_water_level);
    } else {
      Serial.println("Firebase not connected");
    }
    vTaskDelay(3500 / portTICK_PERIOD_MS); 
  }
}
