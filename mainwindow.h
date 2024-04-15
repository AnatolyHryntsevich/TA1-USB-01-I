#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QSpinBox>
#include <QDebug>
#include <QThread>
#include <QCloseEvent>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QSerialPortInfo>

#include <windows.h>
#include <conio.h>

#include <iostream>
#include <string>
#include <sstream>

#include "uarttransfer.h"

#define TRY_CONNECT_DEVICE_BUTTON_STRING "подключиться к устройству"
#define TRY_DISCONNECT_DEVICE_BUTTON_STRING "отключиться от устройства"

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

    QWidget *connectionUARTWidget;
    QGridLayout *connectioinUARTLayout;
    QLabel *baudRatesBoxTitle;
    QComboBox *baudRatesBox;
    QLabel *serialPortsBoxTitle;
    QComboBox *serialPortsBox;
    QList<QString> connectionStatusVariants;
    QLabel *connectionStatusLabel;
    QPushButton *connectButton;
    QTextEdit *receivedTransmittedUARTDataTextEdit;
    QTextEdit *lineDForTransmittedUARTDataTextEdit;
    QPushButton *sendUARTDataButton;

    QLabel *mainWindowTitle;
    QPushButton *connectionDriverButton;
    QPushButton *disconnectionDriverButton;
    QLabel *connectResultText;
    QLabel *devicesNumbersTitleLabel;
    QPushButton *connectionDeviceButton;
    QComboBox *devicesNumbersListBox;
    QLabel *waitAnswerIntervalValueBoxTitleLabel;
    QComboBox *waitAnswerIntervalValueBox;
    QPushButton *setWaitAnswerIntervalButton;
    QLabel *selectModeTitleLabel;
    QPushButton *bcModeSelectButton;
    QPushButton *rtModeSelectButton;
    QPushButton *mtModeSelectButton;
    QLabel *selectBaseForWorkTitleLabel;
    QSpinBox *baseForWorkValueBox;
    QPushButton *selectBaseForWorkButton;
    QLabel *inputYourMessageTitleLabel;
    QLabel *addrOYTitleLabel;
    QSpinBox *addrYOValueBox;
    QLabel *subAddrOYTitleLabel;
    QSpinBox *subAddrYOValueBox;
    QTextEdit *lineSentMessageTextEdit;
    QLabel *lastSendDescriptionTitleLabel;
    QTextEdit *lastSendDescriptionTextEdit;
    QPushButton *sendButton;
    QLabel *sendStatusLabel;
    QLabel *cycleSendTitleLabel;
    QPushButton *cycleSendButton;
    QLabel *cycleSendIntervalValuesBoxTitleLabel;
    QSpinBox *cycleSendIntervalValueBox;
    QLabel *cycleSendStatusLabel;
    QLabel *readDataFromSubaddrTitleLabel;
    QLabel *dataWordNumberLabel;
    QSpinBox *dataWordValueBox;
    QPushButton *readDataFromSubaddrButton;
    QLabel *readStatusLabel;
    QTextEdit *readDataTextEdit;
    QGridLayout *MIL_STD_WidgetLayout;

    static int initTmkEvent();
    static void sleepCurrentThread(int ms);

private:
    int deviceMode;
    QThread *cycleSendOperationThread;
    bool cycleSendIsActive;
    QStringList statusList;
    QStringList cycleSendButtonNameList;
    QString fileName;
    QFile fileCycleSendLogs;
    UartTransfer *uartTransfer;

signals:
    void startCycleSendProcessSignal();
    void cycleSendProcessFinish();

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
    void connectionButtonSlot();
    void updateCOMListSlot(int index);
    void receivedDataSlot(QByteArray data);

public:
    void closeWindow();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
};
#endif // MAINWINDOW_H
