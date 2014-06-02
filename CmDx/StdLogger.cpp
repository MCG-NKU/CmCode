#include "StdAfx.h"
#include "StdLogger.h"
#include <io.h>
#include "FCNTL.H"
#include <ios>
#include <iostream>
#include <fstream>

StdLogger::StdLogger(LogDevice deviceStdCout, LogDevice deviceStdCerr)
{
	FILE *outputHandle = NULL;
	AllocConsole();

	/*Adding Console I/O to a Win32 GUI App, Windows Developer Journal, December 1997
	See Andrew Tucker's Home Page (http://www.halcyon.com/~ast/dload/guicon.htm)*/

	// Redirect std::cout
	if(deviceStdCout == LOGDEVICE_CONSOLE){
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		int output = _open_osfhandle(intptr_t(console), _O_TEXT);
		FILE *outputHandle = _fdopen(output, "w");
		*stdout = *outputHandle;
		setvbuf(outputHandle, NULL, _IONBF, 0);
	}
	else{
		if(outputHandle == NULL)
			outputHandle = fopen("stdlog.txt", "w");
		*stdout = *outputHandle;
	}

	// Redirect std::cerr
	if(deviceStdCerr == LOGDEVICE_CONSOLE){
		HANDLE console = GetStdHandle(STD_ERROR_HANDLE);
		int output = _open_osfhandle(intptr_t(console), _O_TEXT);
		FILE *outputHandle = _fdopen(output, "w");
		*stderr = *outputHandle;
		setvbuf(outputHandle, NULL, _IONBF, 0);
	}
	else{
		if(outputHandle == NULL)
			outputHandle = fopen("stdlog.txt", "w");
		*stderr = *outputHandle;
	}
	std::ios::sync_with_stdio(true);
}


StdLogger::~StdLogger() 
{
	FreeConsole();
}