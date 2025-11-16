#pragma once

#include <QDialog>

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
        int memoryNumber;
        int answerWaitTimeout;
        QString workModeName;
    } DriverSettingsStruct;

    void reloadSettingsBeforeView();

signals:
    void setDriverSettingsSignal(const DriverSettingsStruct newDriverSettings);

private slots:
    void on_okButton_clicked();

private:
    Ui::DriverSettingsDialog *ui;
    DriverSettingsStruct currentDriverSettings;
};
