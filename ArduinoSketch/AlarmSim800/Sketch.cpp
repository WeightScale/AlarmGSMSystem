/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

/*End of auto generated code by Atmel studio */

#include "TaskController.h"
#include "Memory.h"
#include "Alarm.h"
#include "Battery.h"
#include "GsmModemClass.h"
#include "PhoneBookClass.h"
//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio
String str = "";
byte count_ring = 0;
bool watchdog = false;

TaskController taskController = TaskController();

Task tasPowerDown([](){
	//wakeup = false;	
	Alarm.sleep();
	tasPowerDown.updateCache();
},10000);

void test(){
	cli();
	wdt_reset();
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (0<<WDE) | (1<<WDIE)|(1<<WDP3)|(1<<WDP0);
	sei();
}

Task taskTimeAdmin([]{
	PhoneBook.time_admin(false);
	taskController.remove(&taskTimeAdmin);
	test();
	tasPowerDown.resume();
},60000);

void setup(){
	ACSR = (1<<ACD); //Turn off Analog Comparator - this removes about 1uA
	
	cli();
	wdt_reset();
	//MCUSR &= ~(1<<WDRF);
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	/* Set new prescaler (time-out) value = 64K cycles (~0.5 s) */
	WDTCSR = (1<<WDE) | (0<<WDIE)|(1<<WDP3)|(1<<WDP0);
	sei();
	
	SerialUSB.begin(9600);
	Memory.init();
	Alarm.begin();
	//Alarm._addClient(new AlarmClient("0500784234",true,true,true));
	BATTERY = new BatteryClass();
	BATTERY->onDischaged([](int charge) {
		PhoneBook.textAll(F("Charge battery low "));
		delay(1000);
		Alarm.sleep(true);
		BATTERY->updateCache();		
	});
	GsmModem.start();
	PhoneBook.init();	
	//taskController.add(BATTERY);/**/	
	//taskController.add(&tasPowerDown);
	tasPowerDown.pause();
	taskController.add(&taskTimeAdmin);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		
	wdt_disable();
	//test();
}
unsigned int time_sleep;
void loop(){	
	if (watchdog){
		//unsigned long t = millis();
		time_sleep += 8;	/* 8 секунд */
		pci_enable();
		if(Alarm.pinInterrupt()){
			goto alarm;	
		};
		if(BATTERY->shouldRun(time_sleep)){
			BATTERY->fetchCharge();
			time_sleep = 0;
		}			
		watchdog = false;
		Alarm.sleep();
	}
	alarm:
	//wdt_reset();
	Alarm.handle();	
	if (Serial1.available()){
		str = GsmModem._readSerial();
		SerialUSB.println(str);
		if (str.indexOf(F("RING")) != -1) {
			if ((count_ring++) > 2){
				count_ring = 0;	
				str = str.substring(str.indexOf(F("+CLIP:")) + 6, str.indexOf(","));
				str.replace("\"", "");
				str.trim();
				if(Alarm.fetchCall(str)){
					Alarm.waitDTMF();	
				};
			}
		}else if (str.indexOf(F("+CMTI:")) != -1){
			//str = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);  // Отправляем запрос чтения непрочитанных сообщений
			int i = str.substring(str.indexOf(F("SM")) + 4, str.lastIndexOf("\r")).toInt();
			Alarm.fetchMessage(i);			
		}else if (str.indexOf(F("UNDER")) != -1){
			PhoneBook.textAll(str);
		}else if (str.startsWith(F("NO CARRIER"))){
			Alarm._msgDTMF = "";
			count_ring = 0;
		}else{
			if (str.indexOf(F("Call Ready")) != -1) {
			}
			str = "";
		}	
		tasPowerDown.updateCache();
		wdt_reset();
	}
	if (SerialUSB.available()){
		Serial1.write(SerialUSB.read());
	}
	taskController.run();
}

ISR(WDT_vect){
	watchdog = true;
	wdt_reset();
}