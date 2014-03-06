#pragma once

#include <sphelper.h> // Microsoft Speech API

class SREngine
{
public:
	//speech variable
	CComPtr <ISpRecognizer> m_cpRecognizer;
	CComPtr <ISpRecoContext> m_cpRecoContext;
	CComPtr <ISpRecoGrammar> m_cpCmdGrammar;

	//audio variable
	CComPtr <ISpAudio> m_cpAudio;

	// Const values
	static const UINT WM_RECOEVENT = WM_USER+100;
	static const UINT MYGRAMMARID = 101;

public:
	HRESULT SetRuleState(const WCHAR * pszRuleName = NULL, BOOL fActivate = SPRS_ACTIVE);
	HRESULT LoadCmdFromFile(const WCHAR * xmlFileName);
	HRESULT InitializeSapi(HWND hWnd, UINT Msg = WM_RECOEVENT, const WCHAR *xmlFileName = NULL);
};
