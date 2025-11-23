#pragma once

#include <QMainWindow>
#include <QFile>

#include "DriverSettingsDialog.h"
#include "InterfaceParamenetsDialog.h"

#define TRY_CONNECT_DEVICE_BUTTON_STRING "подключиться к устройству"
#define TRY_DISCONNECT_DEVICE_BUTTON_STRING "отключиться от устройства"

class QWidget;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QTextEdit;
class QSpinBox;
class SerialMonitorWindow;
class QTranslator;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum DEVICE_MODE_enum{
        UNKNOW_DEVICE_MODE = 0,
        KK_DEVICE_MODE,
        OY_DEVICE_MODE,
        M_DEVICE_MODE
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static int initTmkEvent();
    static void sleepCurrentThread(int ms);

private:
    Ui::MainWindow* ui;
    DriverSettingsDialog* m_driverSettingsDialog;
    DriverSettingsDialog::DriverSettingsStruct m_currentDriverSettings;
    InterfaceParamenetsDialog* m_interfaceSettingsDialog;
    SerialMonitorWindow* m_serialMonitorWindow;
    QTranslator* m_translator;

    int deviceMode;
    QThread *cycleSendOperationThread;
    bool cycleSendIsActive;
    QStringList statusList;
    QStringList cycleSendButtonNameList;
    QString fileName;
    QFile fileCycleSendLogs;

signals:
    void startCycleSendProcessSignal();
    void cycleSendProcessFinish();
    void retranslateUiSignal();

public slots:
    void connectDriverButtonSlot();
    void disconnectDriverButtonSlot();
    void connectDeviceButtonSlot();
    void setWaitAnswerIntervalButtonSlot();
    void clickDeviceModeButtonsSlot();
    void singleSendButtonSlot();
    void selectBaseValueButtonSlot();
    void readDataFromSubAddrServentDeviceSlot();
    void cycleSendProcessButtonSlot();
    void cycleSendProcessHandlerSlot();

    void connectionUARTButtonSlot();
    void updateCOMListSlot(int index);
    void receivedDataSlot(QByteArray data);
    void sendByUartDataButtonSlot();
    void clearUARTDataTextEditButtonSlot();

    //Новый функционал:
    void openDriverSettingsDialogSlot();
    void setDriverSettingsSlot(const DriverSettingsDialog::DriverSettingsStruct newDriverSettings);
    void openInterfaceSettingsDialogSlot();
    void setInterfaceSettingsSlot();
    void openSerialMonitorWindowSlot();
    void switchToEnglish();
    void switchToRussian();

public:
    void closeWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
};
