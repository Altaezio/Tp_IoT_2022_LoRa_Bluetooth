#include <Wire.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include <SHT21.h>
#include <PubSubClient.h>
#include <LoRa.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#define SSID "marcbarbierlaptop"
#define PASSWORD "suassig1"

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DI0 26

#define LORA

const char * MQTT_SRV = "test.mosquitto.org";

WiFiClient wclient;
PubSubClient client(wclient);

U8X8_SSD1306_128X64_NONAME_SW_I2C display(15, 4, 16);   // OLEDs without Reset of the Display
float stab[] = { 0, 0, 0, 0 };
bool lora_started;
SHT21 sht21;

void read_sht21() {
	stab[0] = sht21.getTemperature();
	stab[1] = sht21.getHumidity();
	Serial.printf("T:%f H:%f\n", stab[0], stab[1]);
}

void print_display() {
	char buff[64];
	display.setFont(u8x8_font_5x7_f);
	display.drawString(0, 0, "Chibrax co LTD");
	sprintf(buff, "T: %2.f, H: %2.f\nd3: %2.f, d4: %2.f", stab[0], stab[1], stab[2], stab[3]);
	display.drawString(0, 18, buff);
	display.display();
}

void read_mqtt(char * topic, byte * payload, unsigned int length) {
	Serial.println(topic);
	if(strcmp(topic, "/info/youssance") != 0 || lora_started)
		return;

	char message[length + 1];
	memcpy(message, payload, length);
	char *end;
	message[length] = '\0';

	Serial.println(message);

	long freq = strtol(message, &end,10);
	long sf = strtol(end + 1, &end,10);
	long sb = strtol(end + 1, &end,10);
	Serial.printf("%ld %ld %ld\n", freq, sf, sb);
	LoRa.setSpreadingFactor(sf);
	LoRa.setSignalBandwidth(sb);
	lora_started = true;
	if(!LoRa.begin(freq)) {
		Serial.println("Starting lora failed");
		lora_started = false;
	}
	Serial.println("Started lora");
}

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(9600);
	Wire.begin(21, 22);
	display.begin();
	SPI.begin(SCK, MISO, MOSI, SS);
	LoRa.setPins(SS, RST, DI0);

	WiFi.begin(SSID, PASSWORD);
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print('.');
	}
	Serial.printf("IP: %s", WiFi.localIP().toString().c_str());

	client.setServer(MQTT_SRV, 1883);

	while(!client.connected()) {
		Serial.println("Connecting to MQTT");
		if(client.connect("Chibrax Co")) {
			Serial.println("Connected");
		} else {
			Serial.println("Failed to connect");
			delay(1000);
		}
	}

	client.setCallback(read_mqtt);
	client.subscribe("/info/youssance");
	client.publish("/info/alban", "Sum good suassig");
}

void lora_loop() {
	Serial.println("Sending to lora temps");
	LoRa.beginPacket();
	char buff[64];
	sprintf(buff, "T: %2.f, H: %2.f\nd3: %2.f, d4: %2.f", stab[0], stab[1], stab[2], stab[3]);
	LoRa.print(buff);
	LoRa.endPacket();
}

void loop()
{
	client.loop();
	read_sht21();
	print_display();

	if(lora_started) {
		lora_loop();
	}

	delay(5000);
}
