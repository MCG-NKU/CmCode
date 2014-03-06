#include "stdafx.h"
#include "SREngine.h"


HRESULT SREngine::InitializeSapi(HWND hWnd, UINT Msg, const WCHAR *xmlFileName)
{
	HRESULT hr = S_OK;
	const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_PHRASE_START) | 
		SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) | 
		SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_OTHER_CONTEXT) | SPFEI(SPEI_REQUEST_UI) | 
		SPFEI(SPEI_RECO_STATE_CHANGE) | SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE);
	V_RETURN(m_cpRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer));
	V_RETURN(SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &m_cpAudio));
	V_RETURN(m_cpRecognizer ->SetInput(m_cpAudio, TRUE));  
	V_RETURN(m_cpRecognizer->CreateRecoContext(&m_cpRecoContext));  
	V_RETURN(m_cpRecoContext->SetNotifyWindowMessage(hWnd, Msg, 0, 0));
	V_RETURN(m_cpRecoContext->SetInterest(ullInterest, ullInterest)); 
	if (xmlFileName != NULL)
		return LoadCmdFromFile(xmlFileName);
	return hr;
}

HRESULT SREngine::LoadCmdFromFile(const WCHAR *xmlFileName)
{
	HRESULT hr = S_OK;
	if (m_cpCmdGrammar != NULL)
		return hr;
	V_RETURN(m_cpRecoContext ->CreateGrammar(MYGRAMMARID, &m_cpCmdGrammar));  //Command and control---C&C
	V_RETURN(m_cpCmdGrammar->LoadCmdFromFile(xmlFileName, SPLO_DYNAMIC));
	return hr;
}

HRESULT SREngine::SetRuleState(const WCHAR *pszRuleName, BOOL fActivate)
{
	return m_cpCmdGrammar ->SetRuleState(pszRuleName, NULL, fActivate ? SPRS_ACTIVE : SPRS_INACTIVE);
}
