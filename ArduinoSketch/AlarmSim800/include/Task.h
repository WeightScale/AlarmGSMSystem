﻿#pragma once
#include <Arduino.h>

// #define USE_TASK_NAMES	1

class Task{
	
	typedef void(*TaskFunction)(void);
	
protected:
	/*! Интервал между запусками */
	unsigned long interval;

	/*! Последнее время запуска в милисекундах */
	unsigned long last_run;

	/*! Запланированый пробег в милисекундах */	
	unsigned long _cached_next_run;
	
	bool _paused = false;

	/*!
		IMPORTANT! Run after all calls to run()
		Updates last_run and cache next run.
		NOTE: This MUST be called if extending
		this class and implementing run() method
	*/
	void runned(unsigned long time);

	// Default is to mark it runned "now"
	void runned() { runned(millis()); }

	// Callback for run() if not implemented
	TaskFunction _onRun;		

public:

	// If the current Tasks is enabled or not
	bool enabled;

	// ID of the Tasks (initialized from memory adr.)
	int TaskID;

	#ifdef USE_TASK_NAMES
		// Tasks Name (used for better UI).
		String TaskName;			
	#endif
	Task();
	Task(unsigned long _interval = 0);
	Task(void (*callback)(void) = NULL, unsigned long _interval = 0);

	// Set the desired interval for calls, and update _cached_next_run
	virtual void setInterval(unsigned long _interval);

	// Return if the Task should be runned or not
	virtual bool shouldRun(unsigned long time);

	// Default is to check whether it should run "now"
	bool shouldRun() { return shouldRun(millis()); }

	// Callback set
	void onRun(TaskFunction callback){_onRun = callback;};

	/// Запуск
	virtual void run();
	
	void resume(bool forse = false){
		if (!forse)
			runned();
		else
			_cached_next_run = 0;			
		_paused = false; 
		};
	void pause(){_paused = true;};
	void updateCache(){runned();}
};