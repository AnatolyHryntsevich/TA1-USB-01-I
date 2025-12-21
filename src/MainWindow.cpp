#include "MainWindow.h"

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
#include <QPlainTextEdit>
#include <QTranslator>
#include <QSettings>
#include <QShortcut>
#include <QScrollBar>
#include <QtConcurrent>

#include <iostream>
#include <string>
#include <sstream>

#ifdef __unix__
extern "C" {
#include "ltmk.h"
}
extern int tmkError;
int events;
int hTmk;
#endif

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include "WDMTMKv2.cpp" //в хедере не размещать, дабы не нарваться на multiple definition
HANDLE hBcEvent;
#endif

#include "ui_MainWindow.h"

#include "SerialMonitorWindow.h"
#include "UndoBlocker.h"

TTmkEventData tmkEvD;
unsigned short awBuf[32]; //в linux размерность может равняться 64
TMK_DATA wBase, wMaxBase, wAddr, wSubAddr, wLen, wState, wStatus;
unsigned long dwGoodStarts = 0, dwBusyStarts = 0, dwErrStarts = 0, dwStatStarts = 0;
unsigned long dwStarts = 0L;

#define RT_ADDR wAddr /* RT address */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_driverSettingsDialog(new DriverSettingsDialog(this)),
      m_interfaceSettingsDialog(new InterfaceParamenetsDialog(this)),
      m_serialMonitorWindow(new SerialMonitorWindow()),
      m_translator(nullptr),
      m_undoBlocker(new UndoBlocker(this))
{
    ui->setupUi(this);

    m_currentDriverSettings = m_driverSettingsDialog->currentDriverSettings();
    on_hexFormatCheckBox_stateChanged(true);
    ui->inputMpiWriteDataLineEdit->installEventFilter(m_undoBlocker);
//    connectionGuiSlot(false);

    connect(ui->driverSettingsDialogOpenAction, &QAction::triggered, this, &MainWindow::driverSettingsDialogOpenActionSlot);
    connect(m_driverSettingsDialog, &DriverSettingsDialog::setDriverSettingsSignal, this, &MainWindow::setDriverSettingsSlot);
    connect(ui->interfaceSettingsDialogOpenAction, &QAction::triggered, this, &MainWindow::interfaceSettingsDialogOpenActionSlot);
    connect(m_interfaceSettingsDialog, &InterfaceParamenetsDialog::setInterfaceSettingsSignal, this, &MainWindow::setInterfaceSettingsSlot);
    connect(this, &MainWindow::retranslateUiSignal, m_driverSettingsDialog, &DriverSettingsDialog::retranslateUiSlot);
    connect(this, &MainWindow::retranslateUiSignal, m_interfaceSettingsDialog, &InterfaceParamenetsDialog::retranslateUiSlot);
    connect(this, &MainWindow::retranslateUiSignal, m_serialMonitorWindow, &SerialMonitorWindow::retranslateUiSlot);
    connect(ui->serialMonitorOpenAction, &QAction::triggered, this, &MainWindow::serialMonitorOpenActionSlot);
    connect(ui->aboutProgramAction, &QAction::triggered, this, &MainWindow::aboutProgramActionSlot);

    connect(this, &MainWindow::qMessageBoxNeedShowSignal, this, &MainWindow::qMessageBoxNeedShowSlot);
    connect(this, &MainWindow::connectionGuiSignal, this, &MainWindow::connectionGuiSlot);
    connect(this, &MainWindow::putLogDataSignal, this, &MainWindow::putLogDataSlot);

    connect(this, &MainWindow::setCycleSendingSignal, this, &MainWindow::cycleSendingThreadSlot);
    connect(this, &MainWindow::becauseCycleSendingGuiEnabledSignal, this, &MainWindow::becauseCycleSendingGuiEnabledSlot);
    connect(this, &MainWindow::mpiWriteDataSignal, this, &MainWindow::mpiWriteDataSlot);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_driverSettingsDialog;
    delete m_interfaceSettingsDialog;
    delete m_serialMonitorWindow;
    delete m_translator;
    delete m_undoBlocker;
}

