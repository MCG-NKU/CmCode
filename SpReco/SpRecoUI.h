#ifndef SPRECOUI_H
#define SPRECOUI_H

#include "ui_SpRecoUI.h"


class SpRecoUI : public QMainWindow
{
	Q_OBJECT

public:
	SpRecoUI(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~SpRecoUI();

	LRESULT OnRecoEvent();


	bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private slots:
	void onVoiceStart();
	void onVoiceStop();

private:
	Ui::SpRecoUIClass ui;

	// For speech recognition
	bool m_bSoundEnd, m_bSoundStart;
	SREngine m_SREngine;

	void Recognized(CSpEvent &spEvent);
};

#endif // SPRECOUI_H
