#pragma once


#define STD_LOGGER std::unique_ptr<StdLogger> loger(new StdLogger(StdLogger::LOGDEVICE_CONSOLE, StdLogger::LOGDEVICE_CONSOLE));

class StdLogger
{
public:
	enum LogDevice{LOGDEVICE_CONSOLE, LOGDEVICE_FILE, LOGDEVICE_NONE};
	
	StdLogger(LogDevice deviceStdCout, LogDevice deviceStdCerr);
	
	~StdLogger();
};
