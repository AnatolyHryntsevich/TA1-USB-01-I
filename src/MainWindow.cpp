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

    QDir currentDir;
    QString fileName = "cycleSendLogs.txt";
    QString filePath = currentDir.absoluteFilePath(fileName);
    fileCycleSendLogs.setFileName(filePath);

    m_currentDriverSettings = m_driverSettingsDialog->currentDriverSettings();
    on_hexFormatCheckBox_stateChanged(true);
    ui->inputMpiWriteDataLineEdit->installEventFilter(m_undoBlocker);
    //    connectionGuiSlot(true);

    //    //_______________________________________________________________________________________________________
    //    cycleSendOperationThread = new QThread();
    //    connect(cycleSendButton, SIGNAL(clicked()), this, SLOT(cycleSendProcessButtonSlot()));
    //    connect(this, SIGNAL(startCycleSendProcessSignal()), cycleSendOperationThread, SLOT(start()));
    //    connect(cycleSendOperationThread, SIGNAL(started()), this, SLOT(cycleSendProcessHandlerSlot()));
    //    connect(this, SIGNAL(cycleSendProcessFinish()), cycleSendOperationThread, SLOT(quit()));

    // Пункты главного меню
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

    /* Get interrupt data */
    /* We do not need to check tmkEvD.nInt because bcstartx with CX_NOSIG */
    /* guarantees us only single interrupt of single type nInt == 3       */
    tmkgetevd(&tmkEvD);

    if (tmkEvD.bcx.wResultX & SX_IB_MASK)
    {
        /* We have set bit(s) in Status Word */
        if (((tmkEvD.bcx.wResultX & SX_ERR_MASK) == SX_NOERR) ||
                ((tmkEvD.bcx.wResultX & SX_ERR_MASK) == SX_TOD))
        {
            /* We have either no errors or Data Time Out (No Data) error */
            wStatus = bcgetansw(wCtrlCode);
            if (wStatus & BUSY_MASK)
                /* We have BUSY bit set */
                ++dwBusyStarts;
            else
                /* We have unknown bit(s) set */
                ++dwStatStarts;
            //          if (kbhit())
            //            return 1;
        }
        else
        {
            /* We have an error */
            ++dwErrStarts;
        }
    }
    else if (tmkEvD.bcx.wResultX & SX_ERR_MASK)
    {
        /* We have an error */
        ++dwErrStarts;
    }
    else
    {
        /* We have a completed message */
        ++dwGoodStarts;
    }

    if (dwStarts%1000L == 0L)
    {
        qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
    }
    ++dwStarts;
    return 0;
}

