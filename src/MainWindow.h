#pragma once

#include <QMainWindow>
#include <QFile>

#include "DriverSettingsDialog.h"
#include "InterfaceParamenetsDialog.h"

#define VERSION_NUMBER "1.0.0"

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

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static int initTmkEvent();
    static void sleepCurrentThread(const int ms);

signals:
    void startCycleSendProcessSignal();
    void cycleSendProcessFinish();
    void retranslateUiSignal();
    void connectionGuiSignal(const bool connected);
    void qMessageBoxNeedShowSignal(const QString& message);

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
    void driverSettingsDialogOpenActionSlot();
    void setDriverSettingsSlot(const DriverSettingsDialog::DriverSettingsStruct newDriverSettings);
    void interfaceSettingsDialogOpenActionSlot();
    void setInterfaceSettingsSlot();
    void serialMonitorOpenActionSlot();
    void switchToEnglish();
    void switchToRussian();
    void aboutProgramActionSlot();
    void connectionGuiSlot(const bool connected);

public:
    void closeWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);

private slots:
    void on_connectionButton_clicked();

private:
    Ui::MainWindow* ui;
    DriverSettingsDialog* m_driverSettingsDialog;
    DriverSettingsDialog::DriverSettingsStruct m_currentDriverSettings;
    InterfaceParamenetsDialog* m_interfaceSettingsDialog;
    SerialMonitorWindow* m_serialMonitorWindow;
    QTranslator* m_translator;

    QThread *cycleSendOperationThread;
    bool cycleSendIsActive;
    QStringList statusList;
    QStringList cycleSendButtonNameList;
    QString fileName;
    QFile fileCycleSendLogs;

    /*!
     * \brief Флаг состояния модуля сопряжения (вкл / выкл)
     */
    bool m_isMpiStarted;
    QList<QString> m_connectionStatusVariants {tr("Готов"), tr("Не готов"), tr("Ready"), tr("Not ready")};

    /*!
     * \brief Метод активации модуля сопряжения
     * \param deviceNumber - числовой номер, назначаемый модулю
     * \param answerWaitTimeout - время ожидания ответного слова ОУ
     * \param memBaseNumber - номер базы ДОЗУ
     * \details Вернет true или false в зависимости от того, удалось ли активировать модуль сопряжения с указанными параметрами
     */
    bool startMpi(quint64 deviceNumber, quint64 answerWaitTimeout, quint64 memBaseNumber);
    /*!
     * \brief Метод деактивации модуля сопряжения
     */
    void stopMpi();
    void qMessageBoxNeedShowSlot(const QString& message);
};
