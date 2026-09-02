// =====================================================
// SMART SMOKE DETECTOR + TELEGRAM
// ESP32 TYPE-C 30 PIN
// FULL CODE PART 1/3
// =====================================================


// ================= LIBRARY =================

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DHT.h>

// ================= PROJECT =================
#define AP_NAME "ESP32-SMOKE-SETUP"

// ================= TELEGRAM =================
#define BOT_TOKEN "8704392963:AAEgWptbTvmu_7yQy7EMWAhfdOxwU0-SR44"
#define CHAT_ID "816927248"

WiFiClientSecure telegramClient;

UniversalTelegramBot bot(
  BOT_TOKEN,
  telegramClient
);

// Anti spam
bool telegramAlertSent = false;
unsigned long lastTelegram = 0;
const unsigned long TELEGRAM_COOLDOWN = 60000;

// ================= WEB SERVER =================
WebServer server(80);

// ================= PIN CONFIGURATION =================

// MQ2 GAS SENSOR
#define MQ2_PIN 34

// DHT11
#define DHT_PIN 15
#define DHT_TYPE DHT11

// BUZZER
#define BUZZER_PIN 14

// LED
#define GREEN_LED_PIN 25
#define YELLOW_LED_PIN 26
#define RED_LED_PIN 27

// OLED
#define OLED_SDA 21
#define OLED_SCL 22

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ================= DHT =================
DHT dht(
  DHT_PIN,
  DHT_TYPE
);

// ================= THRESHOLD =================

// Gas level
#define SMOKE_WARNING 300
#define SMOKE_DANGER 800

// Temperature
#define TEMP_WARNING 35.0
#define TEMP_DANGER 40.0

// ================= SENSOR VARIABLE =================
int smokeValue = 0;
float temperature = 0.0;
float humidity = 0.0;
bool dhtOK = false;

// ================= SYSTEM STATUS =================
enum SystemStatus
{
  NORMAL,
  WARNING,
  DANGER
};
SystemStatus currentStatus = NORMAL;

// ================= BUZZER =================
#define BUZZER_ON LOW
#define BUZZER_OFF HIGH
String buzzerStatus = "OFF";

// ================= WIFI =================
bool wifiConnected = false;
// =====================================================
// FULL CODE PART 2/3
// FUNCTION SYSTEM
// =====================================================

// ================= TELEGRAM FUNCTION =================
void sendTelegramAlert()
{
  String message = "";
  message += "🚨 SMART SMOKE ALERT!\n\n";
  if(smokeValue >= SMOKE_DANGER)
  {
    message += "🔥 GAS / SMOKE LEVEL HIGH\n";
  }
  if(temperature >= TEMP_DANGER)
  {
    message += "🌡 TEMPERATURE HIGH\n";
  }
  message += "\nSmoke Level: ";
  message += smokeValue;
  message += "\nTemperature: ";
  message += temperature;
  message += " °C";
  message += "\nHumidity: ";
  message += humidity;
  message += " %";
  message += "\n\nStatus: DANGER";
  bot.sendMessage(
    CHAT_ID,
    message,
    ""
  );
}

// ================= READ SENSOR =================
void readSensor()
{
  // MQ2
  smokeValue = analogRead(
    MQ2_PIN
  );
  
  // DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if(!isnan(t))
  {
    temperature = t;
  }
  if(!isnan(h))
  {
    humidity = h;
    dhtOK = true;
  }
  else
  {
    dhtOK = false;
  }
}

// ================= STATUS DETECTION =================
void determineStatus()
{
  SystemStatus oldStatus = currentStatus;
  if(
    smokeValue >= SMOKE_DANGER ||
    temperature >= TEMP_DANGER
  )
  {
    currentStatus = DANGER;
  }
  else if(
    smokeValue >= SMOKE_WARNING ||
    temperature >= TEMP_WARNING
  )
  {
    currentStatus = WARNING;
  }
  else
  {
    currentStatus = NORMAL;
  }

  // ================= TELEGRAM TRIGGER =================
  if(
    currentStatus == DANGER &&
    oldStatus != DANGER
  )
  {
    if(
      millis() - lastTelegram > TELEGRAM_COOLDOWN
    )
    {
      sendTelegramAlert();
      lastTelegram = millis();
    }
  }
}

// ================= LED CONTROL =================
void updateLED()
{
  if(currentStatus == NORMAL)
  {
    digitalWrite(
      GREEN_LED_PIN,
      HIGH
    );
    digitalWrite(
      YELLOW_LED_PIN,
      LOW
    );
    digitalWrite(
      RED_LED_PIN,
      LOW
    );
  }
  else if(currentStatus == WARNING)
  {
    digitalWrite(
      GREEN_LED_PIN,
      LOW
    );
    digitalWrite(
      YELLOW_LED_PIN,
      HIGH
    );
    digitalWrite(
      RED_LED_PIN,
      LOW
    );
  }
  else
  {
    digitalWrite(
      GREEN_LED_PIN,
      LOW
    );
    digitalWrite(
      YELLOW_LED_PIN,
      LOW
    );
    digitalWrite(
      RED_LED_PIN,
      HIGH
    );
  }
}

// ================= BUZZER =================
void updateBuzzer()
{
  if(currentStatus == DANGER)
  {
    digitalWrite(
      BUZZER_PIN,
      BUZZER_ON
    );
    buzzerStatus = "ON";
  }
  else
  {
    digitalWrite(
      BUZZER_PIN,
      BUZZER_OFF
    );
    buzzerStatus = "OFF";
  }
}

// ================= OLED DISPLAY =================
void handleRoot()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta charset='UTF-8'>
<title>Smart Smoke Detector</title>