void MainWindow::singleSendButtonSlot()
{
    //    int result = bcdefbus(BUS_A);
    //    if(result != 0) {
    //        bcdefbus(BUS_B);
    //    }
    //    if(result != 0) {
    //        QMessageBox *msgBox = new QMessageBox(this);
    //        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
    //        msgBox->setDefaultButton(QMessageBox::Ok);
    //        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //        msgBox->show();
    //        disconnectDriverButtonSlot();
    //        return;
    //    }

    //    QStringList dataStringList = lineSentMessageTextEdit->toPlainText().split(";");
    //    lastSendDescriptionTextEdit->clear();
    //    wLen = 0;
    //    for(; wLen < dataStringList.count(); wLen++) {
    //        if(wLen == (dataStringList.count() - 1)) {
    //            if(!dataStringList.at(wLen).isEmpty()) {
    //                if(wLen < 32) {
    //                    std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
    //                    iss >> std::hex >> awBuf[wLen];
    //                } else {
    //                    qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
    //                    QMessageBox *msgBox = new QMessageBox(this);
    //                    msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
    //                    msgBox->setDefaultButton(QMessageBox::Ok);
    //                    msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //                    msgBox->show();
    //                    return;
    //                }
    //            } else {
    //                break;
    //            }
    //        }
    //        if(wLen < 32) {
    //            std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
    //            iss >> std::hex >> awBuf[wLen];
    //        } else {
    //            qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
    //            QMessageBox *msgBox = new QMessageBox(this);
    //            msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
    //            msgBox->setDefaultButton(QMessageBox::Ok);
    //            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //            msgBox->show();
    //            return;
    //        }
    //    }

    //    if(deviceMode == KK_DEVICE_MODE) {
    //        RT_ADDR = addrYOValueBox->value();
    //        wSubAddr = subAddrYOValueBox->value();
    //        QString logStr;

    //        /* Пытаемся отправить(десять попыток, если что) */
    //        int tryNumber = 0;
    //        bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
    //        bcputblk(1, awBuf, wLen);
    //        do
    //        {
    //            bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
    //            if (WaitInt(DATA_BC_RT)) {
    //                qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
    //                //_______________________________________________________________________________________________________________
    //                logStr = "";
    //                logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                        + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                        + " RESULT: INTERRUPT_ERROR";
    //                lastSendDescriptionTextEdit->setText(logStr);
    //                //_______________________________________________________________________________________________________________
    //                sendStatusLabel->setEnabled(true);
    //                sendStatusLabel->setText(statusList.at(1));
    //                sendStatusLabel->setStyleSheet("QLabel{color:red;}");
    //                sendStatusLabel->setHidden(false);
    //                sendStatusLabel->setToolTip("Ответ от указанного ОУ не получен!\nЗначение tmkError == " + QString::number(tmkError) +
    //                                            "\nПроверьте введенные вами значения и попытайтесь снова...");
    //                break;
    //            }
    //            if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
    //                //_______________________________________________________________________________________________________________
    //                logStr = "";
    //                logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                        + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                        + " RESULT: OK";
    //                lastSendDescriptionTextEdit->setText(logStr);
    //                //_______________________________________________________________________________________________________________
    //                sendStatusLabel->setEnabled(true);
    //                sendStatusLabel->setText(statusList.at(0));
    //                sendStatusLabel->setStyleSheet("QLabel{color:green;}");
    //                sendStatusLabel->setHidden(false);
    //                sendStatusLabel->setToolTip("Данные успешно переданы в ОУ!");
    //                break;
    //            } else {
    //                qDebug() << tmkError;
    //                //_______________________________________________________________________________________________________________
    //                logStr = "";
    //                logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                        + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                        + " RESULT: WRONG_ADDRESS/SUBADDRESS_OR_ERROR_CONNECT_LINE/DEVICE";
    //                lastSendDescriptionTextEdit->setText(logStr);
    //                //_______________________________________________________________________________________________________________
    //                sendStatusLabel->setEnabled(true);
    //                sendStatusLabel->setText(statusList.at(1));
    //                sendStatusLabel->setStyleSheet("QLabel{color:red;}");
    //                sendStatusLabel->setHidden(false);
    //                sendStatusLabel->setToolTip("В ответном слове ОУ установлен бит!"
    //                                            "\nПроверьте адрес и/или подадрес выбранного ОУ, состояние приемо-передающего тракта и попытайтесь снова...");
    //                break;
    //            }
    //            tryNumber++;
    //        }
    //        while (tryNumber < 10);
    //    } else if (deviceMode == OY_DEVICE_MODE) {

    //    } else if (deviceMode == M_DEVICE_MODE) {

    //    } else {
    //        QMessageBox *msgBox = new QMessageBox(this);
    //        msgBox->setText("Не удалось отправить одиночное сообщение!\nПроверьте передаваемые вами значения и попытайтесь снова...");
    //        msgBox->setDefaultButton(QMessageBox::Ok);
    //        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //        msgBox->show();
    //    }
}

