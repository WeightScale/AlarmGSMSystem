#include "Alarm.h"
#include "GsmModemClass.h"
#include "Battery.h"
#include "Commands.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "PhoneBookClass.h"

AlarmClass Alarm;

AlarmSMSMessage::AlarmSMSMessage(const char * data, size_t len)	: _len(len){	
	_data = (uint8_t*)malloc(_len + 1);
	if (_data == NULL) {
		_len = 0;
		_status = AL_MSG_ERROR;
	}else {
		_status = AL_MSG_SENDING;
		memcpy(_data, data, _len);
		_data[_len] = 0;
	}
};

AlarmSMSMessage::~AlarmSMSMessage() {
	if (_data != NULL)
		free(_data);
};

void AlarmSMSMessage::send(AlarmClient *client) {
	if (_status != AL_MSG_SENDING)
		return ;
	GsmModem.sendSMS(client->phone().c_str(), _data);
	_status = AL_MSG_SENDING;
}

void AlarmClient::_runQueue() {	
	if (_message) {
		_message->send(this);
		delete _message;
	}
}

void AlarmClient::_queueMessage(AlarmMessage *dataMessage) {	
	if (dataMessage == NULL)
		return;	
	_message = dataMessage;
	if (canSend())
		_runQueue();
}

void AlarmClient::text(const char * message, size_t len) {
	_queueMessage(new AlarmSMSMessage(message, len));
}
void AlarmClient::text(const String &message) {
	text(message.c_str(), message.length());
}

void AlarmClient::call() {
	GsmModem.onCallAccept([](int value) {
		GsmModem.sendATCommand(F("ATH"), false);
	});
	GsmModem.doCall(_phone, 10000);
};

AlarmClass::AlarmClass(){
};

AlarmClass::~AlarmClass() {
};

void AlarmClass::begin() {		
	TX_RX_LED_INIT;
	TXLED0;
	RXLED0;
	pinMode(SENSOR_INT_PIN, INPUT_PULLUP);
	pinMode(WAKEUP_INT_PIN, INPUT);	
	pinInterrupt();	
	//attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
	PCMSK0 |= (1<<PCINT2)|(1<<PCINT1);
	pci_enable();		
}

/*void AlarmClass::interrupt(){
	bool p = debounce(SENSOR_INT_PIN);
	if (_pinInterrupt == p)
		return;
	_pinInterrupt = p;
	interrupt(true);	
}*/

void AlarmClass::handle() {	
	//pinInterrupt();
	if (!event()){
		return;	
	}		
	if (!_safe){
		event(false);
		return;	
	}
	//sleep(false);
	//sleep(!GsmModem.disableSleep());
	//detachInterrupt(interruptPin);
	_pinInterrupt = debounce(SENSOR_INT_PIN);
	//digitalWrite(DEFAULT_LED_PIN, LOW);
	pci_disable();	
	PhoneBook.callAll();
	PhoneBook.textAll(_pinInterrupt ? F("Alarm: Open sensor!!!") : F("Alarm: Closed sensor!!!"));	
	pci_enable();
	event( false);
	//interrupt(false);																		//TODO сделать false когда было доставлено сообщения
	//attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
	//digitalWrite(DEFAULT_LED_PIN, HIGH);
}



/*void AlarmClass::addClient(AlarmClient * client) {
	if (!client)
		return;
	//if (PhoneBook.addContactToSIM(client)){
	_clients.append(client);
	//}
}

bool AlarmClass::removeClient(AlarmClient * client) {
if (!client)
return false;
if (client->root())
return false;
_clients.remove(client);
return true;
}*/



void AlarmClass::fetchMessage(uint8_t index) {
	String str = "";
	bool hasmsg = false;
	str = GsmModem.getSMS(index);
	str.trim();                                         // Убираем пробелы в начале/конце
    if(str.endsWith(F("OK"))) {// Если ответ заканчивается на "ОК"
		if(!hasmsg) 
			hasmsg = true;                            // Ставим флаг наличия сообщений для удаления
        //GsmModem.sendATCommand("AT+CMGR=" + (String)index, true);     // Делаем сообщение прочитанным
		//GsmModem.sendATCommand("\n", true);                              // Перестраховка - вывод новой строки
		parseSMS(str);                                   // Отправляем текст сообщения на обработку            
	}else {		                                                  // Если сообщение не заканчивается на OK	  
		GsmModem.sendATCommand("\n", true);                              // Отправляем новую строку и повторяем попытку
	}
};

bool AlarmClass::fetchCall(String& phone) {
	_curentClient = PhoneBook.hashClient(phone);	
	if (_curentClient){
		_msgDTMF = "";
		GsmModem.sendATCommand(F("ATA"), true);                  // ...отвечаем (поднимаем трубку)
		return true;
	}else{
		GsmModem.sendATCommand(F("ATH"), false);				//...сбрасываем
		return false;
	}	
};

void AlarmClass::parseSMS(String& msg) {
	String msgheader  = "";
	String msgbody    = "";
	String msgphone   = "";

	msg = msg.substring(msg.indexOf(F("+CMGR: ")));
	msgheader = msg.substring(0, msg.indexOf("\r"));             // Выдергиваем телефон

	msgbody = msg.substring(msgheader.length() + 2);
	msgbody = msgbody.substring(0, msgbody.lastIndexOf(F("OK")));   // Выдергиваем текст SMS
	msgbody.trim();

	int firstIndex = msgheader.indexOf("\",\"") + 3;
	int secondIndex = msgheader.indexOf("\",\"", firstIndex);
	msgphone = msgheader.substring(firstIndex, secondIndex);
	
	_curentClient = PhoneBook.hashClient(msgphone);
	if (!_curentClient){			// Если телефон в белом списке, то...				 
		fetchCommand(msgbody);                  // ...выполняем команду
	}else{
		PhoneBook.textAll("In come message from tel:" + msgphone + " "+msgbody);	//отправляем чужие сообщения 
	}	
}

