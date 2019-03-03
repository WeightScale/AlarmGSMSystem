/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include <avr/sleep.h>

/*End of auto generated code by Atmel studio */

#include "TaskController.h"
#include "Memory.h"
#include "Alarm.h"
#include "Battery.h"
#include "GsmModemClass.h"
//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio
String str = "";
byte count_ring = 0;
//bool wakeup = false;

Task tasPowerDown([](){
	//wakeup = false;	
	Alarm.sleep();
	tasPowerDown.updateCache();
},10000);
TaskController taskController = TaskController();

void setup(){
	ACSR = (1<<ACD); //Turn off Analog Comparator - this removes about 1uA
	SerialUSB.begin(9600);
	Memory.init();
	Alarm.begin();
	Alarm._addClient(new AlarmClient("0500784234",true,true,true));
	BATTERY = new BatteryClass();
	/*BATTERY->onDischaged([](int charge) {
		/ *if (!Alarm.sleep()) {
			taskSleepModem.resume();	
		}	* /
		BATTERY->resume();
	});*/
	GsmModem.start();
		
	taskController.add(BATTERY);/**/	
	taskController.add(&tasPowerDown);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void loop(){
	taskController.run();
	Alarm.handle();
	if (Serial1.available()){
		str = GsmModem._readSerial();
		if (str.indexOf(F("RING")) != -1) {
			if ((count_ring++) > 1){
				count_ring = 0;	
				str = str.substring(str.indexOf(F("+CLIP:")) + 6, str.indexOf(","));
				str.replace("\"", "");
				str.trim();
				if(Alarm.fetchCall(str)){
					Alarm.waitDTMF();	
				};
			}//else
				//goto _exit;
		}else if (str.indexOf(F("+CMTI:")) != -1){
			//str = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);  // Отправляем запрос чтения непрочитанных сообщений
			int i = str.substring(str.indexOf(F("SM")) + 4, str.lastIndexOf("\r")).toInt();
			Alarm.fetchMessage(i);
			
		}else if (str.indexOf(F("UNDER")) != -1){
			Alarm.textAll(str);
		}else if (str.startsWith(F("NO CARRIER"))){
			Alarm._msgDTMF = "";
			count_ring = 0;
			//goto _exit;
		}else{
			if (str.indexOf(F("Call Ready")) != -1) {
			}
			str = "";
			//goto _exit;
		}	
		tasPowerDown.updateCache();
	}
	//_exit:{}
}