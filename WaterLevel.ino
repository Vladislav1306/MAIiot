#include <Wire.h> // Библиотека для работы с шиной I2C
#include <SPI.h> // Библиотека для работы с шиной SPI
#include <ESP8266WiFi.h> // Библиотека для подключения к WiFi
#include <PubSubClient.h> // Библиотека для подключения к MQTT
#include <Adafruit_BMP280.h> // Библиотека для работы с датчиком BMP280
#include <Adafruit_GFX.h> // Библиотекb для работы с дисплеем
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // Ширина дисплея в пикселях
#define SCREEN_HEIGHT 32 // Высота дисплея в пикселях

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Для подключения по аппаратному SPI (указываем только номер пина CS)
#define PIN_CS 10 Adafruit_BMP280 bmp(PIN_CS);
//константы
const float l = 0.4;
const float ro = 1000.0;
const float g = 9.8;
const float P_a = 101325.0;

//переменные
float height = 0;
float P = 0;
float T = 0;
float T_0 = 0;

// WiFi настройки
const char *ssid = "network"; // Имя WiFi сети
const char *password = "12312312"; // WiFi пароль

// MQTT Broker настройки
const char *mqtt_broker = "broker.emqx.io"; // EMQX broker endpoint
const char *mqtt_topic = "skibidi"; // Название MQTT топика
const char *mqtt_username = "skibidi_well"; // MQTT имя для аутентификации
const char *mqtt_password = "public"; // MQTT пароль для аутентификации
const int mqtt_port = 1883; // MQTT порт (TCP)

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
 Serial.begin(9600); // Для вывода отладочной информации в терминал
   WiFi.begin(ssid, password); // Подключение к WiFi
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
   Serial.println("Connected");
 Serial.println(WiFi.localIP());
   mqtt_client.setServer(mqtt_broker, mqtt_port); //Подключение к MQTT
   mqtt_client.setCallback(mqttCallback);
   connectToMQTTBroker();

 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //Ошибка если дисплей не найден
   Serial.println(F("SSD1306 allocation failed"));
   if(!bmp.begin()) { // Если датчик BMP280 не найден
     Serial.println(“BMP280 SENSOR ERROR”); // Выводим сообщение об ошибке
     for(;;); // Don't proceed, loop forever
   }
 bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, // Режим работы
       Adafruit_BMP280::SAMPLING_X2, // Точность изм. температуры
       Adafruit_BMP280::SAMPLING_X16, // Точность изм. давления
       Adafruit_BMP280::FILTER_X16, // Уровень фильтрации
       Adafruit_BMP280::STANDBY_MS_500); // Период просыпания, мСек
}
  void connectToMQTTBroker() {
     while (!mqtt_client.connected()) {
       String client_id = "esp8266-client-" + String(WiFi.macAddress());
       Serial.printf("Connecting to MQTT Broker as %s.....\n",
client_id.c_str());
       if (mqtt_client.connect(client_id.c_str(), mqtt_username,
mqtt_password)) {
           Serial.println("Connected to MQTT broker");
           mqtt_client.subscribe(mqtt_topic);
           // Публикуется сообщение при успешном подключении
           mqtt_client.publish(mqtt_topic, "SUCSSESS");
     } else {
           Serial.print("Failed to connect to MQTT broker, rc="); //Если не
получится подключится сразу, будут попытки каждые 5 секунд
           Serial.print(mqtt_client.state());
           Serial.println(" try again in 5 seconds");
           delay(5000);
       }
   }
}
void mqttCallback(char *topic, byte *payload, unsigned int length) {
     Serial.print("Message received on topic: ");
     Serial.println(topic);
     Serial.print("Message:");
     for (unsigned int i = 0; i < length; i++) {
       Serial.print((char) payload[i]);
     }
     Serial.println();
     Serial.println("-----------------------");
}

void loop() {
     if (!mqtt_client.connected()) {
     connectToMQTTBroker();
 }
   mqtt_client.loop();
   display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0,0);
 if (T_0 == 0) T_0 = bmp.readTemperature() + 273;
T = bmp.readTemperature() + 273;
P = bmp.readPressure();
height = (ro*g*l(P - P_a*T) + P*(P*T_0 - P_a*T_0))/(ro*g*P*T_0);
 if (height < 0) height = 0;
display.print("h = "); display.print(height, 4);
display.print(" m");
display.display();
char msg_out[20];
dtostrf(height,7,4,msg_out);
mqtt_client.publish(mqtt_topic, msg_out);
delay(10000);
}