void AlarmClass::waitDTMF(unsigned long timeout){
	String str = "";
	unsigned long t = millis();
	while(millis() < t + timeout){
		str = GsmModem._readSerial();		
		if (str.indexOf(F("+DTMF:")) != -1) {
			str.trim();
			str = str.substring(7, 8);
			if(Alarm.parseDTMF(str))
				return;
			t = millis();
		}
	}
	GsmModem.sendATCommand(F("ATH"), false);
}

bool AlarmClass::parseDTMF(String& msg) {
	if (msg.equals("#")) {			
		int _inx = _msgDTMF.indexOf("*");
		String command="",value="";
		if (_inx != -1) {
			command = _msgDTMF.substring(0, _inx);
			value = _msgDTMF.substring(_msgDTMF.indexOf("*") + 1);
		}else{
			command = _msgDTMF.substring(0);
		}		
		if (command.length() == 3){
			if (fetchCommand(command)) {// ...выполняем команду
				GsmModem.sendATCommand(F("ATH"), false);
				if (_handleCommand)
					_handleCommand(value);
				_msgDTMF = "";
				return true;
			};
		}		
	}else {
		_msgDTMF += msg;	
	};
	return false;
};

bool AlarmClass::fetchCommand(String cmd) {	
	switch (cmd.toInt()){
		case CMD_ADD_RESERVE:																//добавить клиента
			if(!_curentClient->root())
				return false;
			onCommand([](const String& value) {
				if(value.length() < 10)
					return;
				AlarmClient c(2,value);
				if(!PhoneBook.addContactToSIM(&c,"Reserve"))
					return;
				if (PhoneBook.addReserve()){
					//Alarm.curentClient()->text(String("Client reserve added"));	
					PhoneBook.reserve()->call();				
				}
			});			
		break;
		case CMD_DEL_RESERVE:																//удалить клиента
			if(!_curentClient->root())
				return false;
			onCommand([](const String& value) {	
				if(PhoneBook.delContactFromSIMM(2))
					Alarm.curentClient()->text(String("Client reserve removed"));
					//GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), String("Client: " + value + "removed").c_str());	
			});						
		break;
		case CMD_ADD_ADMIN:		
			if(!PhoneBook.time_admin())
				return false;
			onCommand([](const String& value) {
				if(!PhoneBook.addContactToSIM(Alarm.curentClient(),"Admin"))
					return;
				if (PhoneBook.addAdmin()){
					//Alarm.curentClient()->text(String("Client admin added"));
					PhoneBook.admin()->call();
				}
			});
		break;			
		case CMD_LIST_CLIENT:																//список клиентов			
			if (!_curentClient->root())
				return false;		
			onCommand([](const String& value){
				Alarm.curentClient()->text(PhoneBook.listClients());	
			});			
		break;	
		case CMD_ALARM_ON:																//поставить на сигнализацию
			if(_curentClient->safe())
				onCommand([](const String& value) {
					Alarm.safe(true);
					Alarm.curentClient()->text("guarded!!!");
					//GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), "guarded!!!");	
				});
			else
				return false;			
		break;
		case CMD_ALARM_OFF:																//снять с сигнализации
			if(_curentClient->safe())
				onCommand([](const String& value) {
				Alarm.safe(false);
				Alarm.curentClient()->text("not guarded!!!");
				//GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), "not guarded!!!");	
			});
			else
				return false;
		break;
		case CMD_INFO:																//информация состояния модуля
			onCommand([](const String& value) {
				String info = "Battery:" + (String)BATTERY->charge() + "%";
				info += " Safe:" + (String)Alarm.safe();
				info += " Sensor:" + (String)(debounce(Alarm.interruptPin())?"OPEN":"CLOSE");
				Alarm.curentClient()->text(info);				
				//GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), info.c_str());	
			});			
		break;
		/*case 541: {															//уснуть	
			onCommand([](const String& value) {
				bool f = (bool)value.toInt();
				Alarm.sleep(f);
			});	
			break;
		}
		case 542: {																		//проснутся	
			onCommand([](const String& value) {
				Alarm.sleep(false);	
			});
			break;
		}*/
		default:
			return false;
	}	
	return true;
};

/*bool AlarmClass::createClient(const String& phone){
addClient(new AlarmClient(phone,false, false,false));
return true;
}*/

void AlarmClass::sleep(bool full_mode) {
	if(GsmModem.enterSleepMode(full_mode)){
		sei();
		sleep_mode();
		GsmModem.disableSleep();
	}
};



bool AlarmClass::pinInterrupt(){
	bool p = debounce(SENSOR_INT_PIN);
	if (_pinInterrupt == p)
		return false;
	_pinInterrupt = p;
	return _event = true;	
}

bool debounce(uint8_t pin) {
	bool current = digitalRead(pin);
	if (current != Alarm.getStatusPinInt()) {// Старое значение отличается от полученного		                  
	  delay(10);                                   // Ждем пока состояние стабилизируется - игнорируем дребезг
	  current = digitalRead(pin);             // Считываем стабилизированное значение
	}
	return current;
}

void handleInterrupt() {
	Alarm.event(true);	               
	//Alarm.setStatusPinInt(debounce());// Получаем стабилизированное значение
	//digitalWrite(DEFAULT_LED_PIN, LOW);	
};

ISR(PCINT0_vect){
	wdt_reset();
	Alarm.pinInterrupt();
}