void MainWindow::readDataFromSubAddrServentDeviceSlot()
{
    //    int result = bcdefbus(BUS_A);
    //    if(result != 0) {
    //        bcdefbus(BUS_B);
    //    }
    //    if(result != 0) {
    //        QMessageBox *msgBox = new QMessageBox(this);
    //        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
    //        msgBox->setDefaultButton(QMessageBox::Ok);
    //        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //        msgBox->show();
    //        disconnectDriverButtonSlot();
    //        return;
    //    }

    //    if(deviceMode == KK_DEVICE_MODE) {
    //        RT_ADDR = addrYOValueBox->value();
    //        wSubAddr = subAddrYOValueBox->value();
    //        wLen = dataWordValueBox->value();

    //        /* Пытаемся получить ответ от ОУ до тех пор, пока не получится */
    //        int tryNumber = 0;
    //        bcputw(0, CW(RT_ADDR, RT_TRANSMIT, wSubAddr, wLen));
    //        do
    //        {
    //            bcstartx(wBase, DATA_RT_BC | CX_STOP | CX_BUS_A | CX_NOSIG);
    //            if (WaitInt(DATA_RT_BC)) {
    //                qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
    //                readStatusLabel->setEnabled(true);
    //                readStatusLabel->setText(statusList.at(1));
    //                readStatusLabel->setStyleSheet("QLabel{color:red;}");
    //                readStatusLabel->setHidden(false);
    //                readStatusLabel->setToolTip("Ответ от указанного ОУ не получен!\nЗначение tmkError == " + QString::number(tmkError) +
    //                                            "\nПроверьте запрашиваемые вами значения и попытайтесь снова...");
    //                break;
    //            }
    //            if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
    //                readStatusLabel->setEnabled(true);
    //                readStatusLabel->setText(statusList.at(2));
    //                readStatusLabel->setStyleSheet("QLabel{color:green;}");
    //                readStatusLabel->setHidden(false);
    //                readStatusLabel->setToolTip("Данные из подадреса ОУ успешно получены!");
    //                /* Check data from RT */
    //                QString receivedData = "READ: ";
    //                bcgetblk(2, awBuf, wLen);
    //                for (int i = 0; i < wLen; ++i)
    //                {
    //                    if (awBuf[i] != ((wSubAddr<<8) | i)) {
    //                        qDebug() << "\nCW:" + QString::number(bcgetw(0)) + " Data error " + "[" + QString::number(i) + "]" + " = " + QString::number(awBuf[i]);
    //                    }
    //                    std::ostringstream os;
    //                    os << std::hex << awBuf[i];
    //                    if(i != (wLen - 1)) {
    //                        receivedData.append(QString::fromStdString(os.str()) + ";");
    //                    } else {
    //                        receivedData.append(QString::fromStdString(os.str()));
    //                    }
    //                }
    //                readDataTextEdit->clear();
    //                readDataTextEdit->setText(receivedData);
    //                break;
    //            } else {
    //                qDebug() << tmkError;
    //                readStatusLabel->setEnabled(true);
    //                readStatusLabel->setText(statusList.at(3));
    //                readStatusLabel->setStyleSheet("QLabel{color:red;}");
    //                readStatusLabel->setHidden(false);
    //                readStatusLabel->setToolTip("В ответном слове ОУ установлен бит!\ntmkError = " + QString::number(tmkError) +
    //                                            "\nПроверьте запрашиваемые вами значения и/или состояние ОУ и попытайтесь снова...");
    //                break;
    //            }
    //            tryNumber++;
    //        }
    //        while (tryNumber < 10);
    //    }
}

