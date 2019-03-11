#include "PhoneBookClass.h"
#include "GsmModemClass.h"
#include <avr/wdt.h>

PhoneBookClass PhoneBook;

// default constructor
PhoneBookClass::PhoneBookClass(){
} //PhoneBookClass

// default destructor

PhoneBookClass::~PhoneBookClass(){
} //~PhoneBookClass

void PhoneBookClass::init(){
	while (GsmModem.sendATCommand(F("AT+CPBS=\"ME\""), true).indexOf("OK") == -1) {};	/*Выбираем место хранения контактов*/
	_admin = getContactFromSIM(1);	//Admin
	_reserve = getContactFromSIM(2);	//Reserve
}

AlarmClient* PhoneBookClass::getContactFromSIM(unsigned char id){
	String sub = "";
	String str = GsmModem.sendATCommand("AT+CPBR="+String(id),true);
	wdt_reset();
	if (str.indexOf("OK")!=-1){
			int index = str.indexOf("+CPBR:");
			if(index == -1)
				return nullptr;				
			int id = str.substring(index + 7, str.indexOf(",")).toInt();
			str = str.substring(str.indexOf(",")+1);
			sub = str.substring(1, str.indexOf(",")-1);
			//AlarmClient c(id,sub);
			//Alarm.addClient(&c);
			return new AlarmClient(id,sub);
			//_contact.append(new AlarmClient(id,sub));
			//_contact.append(c);
		}
	return NULL;		
}

bool PhoneBookClass::addContactToSIM(AlarmClient * client,const String& name){
	String str = "AT+CPBW=";
	str += String(client->id()) + ",\"" + client->phone() + "\",129,\"" + name +"\"";
	if (GsmModem.sendATCommand(str,true).indexOf("OK")!=-1){
		return true;
	}
	return false;
}

bool PhoneBookClass::delContactFromSIMM(unsigned char id){
	String str = "AT+CPBW=" + String(id);
	if (GsmModem.sendATCommand(str,true).indexOf("OK")!=-1){
		return true;
	}
	return false;	
}

bool PhoneBookClass::addReserve(){
	_reserve = getContactFromSIM(2);
	if(_reserve)
		return true;
	return false;
}

bool PhoneBookClass::addAdmin(){
	_admin = getContactFromSIM(1);
	if(_admin)
		return true;
	return false;
}

void PhoneBookClass::textAll(const String &message) {
	if(!message) return;
	if (_admin->canSend()) {
		_admin->text(message);
	}
	if (_reserve->canSend()) {
		_reserve->text(message);
	}
	/*if (_contact.moveToStart())	{
		do{
			if (_contact.current()->canSend()) {
				_contact.current()->text(message);
			}
		}while(_contact.next());
	}*/
}

void PhoneBookClass::callAll() {
	if (_admin->canSend()) {
		_admin->call();
	}
	if (_reserve->canSend()) {
		_reserve->call();
	}
	/*if(_contact.moveToStart()){
		do{
			if (_contact.current()->canSend()) {
				_contact.current()->call();
			}
		}while(_contact.next());
	}*/
};

String PhoneBookClass::listClients(){
	String msg = "admin:" + _admin->phone() + " reserve:" + _reserve->phone();
	return msg;
	/*if(_contact.moveToStart()){
		do{
			msg += _contact.current()->phone() + ":";
		}while(_contact.next());
		return msg;
		//_curentClient->text(msg);
		//GsmModem.sendSMS(_curentClient->_phone.c_str(), msg.c_str());
	}*/
}

AlarmClient* PhoneBookClass::hashClient(const String& phone){
	if (phone.length() < 10){
		return nullptr;
	}
	
	if(_time_admin){
		return new AlarmClient(1,phone);
	}else if (phone.indexOf(_admin->phone())>-1) {
		return _admin;
	}else if (phone.indexOf(_reserve->phone())!=-1) {
		return _reserve;
	}
	return nullptr;
}
