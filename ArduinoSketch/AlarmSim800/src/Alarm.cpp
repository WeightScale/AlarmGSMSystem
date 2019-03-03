#include "Alarm.h"
#include "GsmModemClass.h"
#include "Battery.h"
#include <avr/sleep.h>

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
	GsmModem.sendSMS(client->_phone.c_str(), _data);
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
		GsmModem.sendATCommand(F("ATH\n"), false);
		Alarm.event( false);
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
	PCICR |=(1<<PCIE0);
	PCMSK0 |= (1<<PCINT2)|(1<<PCINT1);	
}

/*void AlarmClass::interrupt(){
	bool p = debounce(SENSOR_INT_PIN);
	if (_pinInterrupt == p)
		return;
	_pinInterrupt = p;
	interrupt(true);	
}*/

void AlarmClass::handle() {	
	pinInterrupt();
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
	callAll();
	textAll(_pinInterrupt ? F("Alarm: Open sensor!!!") : F("Alarm: Closed sensor!!!"));	
	//interrupt(false);																		//TODO сделать false когда было доставлено сообщения
	//attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
	//digitalWrite(DEFAULT_LED_PIN, HIGH);
}

void AlarmClass::textAll(const String &message) {
	if(!message) return;
	if (_clients.moveToStart())	{
		do{
			if (_clients.current()->canSend()) {
				_clients.current()->text(message);
			}
		}while(_clients.next());		
	}	
}

void AlarmClass::callAll() {
	if(_clients.moveToStart()){
		do{
			if (_clients.current()->canSend()) {
				_clients.current()->call();
			}
		}while(_clients.next());
	}
};

void AlarmClass::_addClient(AlarmClient * client) {
	_clients.append(client);
}

bool AlarmClass::removeClient(AlarmClient * client) {
	if (!client)
		return false;
	if (client->root())
		return false;
	_clients.remove(client);	
	return true;
}

AlarmClient* AlarmClass::hashClient(const String& phone){
	if (_clients.length() == 0)
		return NULL;
	if(_clients.moveToStart()){
		do{
			if (phone.indexOf(_clients.current()->_phone)!=-1) {
				return _clients.current();
			}
		}while(_clients.next());
	}
	return NULL;
}

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
	_curentClient = hashClient(phone);	
	if (_curentClient){
		_msgDTMF = "";
		GsmModem.sendATCommand(F("ATA"), true);                  // ...отвечаем (поднимаем трубку)
		return true;
	}else{
		GsmModem.sendATCommand(F("ATH\n"), false);				//...сбрасываем
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
	
	_curentClient = hashClient(msgphone);
	if (!_curentClient){			// Если телефон в белом списке, то...				 
		fetchCommand(msgbody);                  // ...выполняем команду
	}else{
		textAll("In come message from tel:" + msgphone + " "+msgbody);	//отправляем чужие сообщения 
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
	GsmModem.sendATCommand(F("ATH\n"), false);
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
				GsmModem.sendATCommand(F("ATH\n"), false);
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
		case 123:																//добавить клиента
			if(!_curentClient->root())
				return false;
			onCommand([](const String& value) {
				if(Alarm.createClient(value))
					GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), String("Client: "+ value + "added").c_str());	
			});			
		break;
		case 321:																//удалить клиента
			if(!_curentClient->root())
				return false;
			onCommand([](const String& value) {	
				if(Alarm.removeClient(Alarm.hashClient(value)))
					GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), String("Client: " + value + "removed").c_str());	
			});						
		break;
		case 111:{																//список клиентов			
			if (!_curentClient->root())
				return false;
			//if (_clients.size() == 0)
			if (_clients.length() == 0)
				return false;			
			onCommand([](const String& value){
				Alarm.listClients();	
			});			
			break;			
		}
		case 222:																//поставить на сигнализацию
			if(_curentClient->safe())
				onCommand([](const String& value) {
					Alarm.safe(true);
					GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), "guarded!!!");	
				});
			else
				return false;			
		break;
		case 333:																//снять с сигнализации
			if(_curentClient->safe())
				onCommand([](const String& value) {
				Alarm.safe(false);
				GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), "not guarded!!!");	
			});
			else
				return false;
		break;
		case 444: {																//информация состояния модуля
			onCommand([](const String& value) {
				String info = "Battery:" + (String)BATTERY->charge() + "%";
				info += " Safe:" + (String)Alarm.safe();
				info += " Sensor:" + (String)(debounce(Alarm.interruptPin())?"OPEN":"CLOSE");				
				GsmModem.sendSMS(Alarm.curentClient()->_phone.c_str(), info.c_str());	
			});			
			break;
		}case 541: {															//уснуть	
			onCommand([](const String& value) {
				bool f = (bool)value.toInt();
				Alarm.sleep(f);
			});	
			break;
		}
		/*case 542: {																		//проснутся	
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

bool AlarmClass::createClient(const String& phone){	
	_addClient(new AlarmClient(phone,false, false,false));
	return true;
}

void AlarmClass::sleep(bool full_mode) {
	if(GsmModem.enterSleepMode(full_mode)){
		sleep_mode();
		GsmModem.disableSleep();
	}
	/*if (s){
		_sleep = GsmModem.enterSleepMode(full_mode);	
	}else{
		if (GsmModem.disableSleep())
			_sleep = false;
	}
	if(_sleep){
		//while ( !( UCSR1A & (1<<TXC1)) );
		delay(1000);
		sleep_mode();
		sleep(false);
	}*/
};

void AlarmClass::listClients(){
	String msg = "";
	if(_clients.moveToStart()){
		do{
			msg += _clients.current()->_phone + ":";
		}while(_clients.next());
		GsmModem.sendSMS(_curentClient->_phone.c_str(), msg.c_str());
	}
}

void AlarmClass::pinInterrupt(){
	bool p = debounce(SENSOR_INT_PIN);
	if (_pinInterrupt == p)
		return;
	_pinInterrupt = p;
	event(true);	
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
	//handleInterrupt();
}