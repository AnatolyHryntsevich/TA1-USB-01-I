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

    struct DriverSettingsStruct {
        int deviceNumber;
        int memBaseNumber;
        int answerWaitTimeout;
        QString workModeName;

        DriverSettingsStruct& operator=(const DriverSettingsStruct& other)
        {
            if (this != &other)
            {
                deviceNumber = other.deviceNumber;
                memBaseNumber = other.memBaseNumber;
                answerWaitTimeout = other.answerWaitTimeout;
                workModeName = other.workModeName;
            }
            return *this;
        }

        bool operator==(const DriverSettingsStruct& other) const
        {
            return deviceNumber == other.deviceNumber &&
                   memBaseNumber == other.memBaseNumber &&
                   answerWaitTimeout == other.answerWaitTimeout &&
                   workModeName == other.workModeName;
        }
    };

    void reloadSettingsUiBeforeView();
    void setGuiSate(const bool factor);

    const DriverSettingsStruct &currentDriverSettings() const;

signals:
    void setDriverSettingsSignal(const DriverSettingsStruct newDriverSettings);

public slots:
    void retranslateUiSlot();
    void driverSettingsUpdatedSlot();

private slots:
    void on_okButton_clicked();

private:
    Ui::DriverSettingsDialog *ui;
    DriverSettingsStruct m_currentDriverSettings;
};
