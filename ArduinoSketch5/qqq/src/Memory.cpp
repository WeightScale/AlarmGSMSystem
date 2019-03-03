#include "Memory.h"

void MemoryClass::init() {
	begin();
	get(0,eeprom);	
}

void MemoryClass::save() {
	put(0,eeprom);
}

MemoryClass Memory;