int MainWindow::initTmkEvent()
{
#ifdef _WIN32
    hBcEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hBcEvent)
    {
        qDebug() << "Не удалось создать tmkEvent...";
        return -1;
    }
    ResetEvent(hBcEvent);
    tmkdefevent(hBcEvent, TRUE);
#endif
    return 0;
}

void MainWindow::sleepCurrentThread(const int ms)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(ms);
    loop.exec();
}

static int WaitInt(TMK_DATA wCtrlCode)
{
    /* Wait for an interrupt */
#ifdef _WIN32
    switch (WaitForSingleObject(hBcEvent, 1000))
    {
    case WAIT_OBJECT_0:
        ResetEvent(hBcEvent);
        break;
    case WAIT_TIMEOUT:
        qDebug() << "Interrupt timeout error\n";
        return 1;
    default:
        qDebug() << "Interrupt wait error\n";
        return 1;
    }
#elif defined __unix__
    events = tmkwaitevents(1<<hTmk, 1000);
    if (events < 0)
        printf("Error occured during interrupt waiting!\n");
    else if (events == 0)
        printf("We didn't get interrupt!\n");
    else if (events == (1<<hTmk))
        printf("We got interrupt!\n");
    else
        printf("We got very strange interrupt!\n");
#endif

    tmkgetevd(&tmkEvD);

    if (tmkEvD.bcx.wResultX & SX_IB_MASK)
    {
        if (((tmkEvD.bcx.wResultX & SX_ERR_MASK) == SX_NOERR) ||
                ((tmkEvD.bcx.wResultX & SX_ERR_MASK) == SX_TOD))
        {
            wStatus = bcgetansw(wCtrlCode);
            if (wStatus & BUSY_MASK)
                ++dwBusyStarts;
            else
                ++dwStatStarts;
        }
        else
        {
            ++dwErrStarts;
        }
    }
    else if (tmkEvD.bcx.wResultX & SX_ERR_MASK)
    {
        ++dwErrStarts;
    }
    else
    {
        ++dwGoodStarts;
    }

    if (dwStarts%1000L == 0L)
    {
        qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
    }

    ++dwStarts;

    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox ms(this);
    QPushButton *yesButton;
    QPushButton *noButton;
    ms.setWindowTitle(tr("Подтверждение закрытия"));
    ms.setText(tr("Вы уверены, что хотите закрыть программу?"));
    ms.setIcon(QMessageBox::Question);
    yesButton = ms.addButton(tr("Да"), QMessageBox::YesRole);
    noButton = ms.addButton(tr("Нет"), QMessageBox::NoRole);
    ms.setDefaultButton(noButton);
    ms.setEscapeButton(noButton);

    ms.exec();

    if (ms.clickedButton() == yesButton)
    {
        if (m_serialMonitorWindow)
        {
            m_serialMonitorWindow->close();
        }
        isCycleSendingActive = false;
        emit setCycleSendingSignal(isCycleSendingActive);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    event->ignore();
}

void MainWindow::closeWindow()
{
    QCloseEvent *event = new QCloseEvent();
    this->closeEvent(event);
}

void MainWindow::driverSettingsDialogOpenActionSlot()
{
    if(!m_driverSettingsDialog)
        return;

    m_driverSettingsDialog->reloadSettingsUiBeforeView();

    if (!m_driverSettingsDialog->isVisible()) {
        m_driverSettingsDialog->setVisible(true);
    }
}

void MainWindow::setDriverSettingsSlot(const DriverSettingsDialog::DriverSettingsStruct newDriverSettings)
{
    m_currentDriverSettings = newDriverSettings;
    m_driverSettingsDialog->setVisible(false);
}

void MainWindow::interfaceSettingsDialogOpenActionSlot()
{ 
    if(!m_interfaceSettingsDialog)
        return;

    m_interfaceSettingsDialog->reloadSettingsUiBeforeView();

    if (!m_interfaceSettingsDialog->isVisible()) {
        m_interfaceSettingsDialog->setVisible(true);
    }
}

void MainWindow::setInterfaceSettingsSlot()
{
    m_interfaceSettingsDialog->setVisible(false);

    static QString originalStyleSheet = qApp->styleSheet();

    QString newStyleSheet = originalStyleSheet + QString(
                "\n"
                "QLabel, QPushButton, QComboBox, QCheckBox, "
                "QRadioButton, QGroupBox, QTabWidget, QMenu, "
                "QMenuBar, QToolBar {"
                "    font-size: %1pt;"
                "}"
                ).arg(m_interfaceSettingsDialog->getCurrentInterfaceSettings().fontSize);

    qApp->setStyleSheet(newStyleSheet);

    if(m_interfaceSettingsDialog->getCurrentInterfaceSettings().language
            == InterfaceParamenetsDialog::LanguageEnum::English_language)
    {
        switchToEnglish();
    }
    else if (m_interfaceSettingsDialog->getCurrentInterfaceSettings().language
             == InterfaceParamenetsDialog::LanguageEnum::Russian_language)
    {
        switchToRussian();
    }
}

void MainWindow::serialMonitorOpenActionSlot()
{
    if(!m_serialMonitorWindow)
        return;

    if (m_serialMonitorWindow->isMinimized()) {
        m_serialMonitorWindow->showNormal();
    }

    if (!m_serialMonitorWindow->isVisible()) {
        m_serialMonitorWindow->updatePortNameListSlot(0);
        m_serialMonitorWindow->setVisible(true);
    }

    m_serialMonitorWindow->raise();
    m_serialMonitorWindow->activateWindow();
}

void MainWindow::switchToEnglish()
{
    if (m_translator)
    {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    m_translator = new QTranslator;

    if (m_translator->load("translations/app_en.qm") ||
            m_translator->load("app_en.qm") ||
            m_translator->load("../translations/app_en.qm"))
    {
        qApp->installTranslator(m_translator);
        QSettings settings;
        settings.setValue("language", "en");
        qDebug() << "Switched to English";
        ui->retranslateUi(this);
        emit retranslateUiSignal();
    }
    else
    {
        qWarning() << "Failed to load English translation";
        delete m_translator;
        m_translator = nullptr;
    }
}

void MainWindow::switchToRussian()
{
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
    }

    m_translator = new QTranslator;

    if (m_translator->load("translations/app_ru.qm") ||
            m_translator->load("app_ru.qm") ||
            m_translator->load("../translations/app_ru.qm")) {

        qApp->installTranslator(m_translator);
        QSettings settings;
        settings.setValue("language", "ru");
        qDebug() << "Switched to Russian";
        ui->retranslateUi(this);
        emit retranslateUiSignal();
    }
    else
    {
        qWarning() << "Failed to load Russian translation";
        delete m_translator;
        m_translator = nullptr;
    }
}

void MainWindow::aboutProgramActionSlot()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(tr("О программе"));
    aboutBox.setIconPixmap(QPixmap(":resources/icons/app.ico").scaled(64, 64));

    aboutBox.setText(tr(
                         "<h3>mil-std-1553b-usb-terminal</h3>"
                         "<p><b>Программа управления шиной MIL-STD-1553 (ГОСТ Р 52070-2003) через USB-интерфейс</b></p>"
                         "<p>Предназначена для отладки и тестирования ЭВМ, работающих в сети MIL-STD-1553. Является реализацией API "
                         "драйвера модуля сопряжения с шиной MIL-STD-1553 TA1-USB производства АО «Элкус» "
                         "(<a href='http://www.elcus.ru'>http://www.elcus.ru/boards.php</a>)</p>"
                         "<hr>"
                         "<p><b>Возможности:</b></p>"
                         "<ul>"
                         "<li>Сопряжение USB с резервированным мультиплексным каналом "
                         "посредством модуля TA1-USB в режиме КК (Контроллер канала)</li>"
                         "<li>Запись данных в подадреса ОУ (Оконечных устройств)</li>"
                         "<li>Чтение данных из подадресов ОУ</li>"
                         "<li>Отображение лога работы</li>"
                         "</ul>"
                         "<p><b>Описание модуля на сайте производителя:</b> "
                         "<a href='http://www.elcus.ru/boards.php?ID=ta1-usb'>http://www.elcus.ru/boards.php?ID=ta1-usb</a></p>"
                         "<p><b>Версия программы:</b> %1</p>"
                         "<b>Дата сборки:</b> %2</p>"
                         ).arg(VERSION_NUMBER).arg(__DATE__));

    aboutBox.exec();
}

