#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#define USER_MAX 5

typedef struct {	
	int bat_max;
	int bat_min;
} settings_t;

typedef struct {	
	char name[16];
	char phone[14];
	bool send = false;
	bool root = false;
}user_t;

typedef struct {
	user_t users[USER_MAX];
	settings_t settings;	
}eeprom_t;

class MemoryClass : protected EEPROMClass {
public:
	eeprom_t eeprom;

public:
	MemoryClass(){};
	~MemoryClass() {};
	void init();
	void save();	
};

extern MemoryClass Memory;