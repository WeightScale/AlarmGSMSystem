#pragma once
#include "Arduino.h"
//#include "HardwareSerial.h"
//#include "HardwareSerial_private.h"

#define RESET_MODE1	1
#define RESET_MODE2	2

#define RESET_MODE RESET_MODE1

#define RESET_PIN	14
#define DTR_PIN		9

//#define DEFAULT_LED_FLAG true
//#define DEFAULT_LED_PIN 15

#define DEFAULT_BAUD_RATE			9600
#define BUFFER_RESERVE_MEMORY		255
#define TIME_OUT_READ_SERIAL		5000

/*enum enum_ask_t{	
	ERROR,
	NOT_READY,
	READY,
	CONNECT_OK,
	CONNECT_FAIL,
	ALREADY_CONNECT,
	SEND_OK,
	SEND_FAIL,
	DATA_ACCEPT,
	CLOSED_,
	READY_TO_RECEIVE,
	OK_
};*/

class GsmModemClass  /*: public HardwareSerial*/{
	
	typedef void(*HandleCallAccept)(int);
	
private:
	unsigned long _startMillis;   // used for timeout measurement
	uint32_t _baud;
	uint8_t _timeout;
	String _buffer;
	HandleCallAccept _handleAccept;	
	bool _checkResponse(String ask, uint16_t timeout = TIME_OUT_READ_SERIAL);
	String _getResponse(String ask, uint16_t timeout = TIME_OUT_READ_SERIAL);
	String _readSerialUtil(char terminator, uint16_t timeout);
	String _waitResponse(uint32_t timeout);		
public:	
	GsmModemClass();
	int timedRead();
	String _readSerial(uint32_t timeout = 50);
	//String _readSerial();
	void start();
	void reset();
	bool setFullMode();
	bool echoOff();
	bool isReady();
	bool echoOn();
	bool enterSleepMode(bool full = false);
	bool disableSleep(bool full = false);
	bool sendSMS(const char* number, const char* text);
	bool sendSMS(const char* number, uint8_t* text);
	String getSMS(uint8_t index);
	void processSMS(const String );
	String sendATCommand(const String& cmd, bool waiting, uint32_t timeout = TIME_OUT_READ_SERIAL);
	void onCallAccept(HandleCallAccept accept) {_handleAccept = accept;};
	void doCall(const String& phone, uint16_t timeout);
	
};

extern GsmModemClass GsmModem;