#pragma once
#include <Arduino.h>
#include "Task.h"
#include "Vector.h"
//#include <ArduinoJson.h>
//#include <functional>

#define SENSOR_INT_PIN 16
#define WAKEUP_INT_PIN 15
#define AL_MAX_QUEUED_MESSAGES 8

#define pci_enable()    PCICR |=(1<<PCIE0)
#define pci_disable()    PCICR &=~(1<<PCIE0)

typedef enum { AL_MSG_SENDING, AL_MSG_SENT, AL_MSG_ERROR } AlarmMessageStatus;
class AlarmMessage;
class AlarmClient;
class AlarmClass;

class AlarmMessage{
protected:
	AlarmMessageStatus _status;
public:
	AlarmMessage() : _status(AL_MSG_ERROR) {}
	virtual ~AlarmMessage() {};
	virtual void send(AlarmClient *client __attribute__((unused))){ return; }
	virtual bool finished(){ return _status != AL_MSG_SENDING; }
	virtual bool betweenFrames() const { return false; }
};

class AlarmSMSMessage : public AlarmMessage {
private:
	size_t _len;
	uint8_t * _data;
public:
	AlarmSMSMessage(const char * data, size_t len);
	virtual ~AlarmSMSMessage() override;
	virtual void send(AlarmClient *client) override;
};

class AlarmCallMessage : public AlarmMessage {
public:
	virtual ~AlarmCallMessage() override;
	virtual void send(AlarmClient *client) override;
};

class AlarmClient{
private:
	//LinkedList<AlarmMessage *> _messageQueue;
	AlarmMessage * _message;
	void _queueMessage(AlarmMessage *dataMessage);
	void _runQueue();
	unsigned char _id;
	bool _send;
	bool _safe;
	//bool _root = false;
	String _phone;	
protected:	
public:
	AlarmClient(int id, const String& phone, bool send = true, bool safe = false/*, bool root = false*/)
		: _id(id), _send(send),_safe(safe)/*,_root(root)*/ {	
			_phone = phone;	
		};
	AlarmClient(int id, char * phone, bool send=true/*, bool r=false*/): _id(id), _send(send)/*,_root(r)*/{
		_phone = String(phone);
	};
	AlarmClient(int id, String& phone, bool send = true) : _id(id), _send(send) {
		_phone = phone;
	};
	/*AlarmClient(String p, bool s = true) : _send(s) {
		//_phone = String(&p);
	};*/
	~AlarmClient() {if (_message != NULL)free(_message); };
	String& phone(){return _phone;};
	//bool operator ! (const AlarmClient& c){return true;};
	//const bool operator !(const AlarmClient &c){return !(bool)c;};
	bool canSend(){return _send;}; //ack is not pending
	void text(const char * message, size_t len);
	void text(const String &message);
	void call();
	bool root() {return _id == 1;};
	//void root(bool root) {_root = root;};
	bool safe() {return _safe;};
	unsigned char id(){return _id;};	
};

//typedef std::function<void(String)> AlarmHandleCommand;
typedef void(*AlarmHandleCommand)(const String&);
class AlarmClass{	
private:
	AlarmHandleCommand _handleCommand;
	bool _event = false;
	bool _safe = true;
	//bool _sleep = false;
	bool _pinInterrupt;
	//volatile byte interruptCounter = 0;
	//const byte _interruptPin = SENSOR_INT_PIN;
	//LinkedList<AlarmClient *> _clients;
	//AlarmClient* _clients[10];
	//String _codeOnAlarm = "1234";
	//String _codeOffAlarm = "4321";
	AlarmClient* _curentClient;
	
public:
	AlarmClass();
	~AlarmClass();
	String _msgDTMF = "";
	
	//AlarmClient getClient(String phone);
	void begin();
	AlarmClient* curentClient() {return _curentClient;};
	bool event(){return _event;};
	void event(bool e){_event = e;};
	bool pinInterrupt();
	bool safe(){return _safe;};
	void safe(bool safe) {_safe = safe;};
	//bool is_sleep(){return _sleep;};
	void sleep(bool full_mode = false);
	//bool sleep() {return _sleep;};
	void handle();
	
	void cleanBuffers();
	//void addClient(AlarmClient * client); 
	//bool removeClient(AlarmClient * client);
	void fetchMessage(uint8_t index);
	bool fetchCall(String& phone);
	void parseSMS(String& msg);
	bool parseDTMF(String& msg);
	bool fetchCommand(String cmd);
	void setStatusPinInt(bool pin) {_pinInterrupt = pin; };
	bool getStatusPinInt() {return _pinInterrupt;};
	byte interruptPin() {return SENSOR_INT_PIN;};
	//bool createClient(const String& phone);
	
	//size_t doStatus(JsonObject& json);
	void onCommand(AlarmHandleCommand fn){_handleCommand = fn;};
	void waitDTMF(unsigned long timeout = 10000);
};

extern AlarmClass Alarm;
extern void handleInterrupt();
extern bool /*ICACHE_RAM_ATTR*/ debounce(uint8_t pin);