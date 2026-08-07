#pragma once

#include <QMainWindow>
#include <QFile>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "DriverSettingsDialog.h"
#include "InterfaceParamenetsDialog.h"

#define VERSION_NUMBER "1.0.0"
#define LOG_DATA_LINE_LIMIT 50000

class QWidget;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QTextEdit;
class QSpinBox;
class SerialMonitorWindow;
class QTranslator;
class UndoBlocker;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    typedef enum
    {
        Tx_MPI = 0,
        Rx_MPI
    } MpiOperationType;

signals:
    void startCycleSendProcessSignal();
    void cycleSendProcessFinish();
    void retranslateUiSignal();
    void connectionGuiSignal(const bool connected);
    void qMessageBoxNeedShowSignal(const QString& message);
    void putLogDataSignal(MpiOperationType operationType, const QString& logData);
    void setCycleSendingSignal(bool sendingState);
    void becauseCycleSendingGuiEnabledSignal(bool enable);
    void mpiWriteDataSignal();
    void driverSettingsUpdatedSignal();

public:
    void closeWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_connectionButton_clicked();
    void on_logWriteClearButton_clicked();
    void on_logReadClearButton_clicked();
    void on_decimalFormatCheckBox_stateChanged(int arg1);
    void on_hexFormatCheckBox_stateChanged(int arg1);
    void on_inputMpiWriteDataButton_clicked();
    void on_mpiReadWordsNumberButton_clicked();
    void mpiWriteDataSlot();
    void cycleSendingThreadSlot();
    void becauseCycleSendingGuiEnabledSlot(bool enable);
    void qMessageBoxNeedShowSlot(const QString& message);
    void driverSettingsDialogOpenActionSlot();
    void setDriverSettingsSlot(const DriverSettingsDialog::DriverSettingsStruct newDriverSettings);
    void interfaceSettingsDialogOpenActionSlot();
    void setInterfaceSettingsSlot();
    void serialMonitorOpenActionSlot();
    void aboutProgramActionSlot();
    void connectionGuiSlot(const bool connected);
    void putLogDataSlot(MpiOperationType operationType, const QString& logData);

private:
    Ui::MainWindow* ui;
    UndoBlocker* m_undoBlocker;
    DriverSettingsDialog* m_driverSettingsDialog;
    DriverSettingsDialog::DriverSettingsStruct m_currentDriverSettings;
    InterfaceParamenetsDialog* m_interfaceSettingsDialog;
    SerialMonitorWindow* m_serialMonitorWindow;
    QTranslator* m_translator;

    /*!
     * \brief Флаг состояния модуля сопряжения (вкл / выкл)
     */
    bool m_isMpiStarted = false;
    bool isCycleSendingActive = false;
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
    bool checkMpiLine();

    static int initTmkEvent();
    static void sleepCurrentThread(const int ms);
    void switchToEnglish();
    void switchToRussian();
};
