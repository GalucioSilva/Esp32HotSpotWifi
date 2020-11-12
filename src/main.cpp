#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <pthread.h>
#include <string>

#define BUTTON1 32
#define BUTTON2 33

const char *ssid = "GEORGE D PAULA";
const char *password = "91138576";

int POT1 = 34;
int POT2 = 35;

String sensor1 = "";
String sensor2 = "";

pthread_t COUNT1;
pthread_t COUNT2;
pthread_mutex_t lock;
pthread_mutex_t lock2;

int count1 = 0;
int count2 = 0;

bool flagButton = true;
bool flagButton2 = true;

AsyncWebServer server(80);

String readSensor1()
{
  int value = analogRead(POT1);
  //Serial.print("POT1: ");
  //Serial.println(value);
  return String(value);
}

String readSensor2()
{
  int value = analogRead(POT2);
  //Serial.print("POT2: ");
  //Serial.println(value);
  return String(value);
}

String convertCount1()
{
  return String(count1);
}

String convertCount2()
{
  return String(count2);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link
      rel="stylesheet"
      href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
      integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr"
      crossorigin="anonymous"
    />
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      h2 {
        font-size: 3rem;
      }
      p {
        font-size: 3rem;
      }
      .units {
        font-size: 1.2rem;
      }
      .dht-labels {
        font-size: 1.5rem;
        vertical-align: middle;
        padding-bottom: 15px;
      }
    </style>
  </head>
  <body>
    <h2>ESP32 TEST MULTI TASK READ</h2>
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
      <span class="dht-labels">Sensor1</span>
      <span id="sensor1">%SENSOR1%</span>
      <sup class="units">&deg;C</sup>
    </p>
    <p>
      <i class="fas fa-tint" style="color:#00add6;"></i>
      <span class="dht-labels">Sensor2</span>
      <span id="sensor2">%SENSOR2%</span>
      <sup class="units">%</sup>
    </p>
    <p>
      <i class="fas fa-snowflake" style="color:#9e0505;"></i>
      <span class="dht-labels">Contador1</span>
      <span id="count1">VAI1</span>
    </p>
    <p>
      <i class="fas fa-flask" style="color:#66059e;"></i>
      <span class="dht-labels">Contador2</span>
      <span id="sensor1">VAI2</span>
    </p>
  </body>
  <script>
    setInterval(function() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("sensor1").innerHTML = this.responseText;
        }
      };
      xhttp.open("GET", "/sensor1", true);
      xhttp.send();
    }, 100);

    setInterval(function() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("sensor2").innerHTML = this.responseText;
        }
      };
      xhttp.open("GET", "/sensor2", true);
      xhttp.send();
    }, 100);

    setInterval(function() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("count1").innerHTML = this.responseText;
        }
      };
      xhttp.open("GET", "/count1", true);
      xhttp.send();
    }, 100);
  </script>
</html>
)rawliteral";

String processor(const String &var)
{
  //Serial.println(var);
  if (var == "SENSOR1")
  {
    return readSensor1();
  }
  else if (var == "SENSOR2")
  {
    return readSensor2();
  }
  else if (var == "COUNT1#")
  {
    return convertCount1();
  }
  else if (var == "COUNT2#")
  {
    return convertCount2();
  }
  return String();
}

void TaskRead1(void *parameter)
{
  while (1)
  {
    sensor1 = readSensor1();
    delay(100);
  }
}

void *Count2(void *args)
{
  delay(1000);
  flagButton2 = true;
  for (int i = 0; i < 5; i++)
  {
    pthread_mutex_lock(&lock2);
    count2++;
    Serial.print("Count2=");
    Serial.println(count2);
    pthread_mutex_unlock(&lock2);
    delay(1000);
  }
}

void TaskRead2(void *parameter)
{
  while (1)
  {
    int button_state = digitalRead(BUTTON2);
    if (button_state && flagButton2)
    {
      flagButton2 = false;
      pthread_create(&COUNT2, NULL, Count2, NULL);
    }
    sensor2 = readSensor2();
    delay(100);
  }
}

void *Count1(void *args)
{
  delay(1000);
  flagButton = true;
  for (int i = 0; i < 5; i++)
  {
    pthread_mutex_lock(&lock);
    count1++;
    Serial.print("Count1=");
    Serial.println(count1);
    pthread_mutex_unlock(&lock);
    delay(1000);
  }
}

void TaskRead3(void *parameter)
{
  server.begin();
  while (1)
  {
    int button_state = digitalRead(BUTTON1);
    if (button_state && flagButton)
    {
      flagButton = false;
      pthread_create(&COUNT1, NULL, Count1, NULL);
    }
  };
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/sensor1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", sensor1.c_str());
  });
  server.on("/sensor2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", sensor2.c_str());
  });
  server.on("/count1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", convertCount1().c_str());
  });

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  xTaskCreatePinnedToCore(
      TaskRead1, // Function that should be called
      "core1",   // Name of the task (for debugging)
      10000,     // Stack size (bytes)
      NULL,      // Parameter to pass
      1,         // Task priority
      NULL,      // Task handle
      0          // Core you want to run the task on (0 or 1)
  );

  xTaskCreatePinnedToCore(
      TaskRead2, // Function that should be called
      "core2",   // Name of the task (for debugging)
      10000,     // Stack size (bytes)
      NULL,      // Parameter to pass
      1,         // Task priority
      NULL,      // Task handle
      0          // Core you want to run the task on (0 or 1)
  );

  xTaskCreatePinnedToCore(
      TaskRead3, // Function that should be called
      "core3",   // Name of the task (for debugging)
      10000,     // Stack size (bytes)
      NULL,      // Parameter to pass
      2,         // Task priority
      NULL,      // Task handle
      1          // Core you want to run the task on (0 or 1)
  );
  delay(500);
}

void loop()
{
  // put your main code here, to run repeatedly:
}