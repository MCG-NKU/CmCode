/********************************************************************************
** Form generated from reading UI file 'SpRecoUI.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPRECOUI_H
#define UI_SPRECOUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SpRecoUIClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *pbVoiceInput;
    QTextEdit *textEdit;

    void setupUi(QMainWindow *SpRecoUIClass)
    {
        if (SpRecoUIClass->objectName().isEmpty())
            SpRecoUIClass->setObjectName(QStringLiteral("SpRecoUIClass"));
        SpRecoUIClass->resize(600, 400);
        centralWidget = new QWidget(SpRecoUIClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        pbVoiceInput = new QPushButton(centralWidget);
        pbVoiceInput->setObjectName(QStringLiteral("pbVoiceInput"));

        horizontalLayout->addWidget(pbVoiceInput);


        verticalLayout->addLayout(horizontalLayout);

        textEdit = new QTextEdit(centralWidget);
        textEdit->setObjectName(QStringLiteral("textEdit"));

        verticalLayout->addWidget(textEdit);

        SpRecoUIClass->setCentralWidget(centralWidget);

        retranslateUi(SpRecoUIClass);

        QMetaObject::connectSlotsByName(SpRecoUIClass);
    } // setupUi

    void retranslateUi(QMainWindow *SpRecoUIClass)
    {
        SpRecoUIClass->setWindowTitle(QApplication::translate("SpRecoUIClass", "SpRecoUI", 0));
        pbVoiceInput->setText(QApplication::translate("SpRecoUIClass", "Press the button to input voice command", 0));
    } // retranslateUi

};

namespace Ui {
    class SpRecoUIClass: public Ui_SpRecoUIClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPRECOUI_H
