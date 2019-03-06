#include <Arduino.h>

#define RESET_PIN	14

void setup(){	
	pinMode(RESET_PIN, OUTPUT);
	digitalWrite(RESET_PIN, HIGH);
	SerialUSB.begin(9600);
	SerialUSB.println("START SIM800L");
	
	digitalWrite(RESET_PIN, LOW);
	delay(1000);
	digitalWrite(RESET_PIN, HIGH);
	delay(1000);
	
	Serial1.begin(9600);
	
	
	Serial1.println("AT");
	Serial1.print("AT+GMM\r\n");
	Serial1.print("AT+DDET=1\r\n");
}

void loop()
{
	if (Serial1.available()){
		//String str = mySerial.readString();
		SerialUSB.write(Serial1.read());
	}
	if (SerialUSB.available())
	{
		Serial1.write(SerialUSB.read());
	}
}