void MainWindow::cycleSendProcessButtonSlot()
{
    //    if(cycleSendButton->text() == cycleSendButtonNameList.at(0)) {
    //        cycleSendIsActive = true;
    //        cycleSendButton->setText(cycleSendButtonNameList.at(1));
    //        selectModeTitleLabel->setEnabled(false);
    //        bcModeSelectButton->setEnabled(false);
    //        rtModeSelectButton->setEnabled(false);
    //        mtModeSelectButton->setEnabled(false);
    //        waitAnswerIntervalValueBoxTitleLabel->setEnabled(false);
    //        waitAnswerIntervalValueBox->setEnabled(false);
    //        setWaitAnswerIntervalButton->setEnabled(false);
    //        selectBaseForWorkTitleLabel->setEnabled(false);
    //        baseForWorkValueBox->setEnabled(false);
    //        selectBaseForWorkButton->setEnabled(false);
    //        inputYourMessageTitleLabel->setEnabled(false);
    //        lineSentMessageTextEdit->setEnabled(false);
    //        sendButton->setEnabled(false);
    //        addrOYTitleLabel->setEnabled(false);
    //        addrYOValueBox->setEnabled(false);
    //        subAddrOYTitleLabel->setEnabled(false);
    //        subAddrYOValueBox->setEnabled(false);
    //        readDataFromSubaddrTitleLabel->setEnabled(false);
    //        dataWordNumberLabel->setEnabled(false);
    //        dataWordValueBox->setEnabled(false);
    //        readDataFromSubaddrButton->setEnabled(false);
    //        readDataTextEdit->setEnabled(false);
    //        sendStatusLabel->setEnabled(false);
    //        readStatusLabel->setEnabled(false);
    //        sendStatusLabel->setHidden(true);
    //        readStatusLabel->setHidden(true);
    //        cycleSendStatusLabel->setHidden(false);
    //        lastSendDescriptionTitleLabel->setEnabled(false);
    //        lastSendDescriptionTextEdit->setEnabled(false);
    //        lastSendDescriptionTextEdit->clear();
    //        cycleSendStatusLabel->setText(statusList.at(4));
    //        emit startCycleSendProcessSignal();
    //    } else if (cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
    //        cycleSendIsActive = false;
    //        cycleSendButton->setText(cycleSendButtonNameList.at(0));
    //        selectModeTitleLabel->setEnabled(true);
    //        bcModeSelectButton->setEnabled(true);
    //        rtModeSelectButton->setEnabled(true);
    //        mtModeSelectButton->setEnabled(true);
    //        waitAnswerIntervalValueBoxTitleLabel->setEnabled(true);
    //        waitAnswerIntervalValueBox->setEnabled(true);
    //        setWaitAnswerIntervalButton->setEnabled(true);
    //        selectBaseForWorkTitleLabel->setEnabled(true);
    //        baseForWorkValueBox->setEnabled(true);
    //        selectBaseForWorkButton->setEnabled(true);
    //        inputYourMessageTitleLabel->setEnabled(true);
    //        lineSentMessageTextEdit->setEnabled(true);
    //        sendButton->setEnabled(true);
    //        addrOYTitleLabel->setEnabled(true);
    //        addrYOValueBox->setEnabled(true);
    //        subAddrOYTitleLabel->setEnabled(true);
    //        subAddrYOValueBox->setEnabled(true);
    //        readDataFromSubaddrTitleLabel->setEnabled(true);
    //        dataWordNumberLabel->setEnabled(true);
    //        dataWordValueBox->setEnabled(true);
    //        readDataFromSubaddrButton->setEnabled(true);
    //        readDataTextEdit->setEnabled(true);
    //        sendStatusLabel->setEnabled(true);
    //        readStatusLabel->setEnabled(true);
    //        sendStatusLabel->setHidden(true);
    //        readStatusLabel->setHidden(true);
    //        cycleSendStatusLabel->setHidden(true);
    //        lastSendDescriptionTitleLabel->setEnabled(true);
    //        lastSendDescriptionTextEdit->setEnabled(true);
    //        emit cycleSendProcessFinish();
    //    }
}