void MainWindow::connectionGuiSlot(const bool connected)
{
    ui->mpiWorkGroupBox->setEnabled(connected);
    m_driverSettingsDialog->setGuiSate(!connected);

    if(connected)
    {
        if(m_interfaceSettingsDialog->getCurrentInterfaceSettings().language ==
                InterfaceParamenetsDialog::Russian_language)
        {
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(0));
            ui->connectionButton->setText(tr("Деактивировать"));
            ui->connectionButton->setToolTip(tr("Деактивировать модуль сопряжения"));
        }
        else if (m_interfaceSettingsDialog->getCurrentInterfaceSettings().language ==
                 InterfaceParamenetsDialog::English_language)
        {
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(2));
            ui->connectionButton->setText(tr("Deactivate"));
            ui->connectionButton->setToolTip(tr("Deactivate the coupling module"));
        }
        ui->connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
    }
    else
    {
        if(m_interfaceSettingsDialog->getCurrentInterfaceSettings().language ==
                InterfaceParamenetsDialog::Russian_language)
        {
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(1));
            ui->connectionButton->setText(tr("Активировать"));
            ui->connectionButton->setToolTip(tr("Активировать модуль сопряжения"));
        }
        else if (m_interfaceSettingsDialog->getCurrentInterfaceSettings().language ==
                 InterfaceParamenetsDialog::English_language)
        {
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(3));
            ui->connectionButton->setText(tr("Activate"));
            ui->connectionButton->setToolTip(tr("Activate the coupling module"));
        }
        ui->connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
    }
}

