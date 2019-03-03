#include "Battery.h"
#include "Memory.h"

BatteryClass* BATTERY;

BatteryClass::BatteryClass() : Task(60000) {
	/* 20 Обновляем заряд батареи */
	onRun([](){
		BATTERY->fetchCharge();
	});
	_settings = &Memory.eeprom.settings;
	_max = MAX_CHG;//_settings->bat_max;
	_min = MIN_CHG;//_settings->bat_min;
	analogReference(INTERNAL);
	_charge = _get_adc(1);
#ifdef DEBUG_BATTERY
	_isDischarged = false;
#endif // DEBUG_BATTERY

}

int BatteryClass::fetchCharge() {
	_charge = _get_adc(1);
	_charge = constrain(_charge, _min, _max);
	_charge = map(_charge, _min, _max, 0, 100);
	_isDischarged = _charge <= 5;
	if (_isDischarged) {
		if (_handleDischarged)
			_handleDischarged(_charge);
		//ws.textAll("{\"cmd\":\"dchg\"}");
	}
	return _charge;
}

int BatteryClass::_get_adc(byte times) {
	long sum = 0;
#ifdef DEBUG_BATTERY
	for (byte i = 0; i < times; i++) {
		sum += random(ADC);
	}	
#else
	for (byte i = 0; i < times; i++) {
		sum += analogRead(CHANEL);
	}
#endif // DEBUG_BATTERY	
	return times == 0 ? sum : sum / times;	
}