void MainWindow::cycleSendProcessHandlerSlot()
{
    //    int result = bcdefbus(BUS_A);
    //    if(result != 0) {
    //        bcdefbus(BUS_B);
    //    }
    //    if(result != 0) {
    //        QMessageBox *msgBox = new QMessageBox(this);
    //        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
    //        msgBox->setDefaultButton(QMessageBox::Ok);
    //        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //        msgBox->show();
    //        disconnectDriverButtonSlot();
    //        return;
    //    }

    //    QStringList dataStringList = lineSentMessageTextEdit->toPlainText().split(";");
    //    wLen = 0;
    //    for(; wLen < dataStringList.count(); wLen++) {
    //        if(wLen == (dataStringList.count() - 1)) {
    //            if(!dataStringList.at(wLen).isEmpty()) {
    //                if(wLen < 32) {
    //                    std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
    //                    iss >> std::hex >> awBuf[wLen];
    //                } else {
    //                    qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
    //                    QMessageBox *msgBox = new QMessageBox(this);
    //                    msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
    //                    msgBox->setDefaultButton(QMessageBox::Ok);
    //                    msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //                    msgBox->show();
    //                    return;
    //                }
    //            } else {
    //                break;
    //            }
    //        }
    //        if(wLen < 32) {
    //            std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
    //            iss >> std::hex >> awBuf[wLen];
    //        } else {
    //            qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
    //            QMessageBox *msgBox = new QMessageBox(this);
    //            msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
    //            msgBox->setDefaultButton(QMessageBox::Ok);
    //            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //            msgBox->show();
    //            return;
    //        }
    //    }

    //    RT_ADDR = addrYOValueBox->value();
    //    wSubAddr = subAddrYOValueBox->value();
    //    int tryNumber;
    //    int timeout = cycleSendIntervalValueBox->value();
    //    QString logStr;

    //    if(deviceMode == KK_DEVICE_MODE && RT_ADDR != 0) {
    //        fileCycleSendLogs.open(QIODevice::WriteOnly | QIODevice::Truncate);
    //        if(!fileCycleSendLogs.isOpen())
    //            qDebug() << "Ошибка предварительной очистки файла fileCycleSendLogs.txt...";
    //        fileCycleSendLogs.close();
    //        fileCycleSendLogs.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    //        if(!fileCycleSendLogs.isOpen())
    //            qDebug() << "Ошибка открытия файла fileCycleSendLogs.txt...";
    //        QTextStream stream(&fileCycleSendLogs);

    //        /* Пытаемся отправить(десять попыток, если что) */
    //        bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
    //        bcputblk(1, awBuf, wLen);
    //        do
    //        {
    //            tryNumber = 0;
    //            do
    //            {
    //                bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
    //                qDebug() << tryNumber;
    //                if (WaitInt(DATA_BC_RT)) {
    //                    qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
    //                    //_______________________________________________________________________________________________________________
    //                    logStr = "";
    //                    logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                            + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                            + " RESULT: INTERRUPT_ERROR";
    //                    stream << (logStr + "\n");
    //                    //_______________________________________________________________________________________________________________
    //                    QMessageBox *msgBox = new QMessageBox(this);
    //                    msgBox->setText("Выбранное ОУ не отвечает!\nЗначение tmkError == " + QString::number(tmkError) +
    //                                    "\nПроверьте введенные вами значения и/или номер ОУ и попытайтесь запустить циклическую отправку снова...");
    //                    msgBox->setDefaultButton(QMessageBox::Ok);
    //                    msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //                    msgBox->show();
    //                    cycleSendProcessButtonSlot();
    //                    break;
    //                }
    //                if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
    //                    //_______________________________________________________________________________________________________________
    //                    logStr = "";
    //                    logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                            + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                            + " RESULT: OK";
    //                    stream << (logStr + "\n");
    //                    //_______________________________________________________________________________________________________________
    //                    cycleSendStatusLabel->setEnabled(true);
    //                    cycleSendStatusLabel->setText(statusList.at(4));
    //                    cycleSendStatusLabel->setStyleSheet("QLabel{color:green;}");
    //                    cycleSendStatusLabel->setHidden(false);
    //                    cycleSendStatusLabel->setToolTip("Данные успешно передаются в ОУ!");
    //                } else {
    //                    //_______________________________________________________________________________________________________________
    //                    logStr = "";
    //                    logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
    //                            + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
    //                            + " RESULT: WRONG_ADDRESS/SUBADDRESS_OR_ERROR_CONNECT_LINE/DEVICE";
    //                    stream << (logStr + "\n");
    //                    //_______________________________________________________________________________________________________________
    //                    cycleSendStatusLabel->setEnabled(true);
    //                    cycleSendStatusLabel->setText(statusList.at(5));
    //                    cycleSendStatusLabel->setStyleSheet("QLabel{color:red;}");
    //                    cycleSendStatusLabel->setHidden(false);
    //                    cycleSendStatusLabel->setToolTip("В ответном слове ОУ установлен бит!"
    //                                                     "\nПроверьте адрес и/или подадрес выбранного ОУ, состояние приемо-передающего тракта и попытайтесь запустить циклическую отправку снова...");
    //                }
    //                sleepCurrentThread(timeout);
    //                tryNumber++;
    //            }
    //            while (tryNumber < 10 && cycleSendIsActive);
    //        }
    //        while(cycleSendIsActive);
    //        fileCycleSendLogs.close();
    //        if(stream.status() != QTextStream::Ok)
    //            qDebug() << "File write textStream error...";
    //    } else {
    //        QMessageBox *msgBox = new QMessageBox(this);
    //        msgBox->setText("Циклическая отправка исключает использование группового адреса ОУ!\nПожалуйста, укажите адрес конкретного ОУ и попытайтесь запустить циклическую отправку снова...");
    //        msgBox->setDefaultButton(QMessageBox::Ok);
    //        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    //        msgBox->show();
    //        cycleSendProcessButtonSlot();
    //    }
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
        event->accept();
    }
    else
    {
        event->ignore();
    }
    //    Q_UNUSED(event);
    //    if(cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
    //        cycleSendProcessButtonSlot();
    //    }
    //    bcreset();
    //#ifdef _WIN32
    //    CloseHandle(hBcEvent);
    //#endif
    //    tmkdone(ALL_TMKS);
    //    TmkClose();
    //    if(connectionStatusLabel->text() == connectionStatusVariants.at(1))
    //        connectionUARTButtonSlot();
    //    this->close();
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