void MainWindow::putLogDataSlot(MpiOperationType operationType, const QString &logData)
{
    if(!logData.isEmpty()) {
        switch (operationType) {
        case Tx_MPI:
        {
            bool isBottom = ui->logWriteMpiViewTextEdit->verticalScrollBar()->value() ==
                    ui->logWriteMpiViewTextEdit->verticalScrollBar()->maximum();
            int savedScroll = isBottom ? -1 : ui->logWriteMpiViewTextEdit->verticalScrollBar()->value();
            ui->logWriteMpiViewTextEdit->append(logData);
            QStringList logDataList = ui->logWriteMpiViewTextEdit->toPlainText().split("\n");
            if(logDataList.count() > LOG_DATA_LINE_LIMIT) {
                logDataList = logDataList.mid(LOG_DATA_LINE_LIMIT / 2);
                ui->logWriteMpiViewTextEdit->clear();
                ui->logWriteMpiViewTextEdit->append(logDataList.join("\n"));
            }
            if (!isBottom) {
                ui->logWriteMpiViewTextEdit->verticalScrollBar()->setValue(savedScroll);
            }
        }
            break;
        case Rx_MPI:
        {
            bool isBottom = ui->logReadMpiViewTextEdit->verticalScrollBar()->value() ==
                    ui->logReadMpiViewTextEdit->verticalScrollBar()->maximum();
            int savedScroll = isBottom ? -1 : ui->logReadMpiViewTextEdit->verticalScrollBar()->value();
            ui->logReadMpiViewTextEdit->append(logData);
            QStringList logDataList = ui->logReadMpiViewTextEdit->toPlainText().split("\n");
            if(logDataList.count() > LOG_DATA_LINE_LIMIT) {
                logDataList = logDataList.mid(LOG_DATA_LINE_LIMIT / 2);
                ui->logReadMpiViewTextEdit->clear();
                ui->logReadMpiViewTextEdit->append(logDataList.join("\n"));
            }
            if (!isBottom) {
                ui->logReadMpiViewTextEdit->verticalScrollBar()->setValue(savedScroll);
            }
        }
            break;
        }
    }
}

