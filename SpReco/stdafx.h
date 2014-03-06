#pragma once

#include <QtGui>
#include <QtWidgets/QMessageBox>
#pragma warning(disable:4805)

#include <string>
#include <xstring>
using namespace std;

#include "SREngine.h"


#ifdef _DEBUG
#define lnkLIB(name) name "d"
#else
#define lnkLIB(name) name
#endif
#pragma comment(lib, lnkLIB("qtmain"))
#pragma comment(lib, lnkLIB("Qt5Core"))
#pragma comment(lib, lnkLIB("Qt5Gui"))
#pragma comment(lib, lnkLIB("Qt5Widgets"))


#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( hr != S_OK ) { QMessageBox::information(NULL, __FILE__, QString("Error in file `%1' line %2\n").arg(__FILE__).arg(__LINE__), MB_OK); return hr; } }
#endif

#ifndef VERIFY_RES
#define VERIFY_RES(x)    {HRESULT hr = (x); if( hr != S_OK ) { QMessageBox::information(NULL, __FILE__, QString("Error in file `%1' line %2\n").arg(__FILE__).arg(__LINE__), MB_OK); } }
#endif