void MainWindow::connectionUARTButtonSlot()
{
    //    if(connectionStatusLabel->text() == connectionStatusVariants.at(0)) {
    //        qint64 baudRate = baudRatesBox->currentText().toInt();
    //        QString portName = serialPortsBox->currentText();

    //        uartTransfer = new UartTransfer();
    //        uartTransfer->init(portName, baudRate);

    //        if(uartTransfer->isInit()) {
    //            qDebug() << "UART-соединение активно";
    //            connect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
    //            connectButton->setText("отключить");
    //            connectButton->setToolTip("деактивировать линию последовательной передачи");
    //            connectionStatusLabel->setText(connectionStatusVariants.at(1));
    //            connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
    //            sendUARTDataButton->setEnabled(true);
    //            baudRatesBox->setEnabled(false);
    //            serialPortsBox->setEnabled(false);
    //        } else {
    //            qDebug() << "UART-соединение не активно";
    //            sendUARTDataButton->setEnabled(false);
    //            connectButton->setToolTip("активировать линию последовательной передачи");
    //            disconnect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
    //            connectionStatusLabel->setText(connectionStatusVariants.at(0));
    //            connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
    //            baudRatesBox->setEnabled(true);
    //            serialPortsBox->setEnabled(true);
    //        }
    //    } else {
    //        if(uartTransfer != nullptr) {
    //            disconnect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
    //            uartTransfer->~UartTransfer();
    //            uartTransfer = nullptr;
    //            connectionStatusLabel->setText(connectionStatusVariants.at(0));
    //            connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
    //            connectButton->setText("подключить");
    //            connectButton->setToolTip("активировать линию последовательной передачи");
    //            sendUARTDataButton->setEnabled(false);
    //            baudRatesBox->setEnabled(true);
    //            serialPortsBox->setEnabled(true);
    //        }
    //    }
}

void MainWindow::updateCOMListSlot(int index) {
    //    serialPortsBox->blockSignals(true);
    //    serialPortsBox->clear();
    //    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
    //        serialPortsBox->addItem(info.portName());
    //    }
    //    serialPortsBox->setCurrentIndex(index);
    //    serialPortsBox->blockSignals(false);
}

void MainWindow::receivedDataSlot(QByteArray data)
{
    //    QString dataStrForView = "<<<:" + QString::fromUtf8(data) + "\n";
    //    receivedTransmittedUARTDataTextEdit->setText(receivedTransmittedUARTDataTextEdit->toPlainText() + dataStrForView);
    //    QTextCursor cursor = receivedTransmittedUARTDataTextEdit->textCursor();
    //    cursor.movePosition(QTextCursor::End);
    //    receivedTransmittedUARTDataTextEdit->setTextCursor(cursor);
}