void MainWindow::on_connectionButton_clicked()
{
    if(startMpi(m_driverSettingsDialog->currentDriverSettings().deviceNumber,
                m_driverSettingsDialog->currentDriverSettings().answerWaitTimeout,
                m_driverSettingsDialog->currentDriverSettings().memBaseNumber))
    {
        emit connectionGuiSignal(true);
        qDebug() << "MPI module is enabled";
        return;
    }

    qDebug() << "MPI module is disabled";
    stopMpi();
    emit connectionGuiSignal(false);
}

bool MainWindow::startMpi(quint64 deviceNumber, quint64 answerWaitTimeout, quint64 memBaseNumber)
{
    bool funcResult = false;
    if(!m_isMpiStarted)
    {
        if(!TmkOpen() && !tmkconfig(deviceNumber) && !initTmkEvent())
        {
            wMaxBase = bcgetmaxbase();
            tmktimeout(answerWaitTimeout);
            wMaxBase = bcgetmaxbase();
            wBase = memBaseNumber;
            bcreset();
            if(!bcdefbase(wBase))
            {
                funcResult = true;
                m_isMpiStarted = true;
                m_currentDriverSettings.deviceNumber = deviceNumber;
                m_currentDriverSettings.answerWaitTimeout = answerWaitTimeout;
                m_currentDriverSettings.memBaseNumber = memBaseNumber;
#ifdef __unix__
                hTmk = deviceNumber;
#endif
            }
        }
        else
        {
            funcResult = false;
            emit qMessageBoxNeedShowSignal(tr("Доступных модулей TA1-USB на хосте не обнаружено!"));
        }
    }

    return funcResult;
}

void MainWindow::stopMpi()
{
    if(m_isMpiStarted) {
        bcreset();
#ifdef _WIN32
        CloseHandle(hBcEvent);
#endif
        tmkdone(ALL_TMKS);
        TmkClose();
        m_isMpiStarted = false;
    }
}

