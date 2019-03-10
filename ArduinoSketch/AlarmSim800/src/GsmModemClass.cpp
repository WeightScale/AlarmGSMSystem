#include "GsmModemClass.h"
#include <avr/wdt.h>

GsmModemClass GsmModem;
//HardwareSerial ser(&UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1);
GsmModemClass::GsmModemClass() /*: HardwareSerial(&UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1)*/ {
	_buffer.reserve(BUFFER_RESERVE_MEMORY);
};

void GsmModemClass::start() {

	pinMode(RESET_PIN, OUTPUT);
	digitalWrite(RESET_PIN, HIGH);
#if RESET_MODE == RESET_MODE1
	pinMode(DTR_PIN, OUTPUT);
	digitalWrite(DTR_PIN,LOW);	
#endif // RESET_MODE

	_baud = DEFAULT_BAUD_RATE;

	Serial1.begin(_baud);
	
	reset();
	while (sendATCommand(F("AT"), true).indexOf("OK") == -1) {};
	while(!echoOff());		
	while (sendATCommand(F("AT+CLIP=1"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+CMGF=1;&W"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+DDET=1"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+CLCC=1"), true).indexOf("OK") == -1) {};
#if RESET_MODE == RESET_MODE1
	while(sendATCommand(F("AT+CSCLK=1"), true).indexOf(F("OK")) == -1){};
#endif // RESET_MODE
}

void GsmModemClass::reset() {	
	digitalWrite(RESET_PIN, LOW);
	delay(1000);
	digitalWrite(RESET_PIN, HIGH);
	delay(1000);
}

bool GsmModemClass::setFullMode() {
	//This set the device to full funcionality - AT+CFUN
	bool nowReady = false;
	String str = sendATCommand(F("AT+CFUN?"), true);
	int index = str.indexOf(F("CFUN:"));
	index = str.substring(index + 6, index + 7).toInt();
	if (index == 0) {
		Serial1.print(F("AT+CFUN=1"));
		str = _getResponse(F("OK"), 10000);
		if (str.indexOf(F("READY")) != -1)
			nowReady = true;
		str = _getResponse(F("Call Ready"), 20000);
	}else
		return true;
	return nowReady;
}

bool GsmModemClass::enterSleepMode(bool full) {
	if (full){
		String str = sendATCommand(F("AT+CFUN?"), true);
		int index = str.indexOf(F("CFUN:"));
		index = str.substring(index + 6, index + 7).toInt();
		if (index == 1){
			Serial1.print(F("AT+CFUN=0"));
			str = _getResponse(F("OK"),10000);
			if (str.indexOf(F("NOT READY")) == -1)
			return false;
		}	
	}
#if RESET_MODE == RESET_MODE1
	digitalWrite(DTR_PIN,HIGH);
#else if RESET_MODE == RESET_MODE2
	if(sendATCommand(F("AT+CSCLK=2"), true).indexOf(F("OK")) == -1)
		return false;
#endif // RESET_MODE		 	 
	return true;
}
 
bool GsmModemClass::disableSleep(bool full) {
#if RESET_MODE == RESET_MODE1
	digitalWrite(DTR_PIN,LOW);
	delay(100);
#else if RESET_MODE == RESET_MODE2
	Serial1.print(F("FF\r"));
	delay(300);  // this is between waking charaters and next AT commands	
	if(sendATCommand(F("AT+CSCLK=0"), true).indexOf(F("OK")) == -1)
		return false;
	delay(100);  // just chill for 100ms for things to stablize
	if(full){
		if(!setFullMode())
		return false;
	}
#endif // RESET_MODE
	return true;
}

// ECHO OFF
bool GsmModemClass::echoOff() {
	return sendATCommand(F("ATE0"), true).indexOf("OK")!=-1;
}

// ECHO ON
bool GsmModemClass::echoOn() {
	return sendATCommand(F("ATE1"), true).indexOf("OK")!=-1;
}

bool GsmModemClass::isReady() {
	Serial1.print(F("AT+CCALR?\r"));	
	String str = "";
	int status = -1;
	uint64_t timeOld = millis();	
	while (millis() < (timeOld + 2000)) {
		str = _waitResponse(1000);
		int index = str.indexOf(F("CCALR:"));
		if (index == -1)
			continue;
		status = str.substring(index + 7, index + 8).toInt();
		if (status != 1) {
			return false;	
		}
		return true;		
	}
	return false;
}

bool GsmModemClass::sendSMS(const char* number, const char* text) {			
	Serial1.print(F("AT+CMGS=\""));    // command to send sms
	Serial1.print(number);
	Serial1.print(F("\"\r\n"));	
	if(_checkResponse(">", 20000)) {
		Serial1.print(text);
		Serial1.print("\r");
		//delay(1000);
		//_checkResponse(OK_,1000);
		Serial1.print((char)26);		
		return _checkResponse("OK", 10000);
	}
	return false;
}

bool GsmModemClass::sendSMS(const char* number, uint8_t* text) {	
	Serial1.print(F("AT+CMGS=\""));     // command to send sms
	Serial1.print(number);
	Serial1.print(F("\"\r\n"));
	if(_checkResponse(">", 20000)) {
		Serial1.print(String((char*)text));
		Serial1.print("\r");
		//_checkResponse(OK_, 1000);
		Serial1.print((char)26);
		return _checkResponse("OK", 10000);
	}
	return false;
}

String GsmModemClass::getSMS(uint8_t index){
	return sendATCommand("AT+CMGR=" + (String)index + ",1", true);
}

void GsmModemClass::doCall(const String& phone, uint16_t timeout) {
	Serial1.print(F("ATD"));      // command to do call
	Serial1.print(phone);
	Serial1.print(";\n");
	String str = "";
	int status = -1;
	uint64_t timeOld = millis();	
	while (millis() < (timeOld + timeout)){
		str = _waitResponse(1000);
		int index = str.indexOf(F("+CLCC:"));
		if (index == -1)
			continue;
		status = str.substring(index + 11, index + 12).toInt();
		if (status == 3){
			timeOld = millis();
			while (millis() < (timeOld + 6000)) {delay(1); wdt_reset();}
			break;	
		}		
	}
	_handleAccept(status);
};

String GsmModemClass::_getResponse(String ask, uint16_t timeout){
	unsigned long t = millis();
	String tempData = "";
	while (millis() < t + timeout) {
		//count++;
		if(Serial1.available()) {
			tempData += this->_readSerial(timeout);    // reads the response	
			if(tempData.indexOf(ask) != -1)
				break;				  
		}
		delay(1);
		wdt_reset();
	}
	return tempData;
}

bool GsmModemClass::_checkResponse(String ask, uint16_t timeout){	
	unsigned long t = millis();
	while(millis() < t + timeout){
		//count++;
		if(Serial1.available()){
			String tempData = this->_readSerialUtil('\n', timeout);   // reads the response	
			//String tempData = this->_readSerial(timeout);    // reads the response
			if (tempData.indexOf(ask) != -1)
				return true;
		}
		delay(1);
		wdt_reset();		
	}
	return false;
}

int GsmModemClass::timedRead() {
	int c;
	_startMillis = millis();
	do {
		c = Serial1.read();
		if (c >= 0)
			return c;
		yield();
	} while (millis() - _startMillis < _timeout);
	return -1;     // -1 indicates timeout
}

String GsmModemClass::_readSerialUtil(char terminator, uint16_t timeout) {

	uint64_t timeOld = millis();
	String tempData = "";
	while (!Serial1.available() && !(millis() > timeOld + timeout)) {
		delay(1);
	}
	if (Serial1.available()) {		
		//String ret;
		int c = timedRead();
		while (c >= 0 && c != terminator && !(millis() > (timeOld + timeout))) {
			tempData += (char) c;
			c = timedRead();
			delay(1);
		}
	return tempData;		
	}
	return tempData;	
}

/*String GsmModemClass::_readSerial() {	
	String tempData = "";
	uint64_t timeOld = millis();
	while (Serial1.available() && !(millis() > (timeOld + 50))) {
		if (Serial1.available()) {	
			tempData += (char) Serial1.read();
			timeOld = millis();
		}
		delay(1);
	}
	return tempData;	
}*/

String GsmModemClass::_readSerial(uint32_t timeout) {	
	String tempData = "";
	uint64_t timeOld = millis();
	wdt_reset();
	while (Serial1.available() && !(millis() > (timeOld + timeout))) {
		if (Serial1.available()) {	
			tempData += (char) Serial1.read();
			//tempData += Serial1.readString();
			timeOld = millis();
		}
		delay(1);
		wdt_reset();
	}
	return tempData;
}

String GsmModemClass::sendATCommand(const String& cmd, bool waiting, uint32_t timeout){
	String _resp;                                                // Переменная для хранения результата	
	Serial1.println(cmd);                                               // Отправляем команду модулю
	if(waiting) {// Если необходимо дождаться ответа...		
		_resp = _waitResponse(timeout);                                         // ... ждем, когда будет передан ответ
		//_resp = _readSerial(timeout);
		// Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
		if(_resp.startsWith(cmd)) {// Убираем из ответа дублирующуюся команду			
			_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
		}
	}
	return _resp;                                                   // Возвращаем результат. Пусто, если проблема
}

String GsmModemClass::_waitResponse(uint32_t timeout) {
	// Функция ожидания ответа и возврата полученного результата
	String _resp = "";                                                // Переменная для хранения результата
	long _timeout = millis() + timeout;                                 // Переменная для отслеживания таймаута (10 секунд)
	while(!Serial1.available() && millis() < _timeout) {delay(1); wdt_reset(); };         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
	if(Serial1.available()) {
		// Если есть, что считывать...
		_resp = Serial1.readString();                                      // ... считываем и запоминаем
	}	
	return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}

/*static int millis_ge(unsigned long ms) {
long l = (long)(millis() - ms);
return (l) >= 0;
};*/