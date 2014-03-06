#include "stdafx.h"
#include "SpRecoUI.h"

#include <QtWidgets/QMessageBox>


SpRecoUI::SpRecoUI(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	connect(ui.pbVoiceInput, SIGNAL(pressed()), this, SLOT(onVoiceStart()));
	connect(ui.pbVoiceInput, SIGNAL(released()), this, SLOT(onVoiceStop()));	

	//SAPI init
	m_bSoundStart = false;
	m_bSoundEnd = false;	
	if(FAILED(m_SREngine.InitializeSapi((HWND)this->winId(), SREngine::WM_RECOEVENT, L"./SpeechGrammar.xml")))
		QMessageBox::information(NULL, "Error", "Initialize speech engine failed!", MB_OK);
}

SpRecoUI::~SpRecoUI()
{

}

void SpRecoUI::onVoiceStart()
{
	VERIFY_RES(m_SREngine.SetRuleState(NULL, TRUE));
	setWindowTitle("Sound started");
}

void SpRecoUI::onVoiceStop()
{
	VERIFY_RES(m_SREngine.SetRuleState(NULL, FALSE));
	setWindowTitle("Sound stopped");
}

bool SpRecoUI::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	MSG* pMsg = (MSG*) message;
	setWindowTitle("Control - Debug: winEvent");
	if(pMsg->message == SREngine::WM_RECOEVENT)
		*result = this->OnRecoEvent();

	return false;
}

// Speech Recognition Event Process
LRESULT SpRecoUI::OnRecoEvent()
{
	if (m_SREngine.m_cpRecoContext == NULL)
		return FALSE;

	CSpEvent spEvent;
	HRESULT hr = S_OK;
	while(spEvent.GetFrom(m_SREngine.m_cpRecoContext) == S_OK) {
		setWindowTitle("Control - Debug");
		switch(spEvent.eEventId){
		case SPEI_SOUND_START: m_bSoundStart = true; break;
		case SPEI_SOUND_END: m_bSoundEnd = true; break;
		case SPEI_RECOGNITION: if (m_bSoundStart && m_bSoundEnd) Recognized(spEvent); break;
		}
	}
	return TRUE;
}

void SpRecoUI::Recognized(CSpEvent &spEvent)
{
	USES_CONVERSION;
	CComPtr<ISpRecoResult> cpResult = spEvent.RecoResult();
	CSpDynamicString dstrText;
	cpResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
	QString strResult = dstrText.CopyToChar(); //  W2T(dstrText);
	SPPHRASE* pPhrase = NULL;
	if (SUCCEEDED(cpResult->GetPhrase(&pPhrase))){
		strResult += tr(" RuleName:") + QString::fromStdWString(pPhrase->Rule.pszName);
		strResult += tr(" PropName:") + QString::fromStdWString(pPhrase->pProperties->pszName);		
		if (pPhrase->pProperties->pNextSibling)
			strResult += tr(" Sibling:") + QString::fromStdWString(pPhrase->pProperties->pNextSibling->pszName);
		if (pPhrase->pProperties->pFirstChild)
			strResult += tr(" Child:") + QString::fromStdWString(pPhrase->pProperties->pFirstChild->pszName);
	}

	if (pPhrase)
		::CoTaskMemFree(pPhrase);	
	ui.textEdit->insertPlainText(strResult+"\n");
}