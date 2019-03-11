#pragma once
#include <Vector.h>
#include "Alarm.h"
#include "Task.h"

class PhoneBookClass{
//variables
public:
protected:
private:
	AlarmClient* _admin;
	AlarmClient* _reserve;
	bool _time_admin = true;
	//LinkedList<AlarmClient *> _contact;
//functions
public:
	PhoneBookClass();
	~PhoneBookClass();
	void init();
	AlarmClient* getContactFromSIM(unsigned char id);
	bool addContactToSIM(AlarmClient * client, const String& name);
	bool delContactFromSIMM(unsigned char id);
	bool addReserve();
	bool addAdmin();
	AlarmClient* admin() {return _admin;};
	AlarmClient* reserve() {return _reserve;};	
	void textAll(const char * message, size_t len);
	void textAll(const String &message);
	void callAll();
	String listClients();
	void time_admin(bool t){_time_admin = t;};
	bool time_admin(){return _time_admin;};
	//unsigned char length(){return _contact.length();};
	AlarmClient* hashClient(const String& phone);
protected:
private:
	PhoneBookClass( const PhoneBookClass &c );
	PhoneBookClass& operator=( const PhoneBookClass &c );
	
	

}; //PhoneBookClass

extern PhoneBookClass PhoneBook;