void MainWindow::qMessageBoxNeedShowSlot(const QString &message)
{
    QMessageBox msgBox(this);
    msgBox.setText(message);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setWindowModality(Qt::WindowModality::WindowModal);
    msgBox.setWindowFlags (msgBox.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    msgBox.exec();
}

bool MainWindow::checkMpiLine()
{
    if(bcdefbus(BUS_A)) {
        if(bcdefbus(BUS_B)) {
            emit qMessageBoxNeedShowSignal(tr("Ни основную, ни резервную ЛПИ активировать не удалось..."
                                              "\nАктивируйте линию передачи и попытайтесь снова"));
            stopMpi();
            isCycleSendingActive = false;
            emit setCycleSendingSignal(isCycleSendingActive);
            return false;
        }
    }

    return true;
}

void MainWindow::mpiWriteDataSlot()
{
    if(checkMpiLine())
    {
        QStringList sentWordsStringList = ui->inputMpiWriteDataLineEdit->text().trimmed().split(" ");
        if(sentWordsStringList.count() > sizeof(awBuf) / sizeof(unsigned short))
        {
            emit qMessageBoxNeedShowSignal(tr("Размер записываемых в ОУ слов превышает установленный лимит\n"
                                              "[одной транзакцией не более 32-ух 16-битных слов]"));
            return;
        }
        wLen = 0;
        bool ok;
        QString hex16SentDataView;
        QString logLine = QDateTime::currentDateTime().toString("hh:mm:ss") + ";";
        memset(&awBuf, 0, sizeof(awBuf));
        if(ui->decimalFormatCheckBox->isChecked())
        {
            for(; wLen < sentWordsStringList.count(); ++wLen)
            {
                awBuf[wLen] = static_cast<unsigned short>(sentWordsStringList.at(wLen).trimmed().toInt(&ok));
                if(!ok)
                {
                    emit qMessageBoxNeedShowSignal(tr("Ошибка записи слов в ОУ!\n"
                                                      "Проверьте введенные данные и попробуйте снова..."));
                    return;
                }
            }
            logLine.append(ui->inputMpiWriteDataLineEdit->text().trimmed() + ";");
        }
        else if (ui->hexFormatCheckBox->isChecked())
        {
            for(; wLen < sentWordsStringList.count(); ++wLen)
            {
                awBuf[wLen] = static_cast<unsigned short>(sentWordsStringList.at(wLen).trimmed().toInt(&ok, 16));
                hex16SentDataView.append((wLen == 0 ? "0x" : " 0x") + sentWordsStringList.at(wLen).trimmed().toUpper());
                if(!ok)
                {
                    emit qMessageBoxNeedShowSignal(tr("Ошибка записи слов в ОУ!\n"
                                                      "Проверьте введенные данные и попробуйте снова..."));
                    return;
                }
            }
            logLine.append(hex16SentDataView + ";");
        }

        wAddr = ui->deviceAddrSpinBox->value();
        bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
        bcputblk(1, awBuf, wLen);

        bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
        if (WaitInt(DATA_BC_RT))
        {
            qWarning() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts)
                           + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
            logLine.append(tr("Ошибка;время ожидания ответного события драйвера TA1-USB истекло"));
            emit putLogDataSignal(Tx_MPI, logLine);
            return;
        }

        if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0)
        {
            logLine.append(tr("Ок"));
            qDebug() << logLine;
        }
        else
        {
            qWarning() << tmkError;
            logLine.append(tr("Ошибка;данных о приеме/отказе от приема не получено"));
        }

        emit putLogDataSignal(Tx_MPI, logLine);
    }
}

void MainWindow::cycleSendingThreadSlot()
{
    QtConcurrent::run([this]()
    {
        while (isCycleSendingActive)
        {
            emit mpiWriteDataSignal();
            sleepCurrentThread(ui->cycleWriteMpiIntervalValueSpinBox->value());
        }
    });
}

void MainWindow::becauseCycleSendingGuiEnabledSlot(bool enable)
{
    ui->mpiConnectionGroupBox->setEnabled(enable);
    ui->addrSettingsGroupBox->setEnabled(enable);
    ui->generalReadMpiGroupBox->setEnabled(enable);
    ui->cycleWriteMpiModeCheckBox->setEnabled(enable);
    ui->hexFormatCheckBox->setEnabled(enable);
    ui->decimalFormatCheckBox->setEnabled(enable);
    ui->cycleWriteMpiIntervalValueSpinBox->setEnabled(enable);
    enable == true ? ui->inputMpiWriteDataButton->setText(tr("Записать"))
                   : ui->inputMpiWriteDataButton->setText(tr("Остановить запись"));
}

void MainWindow::on_logWriteClearButton_clicked()
{
    ui->logWriteMpiViewTextEdit->clear();
}

void MainWindow::on_logReadClearButton_clicked()
{
    ui->logReadMpiViewTextEdit->clear();
}

