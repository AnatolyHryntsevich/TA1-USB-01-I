#pragma once

#include <QDialog>

#include "InterfaceParamenetsDialog.h"

namespace Ui {
class DriverSettingsDialog;
}

class DriverSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DriverSettingsDialog(QWidget *parent = nullptr);
    ~DriverSettingsDialog();

    typedef struct {
        int deviceNumber;
        int memBaseNumber;
        int answerWaitTimeout;
        QString workModeName;
    } DriverSettingsStruct;

    void reloadSettingsUiBeforeView();
    void setGuiSate(const bool factor);

    const DriverSettingsStruct &currentDriverSettings() const;

signals:
    void setDriverSettingsSignal(const DriverSettingsStruct newDriverSettings);

public slots:
    void retranslateUiSlot();

private slots:
    void on_okButton_clicked();

private:
    Ui::DriverSettingsDialog *ui;
    DriverSettingsStruct m_currentDriverSettings;
};