<style>
body{
font-family:Arial, sans-serif;
background:#eef2f7;
margin:0;
padding:15px;
text-align:center;
}

.header{
background:#202124;
color:white;
padding:20px;
border-radius:18px;
}

.grid{
display:grid;
grid-template-columns:repeat(2,1fr);
gap:15px;
max-width:600px;
margin:20px auto;
}

.card{
background:white;
padding:20px;
border-radius:18px;
box-shadow:0 4px 12px rgba(0,0,0,0.12);
}

.title{
font-size:18px;
font-weight:bold;
}

.value{
font-size:32px;
font-weight:bold;
margin-top:10px;
}

.safe{color:#16a34a;}
.warn{color:#d97706;}
.danger{color:#dc2626;}

.full{
max-width:600px;
margin:15px auto;
}

.small{
font-size:16px;
}
</style>
</head>

<body>
<div class='header'>
<h1>🔥 SMART SMOKE DETECTOR</h1>
<p>ESP32 IoT Safety System</p>
</div>

<div class='card full'>
<div class='title'>SYSTEM STATUS</div>
<div class='value'>
)rawliteral";

if(currentStatus == NORMAL)
  html += "<span class='safe'>🟢 NORMAL</span>";
else if(currentStatus == WARNING)
  html += "<span class='warn'>🟡 WARNING</span>";
else
  html += "<span class='danger'>🔴 DANGER</span>";

html += R"rawliteral(
</div>
</div>

<div class='grid'>

<div class='card'>
<div class='title'>💨 GAS</div>
<div class='value'>)rawliteral";
html += String(smokeValue);
html += R"rawliteral(</div>
<p>ppm level</p>
</div>

<div class='card'>
<div class='title'>🌡 TEMP</div>
<div class='value'>)rawliteral";
html += String(temperature);
html += R"rawliteral(</div>
<p>°C</p>
</div>

<div class='card'>
<div class='title'>💧 HUMIDITY</div>
<div class='value'>)rawliteral";
html += String(humidity);
html += R"rawliteral(</div>
<p>%</p>
</div>

<div class='card'>
<div class='title'>🔊 BUZZER</div>
<div class='value'>)rawliteral";
html += buzzerStatus;
html += R"rawliteral(</div>
</div>

</div>

<div class='card full'>
<div class='title'>📶 CONNECTION</div>
<p class='small'>WiFi : CONNECTED</p>
<p class='small'>Telegram : ONLINE</p>
<p class='small'>IP : )rawliteral";

html += WiFi.localIP().toString();

html += R"rawliteral(</p>
</div>

<script>
setTimeout(function(){
location.reload();
},2000);
</script>

</body>
</html>
)rawliteral";

server.send(200,"text/html; charset=utf-8",html);
}

void updateOLED()
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);

  display.println("SMART SMOKE");
  display.println("----------------");

  display.print("Gas : ");
  display.println(smokeValue);

  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.print("Hum : ");
  display.print(humidity);
  display.println(" %");

  display.print("Status: ");

  if(currentStatus == NORMAL)
    display.println("NORMAL");
  else if(currentStatus == WARNING)
    display.println("WARNING");
  else
    display.println("DANGER");

  display.display();
}

void setup()
{
  Serial.begin(115200);

  // PIN SETUP
  pinMode(
    MQ2_PIN,
    INPUT
  );
  pinMode(
    BUZZER_PIN,
    OUTPUT
  );
  pinMode(
    GREEN_LED_PIN,
    OUTPUT
  );
  pinMode(
    YELLOW_LED_PIN,
    OUTPUT
  );
  pinMode(
    RED_LED_PIN,
    OUTPUT
  );
  digitalWrite(
    BUZZER_PIN,
    BUZZER_OFF
  );

  // DHT START
  dht.begin();

  // OLED START
  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );
  if(!display.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ADDRESS
  ))
  {
    Serial.println(
      "OLED FAILED"
    );
  }
  display.clearDisplay();
  display.display();

  // ================= WIFI MANAGER =================
  WiFiManager wm;
  Serial.println(
    "Starting WiFi Manager..."
  );
  bool result = wm.autoConnect(
    AP_NAME
  );
  if(result)
  {
    wifiConnected = true;
    Serial.println(
      "WiFi Connected"
    );
    Serial.print(
      "IP Address: "
    );
    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    wifiConnected = false;
    Serial.println(
      "WiFi Failed"
    );
  }

  // Telegram SSL
  telegramClient.setInsecure();

  // Telegram online notification
  if(wifiConnected)
  {
    bot.sendMessage(
      CHAT_ID,
      "🔥 Smart Smoke Detector ONLINE",
      ""
    );
  }

  // WEB SERVER
  server.on(
    "/",
    handleRoot
  );
  server.begin();
  Serial.println(
    "Web Server Started"
  );
}

// ================= LOOP =================
void loop()
{
  server.handleClient();
  readSensor();
  determineStatus();
  updateLED();
  updateBuzzer();
  updateOLED();

  // SERIAL MONITOR
  Serial.print(
    "Gas: "
  );
  Serial.print(
    smokeValue
  );
  Serial.print(
    " | Temp: "
  );
  Serial.print(
    temperature
  );
  Serial.print(
    " | Hum: "
  );
  Serial.print(
    humidity
  );
  Serial.print(
    " | Status: "
  );

  if(currentStatus == NORMAL)
  {
    Serial.println(
      "NORMAL"
    );
  }

  else if(currentStatus == WARNING)
  {
    Serial.println(
      "WARNING"
    );
  }

  else
  {
    Serial.println(
      "DANGER"
    );
  }

  delay(1000);
}