void MainWindow::on_decimalFormatCheckBox_stateChanged(int arg1)
{
    if(arg1)
    {
        const QString DEC_NUMBER =
                "(6553[0-5]|"          // 65530-65535
                "655[0-2]\\d|"         // 65500-65529
                "65[0-4]\\d\\d|"       // 65000-65499
                "6[0-4]\\d{3}|"        // 60000-64999
                "[1-5]\\d{4}|"         // 10000-59999
                "\\d{1,4})";           // 0-9999

        QString pattern = QString("^%1( %1){0,31}$").arg(DEC_NUMBER);
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression(pattern), this);
        ui->inputMpiWriteDataLineEdit->setValidator(validator);
        ui->inputMpiWriteDataLineEdit->clear();
        ui->hexFormatCheckBox->setChecked(false);
    }
}

void MainWindow::on_hexFormatCheckBox_stateChanged(int arg1)
{
    if(arg1)
    {
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression("^([0-9A-Fa-f]{4}( [0-9A-Fa-f]{4}){0,31})?$"), this);
        ui->inputMpiWriteDataLineEdit->setValidator(validator);
        ui->inputMpiWriteDataLineEdit->clear();
        ui->decimalFormatCheckBox->setChecked(false);
    }
}

void MainWindow::on_inputMpiWriteDataButton_clicked()
{
    if(checkMpiLine())
    {
        if(ui->cycleWriteMpiModeCheckBox->isChecked() && !isCycleSendingActive)
        {
            isCycleSendingActive = true;
            emit becauseCycleSendingGuiEnabledSignal(false);
            emit setCycleSendingSignal(isCycleSendingActive);
        }
        else if(ui->cycleWriteMpiModeCheckBox->isChecked() && isCycleSendingActive)
        {
            emit becauseCycleSendingGuiEnabledSignal(true);
            isCycleSendingActive = false;
        }
        else
        {
            mpiWriteDataSlot();
        }
    }
}

void MainWindow::on_mpiReadWordsNumberButton_clicked()
{
    if(checkMpiLine())
    {
        wAddr = ui->deviceAddrSpinBox->value();
        wSubAddr = ui->subAddrSpinBox->value();
        wLen = ui->mpiReadWordsNumberSpinBox->value();
        memset(&awBuf, 0, sizeof(awBuf));

        QString logLine = QDateTime::currentDateTime().toString("hh:mm:ss") + ";";
        QString readDataView;

        bcputw(0, CW(RT_ADDR, RT_TRANSMIT, wSubAddr, wLen));
        bcstartx(wBase, DATA_RT_BC | CX_STOP | CX_BUS_A | CX_NOSIG);
        if (WaitInt(DATA_RT_BC))
        {
            qWarning() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
            logLine.append(tr(";Ошибка;время ожидания ответного события драйвера TA1-USB истекло"));
            emit putLogDataSignal(Rx_MPI, logLine);
            return;
        }

        if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0)
        {
            bcgetblk(2, awBuf, wLen);
            for (int i = 0; i < wLen; ++i)
            {
                if(i < wLen - 1)
                {
                    if(ui->decimalFormatCheckBox->isChecked())
                    {
                        readDataView.append(QString::number(awBuf[i]).toUpper() + " ");
                    }
                    else if (ui->hexFormatCheckBox->isChecked())
                    {
                        readDataView.append("0x" + QString::number(awBuf[i], 16).toUpper() + " ");
                    }

                }
                else
                {
                    if(ui->decimalFormatCheckBox->isChecked())
                    {
                        readDataView.append(QString::number(awBuf[i]).toUpper() + ";");
                    }
                    else if (ui->hexFormatCheckBox->isChecked())
                    {
                        readDataView.append("0x" + QString::number(awBuf[i], 16).toUpper() + ";");
                    }
                }

            }
            logLine.append(readDataView + tr("Ок"));
        }
        else
        {
            qWarning() << tmkError;
            logLine.append(tr(";Ошибка;данных о приеме/отказе от приема запроса на чтение не получено"));
        }

        emit putLogDataSignal(Rx_MPI, logLine);
    }
}