void MainWindow::sendByUartDataButtonSlot()
{
    //    QString dataString = lineDForTransmittedUARTDataTextEdit->toPlainText().trimmed();

    //    QByteArray ba;
    //    ba.append(dataString.toUtf8());

    //    if(uartTransfer->isInit()) {
    //        uartTransfer->write(&ba);
    //        qDebug() << "Transmitted data:" +  QString::fromUtf8(ba);
    //        QString dataStrForView = ">>>:" + QString::fromUtf8(ba) + "\n";
    //        receivedTransmittedUARTDataTextEdit->setText(receivedTransmittedUARTDataTextEdit->toPlainText() + dataStrForView);
    //        QTextCursor cursor = receivedTransmittedUARTDataTextEdit->textCursor();
    //        cursor.movePosition(QTextCursor::End);
    //        receivedTransmittedUARTDataTextEdit->setTextCursor(cursor);
    //    } else {
    //        qDebug() << "Send error! UART is not initialized..";
    //    }
}

void MainWindow::clearUARTDataTextEditButtonSlot()
{
    //    receivedTransmittedUARTDataTextEdit->clear();
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
                         "<li>Сохранение лога работы</li>"
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
    msgBox.setWindowTitle(this->windowTitle());
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setWindowModality(Qt::WindowModality::WindowModal);
    msgBox.setWindowFlags (msgBox.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    msgBox.exec();
}


void MainWindow::on_logWriteClearButton_clicked()
{
    ui->logWriteMpiViewTextEdit->clear();
}


void MainWindow::on_logReadClearButton_clicked()
{
    ui->logReadMpiViewTextEdit->clear();
}


void MainWindow::on_inputMpiWriteDataButton_clicked()
{
//    if(bcdefbus(BUS_A)) {
//        if(bcdefbus(BUS_B)) {
//            emit qMessageBoxNeedShowSignal(tr("Ни основную, ни резервную ЛПИ активировать не удалось..."
//                                              "\nАктивируйте линию передачи и попытайтесь снова"));
//            stopMpi();
//            return;
//        }
//    }

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
    QString logLine = tr("КК;") + QDateTime::currentDateTime().toString("hh:mm:ss") + ";";
    if(ui->decimalFormatCheckBox->isChecked())
    {
        for(; wLen < sentWordsStringList.count(); ++wLen) {
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
        for(; wLen < sentWordsStringList.count(); ++wLen) {
            awBuf[wLen] = static_cast<unsigned short>(sentWordsStringList.at(wLen).trimmed().toInt(&ok, 16));
            hex16SentDataView.append((wLen == 0 ? "0x" : " 0x") + sentWordsStringList.at(wLen).trimmed());
            if(!ok)
            {
                emit qMessageBoxNeedShowSignal(tr("Ошибка записи слов в ОУ!\n"
                                                  "Проверьте введенные данные и попробуйте снова..."));
                return;
            }
        }
        logLine.append(hex16SentDataView + ";");
    }

    bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
    bcputblk(1, awBuf, wLen);
    // Отправка
    bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
    if (WaitInt(DATA_BC_RT)) {
        qWarning() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts)
                       + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
        logLine.append(tr("ошибка;время ожидания ответного события драйвера TA1-USB истекло"));
    }
    if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
        logLine.append(tr("ок"));
        qDebug() << logLine;
    } else {
        qWarning() << tmkError;
        logLine.append(tr("ошибка;данных о приеме/отказе от приема не получено"));
    }

    ui->logWriteMpiViewTextEdit->append(logLine);
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
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(
                    QRegularExpression(pattern),
                    this
                    );
        ui->inputMpiWriteDataLineEdit->setValidator(validator);
        ui->inputMpiWriteDataLineEdit->clear();
        ui->hexFormatCheckBox->setChecked(false);
    }
}


void MainWindow::on_hexFormatCheckBox_stateChanged(int arg1)
{
    if(arg1)
    {
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(
                    QRegularExpression("^([0-9A-Fa-f]{4}( [0-9A-Fa-f]{4}){0,31})?$"),
                    this
                    );
        ui->inputMpiWriteDataLineEdit->setValidator(validator);
        ui->inputMpiWriteDataLineEdit->clear();
        ui->decimalFormatCheckBox->setChecked(false);
    }
}

