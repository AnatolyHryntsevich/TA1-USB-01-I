#include "mainwindow.h"
#include "WDMTMKv2.cpp" //в хедере не размещать, дабы не нарваться на multiple definition

HANDLE hBcEvent;
TTmkEventData tmkEvD;
unsigned short awBuf[32];
TMK_DATA wBase, wMaxBase, wAddr, wSubAddr, wLen, wState, wStatus;
unsigned long dwGoodStarts = 0, dwBusyStarts = 0, dwErrStarts = 0, dwStatStarts = 0;
unsigned long dwStarts = 0L;

#define RT_ADDR wAddr /* RT address */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QDir currentDir;
    QString fileName = "cycleSendLogs.txt";
    QString filePath = currentDir.absoluteFilePath(fileName);
    fileCycleSendLogs.setFileName(filePath);

    connectionUARTWidget = new QWidget(this);
    baudRatesBoxTitle = new QLabel("Baud rate:");
    baudRatesBoxTitle->setFixedSize(100, 20);
    baudRatesBoxTitle->setFrameStyle(QFrame::Box);
    baudRatesBox = new QComboBox;
    QList<int> boudRates = QSerialPortInfo::standardBaudRates();
    foreach(int rate, boudRates)
    {
        baudRatesBox->addItem(QString::number(rate));
    }
    baudRatesBox->setToolTip("список доступных значений baudRate");
    serialPortsBoxTitle = new QLabel("COM-port:");
    serialPortsBoxTitle->setFixedSize(100, 20);
    serialPortsBoxTitle->setFrameStyle(QFrame::Box);
    serialPortsBox = new QComboBox;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &info : ports)
    {
        serialPortsBox ->addItem(info.portName());
    }
    serialPortsBox->setToolTip("для обновления выберите любой из доступных портов, а затем откройте список заново");
    connectionStatusVariants << "порт не активен" << "порт активен";
    connectionStatusLabel = new QLabel(connectionStatusVariants.at(0));
    connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
    connectButton = new QPushButton("подключить");
    receivedTransmittedUARTDataTextEdit = new QTextEdit();
    receivedTransmittedUARTDataTextEdit->setFixedSize(550, 670);
    receivedTransmittedUARTDataTextEdit->setToolTip("поле отображения полученных/отправленных по выбранному порту данных");
    lineDForTransmittedUARTDataTextEdit = new QTextEdit();
    lineDForTransmittedUARTDataTextEdit->setFixedSize(550, 23);
    lineDForTransmittedUARTDataTextEdit->setToolTip("поле ввода данных для отправки");
    sendUARTDataButton = new QPushButton("отправить");
    sendUARTDataButton->setToolTip("отправить данные по последовательному порту");
    sendUARTDataButton->setEnabled(false);
    clearUartDataButton = new QPushButton("очистить");
    clearUartDataButton->setToolTip("очистить поле отображения принятых/отправленных по последовательному порту данных");

    connectioinUARTLayout = new QGridLayout;
    connectioinUARTLayout->setContentsMargins(5, 5, 5, 5);
    connectioinUARTLayout->setSpacing(5);

    connectioinUARTLayout->addWidget(serialPortsBoxTitle, 0, 0, Qt::AlignRight);
    connectioinUARTLayout->addWidget(serialPortsBox, 0, 1, Qt::AlignLeft);
    connectioinUARTLayout->addWidget(baudRatesBoxTitle, 0, 2, Qt::AlignLeft);
    connectioinUARTLayout->addWidget(baudRatesBox, 0, 3, Qt::AlignLeft);
    connectioinUARTLayout->addWidget(connectButton, 0, 4, Qt::AlignLeft);
    connectioinUARTLayout->addWidget(connectionStatusLabel, 0, 0, Qt::AlignLeft);
    connectioinUARTLayout->addWidget(receivedTransmittedUARTDataTextEdit, 2, 0);
    connectioinUARTLayout->addWidget(lineDForTransmittedUARTDataTextEdit, 3, 0);
    connectioinUARTLayout->addWidget(clearUartDataButton, 4, 3);
    connectioinUARTLayout->addWidget(sendUARTDataButton, 4, 4);

    connectionUARTWidget->setLayout(connectioinUARTLayout);


    statusList << "отправлено" << "не отправлено" << "прочитано" << "не прочитано" << "без ошибок" << "ошибка";
    cycleSendButtonNameList << "начать" << "завершить";

    QWidget *MIL_STD_Widget = new QWidget;
    mainWindowTitle = new QLabel("Активируйте драйвер устройства:");
    mainWindowTitle->setFixedSize(185, 20);
    mainWindowTitle->setFrameStyle(QFrame::Box);
    mainWindowTitle->setToolTip("активация драйвера system32\\drivers\\ezusb.sys");
    connectionDriverButton = new QPushButton("активировать");
    connectionDriverButton->setToolTip("активация драйвера устройства");
    disconnectionDriverButton = new QPushButton("деактивировать");
    disconnectionDriverButton->setToolTip("деактивация драйвера устройства");
    connectResultText = new QLabel();
    connectResultText->setFixedSize(180, 20);
    connectResultText->setFrameStyle(QFrame::Box);
    connectResultText->setEnabled(true);
    connectResultText->setToolTip("статус работы драйвера system32\\drivers\\ezusb.sys");
    QList<int> devicesNumbersList;
    for(int i = 0; i < MAX_TMK_NUMBER; i++)
    {
        devicesNumbersList << i;
    }
    devicesNumbersTitleLabel = new QLabel("Номер устройства:");
    devicesNumbersListBox = new QComboBox();
    foreach(int number, devicesNumbersList)
    {
        devicesNumbersListBox->addItem(QString::number(number));
    }
    devicesNumbersListBox->setToolTip("выбранный номер будет назначен устройству\n[доступен любой свободный от 0 до максимального]");
    devicesNumbersTitleLabel->setEnabled(false);
    devicesNumbersListBox->setEnabled(false);
    connectionDeviceButton = new QPushButton(TRY_CONNECT_DEVICE_BUTTON_STRING);
    connectionDeviceButton->setEnabled(false);
    connectionDeviceButton->setToolTip("подключить/отключить устройство");
    waitAnswerIntervalValueBoxTitleLabel = new QLabel("Таймаут ожидания:");
    waitAnswerIntervalValueBox = new QComboBox();
    waitAnswerIntervalValueBox->setToolTip("Интервалы ожидания ответного слова(мкс)");
    waitAnswerIntervalValueBox->addItem(QString::number(14));
    waitAnswerIntervalValueBox->addItem(QString::number(18));
    waitAnswerIntervalValueBox->addItem(QString::number(26));
    waitAnswerIntervalValueBox->addItem(QString::number(63));
    setWaitAnswerIntervalButton = new QPushButton("применить");
    setWaitAnswerIntervalButton->setToolTip("изменить значение таймаута ожидания ответного слова");
    selectModeTitleLabel = new QLabel("         Выберите режим работы:");
    selectModeTitleLabel->setFixedSize(180, 20);
    selectModeTitleLabel->setFrameStyle(QFrame::Box);
    selectModeTitleLabel->setToolTip("выбор режима работы подключенного устройства:\nКК - контроллер канала, ОУ - оконечное устройство, МТ - монитор");
    bcModeSelectButton = new QPushButton("KK");
    bcModeSelectButton->setFixedSize(50, 20);
    bcModeSelectButton->setToolTip("контроллер канала\n[зеленый - текущий режим]");
    rtModeSelectButton = new QPushButton("ОУ");
    rtModeSelectButton->setFixedSize(50, 20);
    rtModeSelectButton->setToolTip("оконечное устройство\n[зеленый - текущий режим]");
    mtModeSelectButton = new QPushButton("MT");
    mtModeSelectButton->setFixedSize(50, 20);
    mtModeSelectButton->setToolTip("монитор\n[зеленый - текущий режим]");
    deviceMode = UNKNOW_DEVICE_MODE;
    selectBaseForWorkTitleLabel = new QLabel("  Выберите номер базы ДОЗУ:");
    selectBaseForWorkTitleLabel->setFixedSize(180, 20);
    selectBaseForWorkTitleLabel->setFrameStyle(QFrame::Box);
    selectBaseForWorkTitleLabel->setToolTip("выбор базы ДОЗУ для дальнейшего использования в качестве буфера приема/передачи");
    baseForWorkValueBox = new QSpinBox();
    baseForWorkValueBox->setToolTip("номера баз ДОЗУ");
    baseForWorkValueBox->setRange(0, 10);
    baseForWorkValueBox->setSuffix("-ая");
    baseForWorkValueBox->setButtonSymbols(QSpinBox::PlusMinus);
    baseForWorkValueBox->setWrapping(true);
    baseForWorkValueBox->setValue(0);
    selectBaseForWorkButton = new QPushButton("применить");
    selectBaseForWorkButton->setToolTip("использовать указанную базу");
    inputYourMessageTitleLabel = new QLabel("Отправка одиночного сообщения:");
    inputYourMessageTitleLabel->setFixedSize(180, 20);
    inputYourMessageTitleLabel->setFrameStyle(QFrame::Box);
    inputYourMessageTitleLabel->setToolTip("запись отправляемых данных в подадрес указанного ОУ");
    addrOYTitleLabel = new QLabel("Адрес ОУ-получателя:");
    addrOYTitleLabel->setFixedSize(130, 20);
    addrYOValueBox = new QSpinBox();
    addrYOValueBox->setFixedSize(60, 20);
    addrYOValueBox->setToolTip("адреса ОУ");
    addrYOValueBox->setRange(0, 2147483647);
    addrYOValueBox->setButtonSymbols(QSpinBox::PlusMinus);
    addrYOValueBox->setWrapping(true);
    addrYOValueBox->setValue(0);
    subAddrOYTitleLabel = new QLabel("Субадрес получателя:");
    subAddrOYTitleLabel->setFixedSize(130, 20);
    subAddrYOValueBox = new QSpinBox();
    subAddrYOValueBox->setFixedSize(60, 20);
    subAddrYOValueBox->setToolTip("субадреса ОУ");
    subAddrYOValueBox->setRange(0, 2147483647);
    subAddrYOValueBox->setButtonSymbols(QSpinBox::PlusMinus);
    subAddrYOValueBox->setWrapping(true);
    subAddrYOValueBox->setValue(0);
    lineSentMessageTextEdit = new QTextEdit();
    lineSentMessageTextEdit->setFixedSize(180, 23);
    lineSentMessageTextEdit->setToolTip("введите данные в формате hex16, разделяя точкой с запятой ';'\n[каждая пара чисел(напр: '00;04ff;55' == '0x00', '0x04ff' и '0x55') - это число в hex16]");
    lastSendDescriptionTitleLabel = new QLabel("Описание последней операции:");
    lastSendDescriptionTextEdit = new QTextEdit();
    lastSendDescriptionTextEdit->setFixedSize(180, 50);
    lastSendDescriptionTextEdit->setFrameStyle(QFrame::Box);
    lastSendDescriptionTextEdit->setToolTip("поле вывода описания(буквально, лог) каждой последней операции одиночной отправки данных на запись в подадрес ОУ");
    sendButton = new QPushButton("отправить");
    sendButton->setToolTip("отправить одиночное сообщение");
    sendStatusLabel = new QLabel(statusList.at(0));
    sendStatusLabel->setStyleSheet("QLabel{color:red;}");
    cycleSendTitleLabel = new QLabel("          Циклическая отправка:");
    cycleSendTitleLabel->setFixedSize(180, 20);
    cycleSendTitleLabel->setFrameStyle(QFrame::Box);
    cycleSendTitleLabel->setToolTip("запуск циклической отправки сообщений(на запись данных в подадрес ОУ)");
    cycleSendButton = new QPushButton(cycleSendButtonNameList.at(0));
    cycleSendButton->setToolTip("запуск/остановка цикла отправки с указанным интервалом между пакетами");
    cycleSendIntervalValuesBoxTitleLabel = new QLabel("Время от->до отправки:");
    cycleSendIntervalValueBox = new QSpinBox();
    cycleSendIntervalValueBox->setToolTip("временные интервалы между циклически отправляемыми сообщениями(мс)");
    cycleSendIntervalValueBox->setRange(1, 5000);
    cycleSendIntervalValueBox->setButtonSymbols(QSpinBox::PlusMinus);
    cycleSendIntervalValueBox->setWrapping(true);
    cycleSendIntervalValueBox->setValue(0);
    cycleSendIntervalValueBox->setSuffix("мс");
    cycleSendStatusLabel = new QLabel();
    cycleSendStatusLabel->setStyleSheet("QLabel{color:green;}");
    readDataFromSubaddrTitleLabel = new QLabel("Прочесть данные в подадресе:");
    readDataFromSubaddrTitleLabel->setFixedSize(180, 20);
    readDataFromSubaddrTitleLabel->setFrameStyle(QFrame::Box);
    readDataFromSubaddrTitleLabel->setToolTip("отправка запроса в ОУ на выдачу определенного количества слов данных из указанного подадреса");
    dataWordNumberLabel = new QLabel("Количество слов данных");
    dataWordNumberLabel->setToolTip("количество запрашиваемых из подадреса слов данных(16-bits целое)");
    dataWordValueBox = new QSpinBox();
    dataWordValueBox->setFixedSize(50, 20);
    dataWordValueBox->setToolTip("количество запрашиваемых слов данных");
    dataWordValueBox->setRange(1, 32);
    dataWordValueBox->setButtonSymbols(QSpinBox::PlusMinus);
    dataWordValueBox->setWrapping(true);
    dataWordValueBox->setValue(0);
    readDataFromSubaddrButton = new QPushButton("прочесть");
    readDataFromSubaddrButton->setToolTip("отправить запрос на чтение в ОУ");
    readStatusLabel = new QLabel(statusList.at(2));
    readStatusLabel->setStyleSheet("QLabel{color:red;}");
    readDataTextEdit = new QTextEdit();
    readDataTextEdit->setFixedSize(180, 50);
    readDataTextEdit->setFrameStyle(QFrame::Box);
    readDataTextEdit->setToolTip("поле отображения полученных от ОУ данных из указанного подадреса\nв hex16 через точку с запятой");

    disconnectDriverButtonSlot();

    MIL_STD_WidgetLayout = new QGridLayout;
    MIL_STD_WidgetLayout->setContentsMargins(10, 3, 10, 3);
    MIL_STD_WidgetLayout->setSpacing(10);
    MIL_STD_WidgetLayout->setHorizontalSpacing(3);

    MIL_STD_WidgetLayout->addWidget(mainWindowTitle);
    MIL_STD_WidgetLayout->addWidget(connectResultText);
    MIL_STD_WidgetLayout->addWidget(connectionDriverButton, 2, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(disconnectionDriverButton, 2, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(devicesNumbersTitleLabel, 3, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(devicesNumbersListBox, 3, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(connectionDeviceButton, 4, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(waitAnswerIntervalValueBoxTitleLabel, 5, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(waitAnswerIntervalValueBox, 5, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(setWaitAnswerIntervalButton, 6, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(selectModeTitleLabel, 7, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(bcModeSelectButton, 8, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(rtModeSelectButton, 8, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(mtModeSelectButton, 8, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(selectBaseForWorkTitleLabel, 9, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(baseForWorkValueBox, 10, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(selectBaseForWorkButton, 10, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(addrOYTitleLabel, 11, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(addrYOValueBox, 11, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(subAddrOYTitleLabel, 12, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(subAddrYOValueBox, 12, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(inputYourMessageTitleLabel, 13, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(lineSentMessageTextEdit, 14, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(sendStatusLabel, 15, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(sendButton, 15, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(lastSendDescriptionTitleLabel, 16, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(lastSendDescriptionTextEdit, 17, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(cycleSendTitleLabel, 18, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(cycleSendIntervalValuesBoxTitleLabel, 19, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(cycleSendIntervalValueBox, 19, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(cycleSendButton, 20, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(cycleSendStatusLabel, 20, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(readDataFromSubaddrTitleLabel, 21, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(dataWordNumberLabel, 22, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(dataWordValueBox, 22, 0, Qt::AlignRight);
    MIL_STD_WidgetLayout->addWidget(readDataTextEdit, 23, 0, Qt::AlignHCenter);
    MIL_STD_WidgetLayout->addWidget(readStatusLabel, 24, 0, Qt::AlignLeft);
    MIL_STD_WidgetLayout->addWidget(readDataFromSubaddrButton, 24, 0, Qt::AlignRight);
    MIL_STD_Widget->setLayout(MIL_STD_WidgetLayout);

    QWidget *mainWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(10, 3, 10, 3);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(connectionUARTWidget, 0);
    mainLayout->addWidget(MIL_STD_Widget, 1);
    mainWidget->setLayout(mainLayout);
    this->setCentralWidget(mainWidget);
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    this->setFixedSize(QSize(800, 815));

    connect(connectionDriverButton, SIGNAL(clicked()), this, SLOT(connectDriverButtonSlot()));
    connect(disconnectionDriverButton, SIGNAL(clicked()), this, SLOT(disconnectDriverButtonSlot()));
    connect(connectionDeviceButton, SIGNAL(clicked()), this, SLOT(connectDeviceButtonSlot()));
    connect(setWaitAnswerIntervalButton, SIGNAL(clicked()), this, SLOT(setWaitAnswerIntervalButtonSlot()));
    connect(bcModeSelectButton, SIGNAL(clicked()), this, SLOT(clickDeviceModeButtonsSlot()));
    connect(rtModeSelectButton, SIGNAL(clicked()), this, SLOT(clickDeviceModeButtonsSlot()));
    connect(mtModeSelectButton, SIGNAL(clicked()), this, SLOT(clickDeviceModeButtonsSlot()));
    //_______________________________________________________________________________________________________
    cycleSendOperationThread = new QThread();
    connect(cycleSendButton, SIGNAL(clicked()), this, SLOT(cycleSendProcessButtonSlot()));
    connect(this, SIGNAL(startCycleSendProcessSignal()), cycleSendOperationThread, SLOT(start()));
    connect(cycleSendOperationThread, SIGNAL(started()), this, SLOT(cycleSendProcessHandlerSlot()));
    connect(this, SIGNAL(cycleSendProcessFinish()), cycleSendOperationThread, SLOT(quit()));
    //_______________________________________________________________________________________________________
    connect(sendButton, SIGNAL(clicked()), this, SLOT(singleSendButtonSlot()));
    connect(readDataFromSubaddrButton, SIGNAL(clicked()), this, SLOT(readDataFromSubAddrServentDeviceSlot()));
    connect(selectBaseForWorkButton, SIGNAL(clicked()), this, SLOT(selectBaseValueButtonSlot()));

    //____________UART_____TERMITE___________________________________________________________________________
    connect(serialPortsBox, SIGNAL(activated(int)), this, SLOT(updateCOMListSlot(int)));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectionUARTButtonSlot()));
    connect(sendUARTDataButton, SIGNAL(clicked()), this, SLOT(sendByUartDataButtonSlot()));
    connect(clearUartDataButton, SIGNAL(clicked()), this, SLOT(clearUARTDataTextEditButtonSlot()));
}

MainWindow::~MainWindow()
{
}

int MainWindow::initTmkEvent()
{
    hBcEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hBcEvent)
    {
      qDebug() << "Не удалось создать tmkEvent...";
      return -1;
    }
    ResetEvent(hBcEvent);
    tmkdefevent(hBcEvent, TRUE);
    return 0;
}

void MainWindow::sleepCurrentThread(int ms)
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
      switch (WaitForSingleObject(hBcEvent, 1000))
      {
      case WAIT_OBJECT_0:
        ResetEvent(hBcEvent);
        break;
      case WAIT_TIMEOUT:
        qDebug() << "Interrupt timeout error\n";
        //printf("Interrupt timeout error\n");
        return 1;
      default:
        qDebug() << "Interrupt wait error\n";
        //printf("Interrupt wait error\n");
        return 1;
      }

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
    //  printf("%ld %04X\n", dwGoodStarts, bcgetw(0));
    //  Sleep(500);
      return 0;
}

void MainWindow::connectDriverButtonSlot()
{
    DWORD result;
    if(cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
       cycleSendProcessButtonSlot();
    }
    result = TmkOpen();
    if(result == 0) {
      connectResultText->setText("              driver is activated!");
      connectResultText->setStyleSheet("QLabel{color:green;}");
      connectionDeviceButton->setEnabled(true);
      connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
      connectionDeviceButton->setStyleSheet("QPushButton{color:green;}");
      devicesNumbersListBox->setEnabled(true);
      devicesNumbersTitleLabel->setEnabled(true);
      selectModeTitleLabel->setEnabled(false);
      bcModeSelectButton->setEnabled(false);
      rtModeSelectButton->setEnabled(false);
      mtModeSelectButton->setEnabled(false);
      bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
      rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
      mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
      inputYourMessageTitleLabel->setEnabled(false);
      lineSentMessageTextEdit->setEnabled(false);
      sendButton->setEnabled(false);
      selectBaseForWorkTitleLabel->setEnabled(false);
      baseForWorkValueBox->setEnabled(false);
      selectBaseForWorkButton->setEnabled(false);
      addrOYTitleLabel->setEnabled(false);
      addrYOValueBox->setEnabled(false);
      subAddrOYTitleLabel->setEnabled(false);
      subAddrYOValueBox->setEnabled(false);
      readDataFromSubaddrTitleLabel->setEnabled(false);
      dataWordNumberLabel->setEnabled(false);
      dataWordValueBox->setEnabled(false);
      readDataFromSubaddrButton->setEnabled(false);
      readDataTextEdit->setEnabled(false);
      sendStatusLabel->setEnabled(false);
      readStatusLabel->setEnabled(false);
      sendStatusLabel->setHidden(true);
      readStatusLabel->setHidden(true);
      cycleSendTitleLabel->setEnabled(false);
      cycleSendButton->setEnabled(false);
      cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
      cycleSendIntervalValueBox->setEnabled(false);
      cycleSendStatusLabel->setEnabled(false);
      cycleSendStatusLabel->setHidden(true);
      lastSendDescriptionTitleLabel->setEnabled(false);
      lastSendDescriptionTextEdit->setEnabled(false);
      deviceMode = UNKNOW_DEVICE_MODE;
    } else if (result == 1) {
        connectResultText->setText("            driver is not activated!");
        connectResultText->setStyleSheet("QLabel{color:red;}");
        connectionDeviceButton->setEnabled(false);
        waitAnswerIntervalValueBoxTitleLabel->setEnabled(false);
        waitAnswerIntervalValueBox->setEnabled(false);
        setWaitAnswerIntervalButton->setEnabled(false);
        selectModeTitleLabel->setEnabled(false);
        bcModeSelectButton->setEnabled(false);
        rtModeSelectButton->setEnabled(false);
        mtModeSelectButton->setEnabled(false);
        devicesNumbersTitleLabel->setEnabled(false);
        devicesNumbersListBox->setEnabled(false);
        bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
        rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
        mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
        inputYourMessageTitleLabel->setEnabled(false);
        lineSentMessageTextEdit->setEnabled(false);
        sendButton->setEnabled(false);
        selectBaseForWorkTitleLabel->setEnabled(false);
        baseForWorkValueBox->setEnabled(false);
        selectBaseForWorkButton->setEnabled(false);
        addrOYTitleLabel->setEnabled(false);
        addrYOValueBox->setEnabled(false);
        subAddrOYTitleLabel->setEnabled(false);
        subAddrYOValueBox->setEnabled(false);
        readDataFromSubaddrTitleLabel->setEnabled(false);
        dataWordNumberLabel->setEnabled(false);
        dataWordValueBox->setEnabled(false);
        readDataFromSubaddrButton->setEnabled(false);
        readDataTextEdit->setEnabled(false);
        sendStatusLabel->setEnabled(false);
        readStatusLabel->setEnabled(false);
        sendStatusLabel->setHidden(true);
        readStatusLabel->setHidden(true);
        cycleSendTitleLabel->setEnabled(false);
        cycleSendButton->setEnabled(false);
        cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
        cycleSendIntervalValueBox->setEnabled(false);
        cycleSendStatusLabel->setEnabled(false);
        cycleSendStatusLabel->setHidden(true);
        lastSendDescriptionTitleLabel->setEnabled(false);
        lastSendDescriptionTextEdit->setEnabled(false);
        deviceMode = UNKNOW_DEVICE_MODE;
        connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
        connectionDeviceButton->setStyleSheet("QPushButton{color:gray;}");
    }

}

void MainWindow::disconnectDriverButtonSlot()
{
    if(cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
       cycleSendProcessButtonSlot();
    }
    bcreset();
    CloseHandle(hBcEvent);
    tmkdone(ALL_TMKS);
    TmkClose();
    connectResultText->setText("            driver is not activated!");
    connectResultText->setStyleSheet("QLabel{color:red;}");
    connectionDeviceButton->setEnabled(false);
    waitAnswerIntervalValueBoxTitleLabel->setEnabled(false);
    waitAnswerIntervalValueBox->setEnabled(false);
    setWaitAnswerIntervalButton->setEnabled(false);
    selectModeTitleLabel->setEnabled(false);
    bcModeSelectButton->setEnabled(false);
    rtModeSelectButton->setEnabled(false);
    mtModeSelectButton->setEnabled(false);
    bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
    rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
    mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
    inputYourMessageTitleLabel->setEnabled(false);
    lineSentMessageTextEdit->setEnabled(false);
    sendButton->setEnabled(false);
    selectBaseForWorkTitleLabel->setEnabled(false);
    baseForWorkValueBox->setEnabled(false);
    selectBaseForWorkButton->setEnabled(false);
    devicesNumbersTitleLabel->setEnabled(false);
    devicesNumbersListBox->setEnabled(false);
    addrOYTitleLabel->setEnabled(false);
    addrYOValueBox->setEnabled(false);
    subAddrOYTitleLabel->setEnabled(false);
    subAddrYOValueBox->setEnabled(false);
    readDataFromSubaddrTitleLabel->setEnabled(false);
    dataWordNumberLabel->setEnabled(false);
    dataWordValueBox->setEnabled(false);
    readDataFromSubaddrButton->setEnabled(false);
    readDataTextEdit->setEnabled(false);
    sendStatusLabel->setEnabled(false);
    readStatusLabel->setEnabled(false);
    sendStatusLabel->setHidden(true);
    readStatusLabel->setHidden(true);
    cycleSendTitleLabel->setEnabled(false);
    cycleSendButton->setEnabled(false);
    cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
    cycleSendIntervalValueBox->setEnabled(false);
    cycleSendStatusLabel->setEnabled(false);
    cycleSendStatusLabel->setHidden(true);
    lastSendDescriptionTitleLabel->setEnabled(false);
    lastSendDescriptionTextEdit->setEnabled(false);
    deviceMode = UNKNOW_DEVICE_MODE;
    connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
    connectionDeviceButton->setStyleSheet("QPushButton{color:gray;}");
}

void MainWindow::connectDeviceButtonSlot()
{
    unsigned result;
    if(cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
       cycleSendProcessButtonSlot();
    }
    if(connectionDeviceButton->text() == TRY_DISCONNECT_DEVICE_BUTTON_STRING) {
        result = tmkdone(ALL_TMKS);
        if(result == 0) {
            connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
            connectionDeviceButton->setStyleSheet("QPushButton{color:green;}");
            devicesNumbersListBox->setEnabled(true);
            waitAnswerIntervalValueBoxTitleLabel->setEnabled(false);
            waitAnswerIntervalValueBox->setEnabled(false);
            setWaitAnswerIntervalButton->setEnabled(false);
            selectModeTitleLabel->setEnabled(false);
            bcModeSelectButton->setEnabled(false);
            rtModeSelectButton->setEnabled(false);
            mtModeSelectButton->setEnabled(false);
            bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
            rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
            mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
            deviceMode = UNKNOW_DEVICE_MODE;
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            selectBaseForWorkTitleLabel->setEnabled(false);
            baseForWorkValueBox->setEnabled(false);
            selectBaseForWorkButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(false);
            addrYOValueBox->setEnabled(false);
            subAddrOYTitleLabel->setEnabled(false);
            subAddrYOValueBox->setEnabled(false);
            readDataFromSubaddrTitleLabel->setEnabled(false);
            dataWordNumberLabel->setEnabled(false);
            dataWordValueBox->setEnabled(false);
            readDataFromSubaddrButton->setEnabled(false);
            readDataTextEdit->setEnabled(false);
            sendStatusLabel->setEnabled(false);
            readStatusLabel->setEnabled(false);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(false);
            cycleSendButton->setEnabled(false);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
            cycleSendIntervalValueBox->setEnabled(false);
            cycleSendStatusLabel->setHidden(true);
            lastSendDescriptionTitleLabel->setEnabled(false);
            lastSendDescriptionTextEdit->setEnabled(false);
        } else {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось освободить ресурс!\nПовторите попытку позже...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
        }
    } else if (connectionDeviceButton->text() == TRY_CONNECT_DEVICE_BUTTON_STRING) {
        result = tmkconfig(devicesNumbersListBox->currentData().toInt());
        if(result == 0) {
           if(initTmkEvent() == 0) {
               if(result == 0) {
                   connectionDeviceButton->setText(TRY_DISCONNECT_DEVICE_BUTTON_STRING);
                   connectionDeviceButton->setStyleSheet("QPushButton{color:red;}");
                   devicesNumbersListBox->setEnabled(false);
                   waitAnswerIntervalValueBoxTitleLabel->setEnabled(true);
                   waitAnswerIntervalValueBox->setEnabled(true);
                   setWaitAnswerIntervalButton->setEnabled(true);
                   selectModeTitleLabel->setEnabled(true);
                   bcModeSelectButton->setEnabled(false);
                   bcModeSelectButton->setStyleSheet("QPushButton{color:green;}");
                   rtModeSelectButton->setEnabled(true);
                   mtModeSelectButton->setEnabled(true);
                   deviceMode = KK_DEVICE_MODE;
                   selectBaseForWorkTitleLabel->setEnabled(true);
                   wMaxBase = bcgetmaxbase();
                   baseForWorkValueBox->setRange(0, wMaxBase);
                   baseForWorkValueBox->setEnabled(true);
                   selectBaseForWorkButton->setEnabled(true);
               } else {
                   QMessageBox *msgBox = new QMessageBox(this);
                   msgBox->setText("Не удалось получить доступ к устройству!\nПовторите попытку позже...");
                   msgBox->setDefaultButton(QMessageBox::Ok);
                   msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                   msgBox->show();
                   connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
                   connectionDeviceButton->setStyleSheet("QPushButton{color:green;}");
               }
               result = tmkselect(devicesNumbersListBox->currentData().toInt());
           } else {
               QMessageBox *msgBox = new QMessageBox(this);
               msgBox->setText("Не удалось инициализировать событие приема/передачи tmk!\nПовторите попытку позже...");
               msgBox->setDefaultButton(QMessageBox::Ok);
               msgBox->setWindowModality(Qt::WindowModality::WindowModal);
               msgBox->show();
               connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
               connectionDeviceButton->setStyleSheet("QPushButton{color:green;}");
           }
        } else {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось получить доступ к устройству!\nПовторите попытку позже...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            connectionDeviceButton->setText(TRY_CONNECT_DEVICE_BUTTON_STRING);
            connectionDeviceButton->setStyleSheet("QPushButton{color:green;}");
        }
    }
}

void MainWindow::setWaitAnswerIntervalButtonSlot()
{
    unsigned short result;
    result = tmktimeout((unsigned short) waitAnswerIntervalValueBox->currentText().toInt());
    if(result > 0) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Значение таймаута ожидания ответного слова изменено!\nНаиболее близким к указанному из доступных равно " + QString::number(result) + "мкс");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
    } else if(result == 0) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось изменить значение таймаута ожидания ответного слова!\nПовторите попытку позже...");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
    }
}

void MainWindow::clickDeviceModeButtonsSlot()
{
    unsigned result;
    if(QObject::sender() == bcModeSelectButton) {
        result = bcreset();
            if(result == 0) {
                bcModeSelectButton->setStyleSheet("QPushButton{color:green;}");
                bcModeSelectButton->setEnabled(false);
                rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                rtModeSelectButton->setEnabled(true);
                mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                mtModeSelectButton->setEnabled(true);
                selectBaseForWorkTitleLabel->setEnabled(true);
                baseForWorkValueBox->setEnabled(true);
                selectBaseForWorkButton->setEnabled(true);
                inputYourMessageTitleLabel->setEnabled(false);
                lineSentMessageTextEdit->setEnabled(false);
                sendButton->setEnabled(false);
                addrOYTitleLabel->setEnabled(false);
                addrYOValueBox->setEnabled(false);
                subAddrOYTitleLabel->setEnabled(false);
                subAddrYOValueBox->setEnabled(false);
                readDataFromSubaddrTitleLabel->setEnabled(false);
                dataWordNumberLabel->setEnabled(false);
                dataWordValueBox->setEnabled(false);
                readDataFromSubaddrButton->setEnabled(false);
                readDataTextEdit->setEnabled(false);
                sendStatusLabel->setEnabled(false);
                readStatusLabel->setEnabled(false);
                sendStatusLabel->setHidden(true);
                readStatusLabel->setHidden(true);
                cycleSendTitleLabel->setEnabled(false);
                cycleSendButton->setEnabled(false);
                cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
                cycleSendIntervalValueBox->setEnabled(false);
                cycleSendStatusLabel->setEnabled(false);
                cycleSendStatusLabel->setHidden(true);
                lastSendDescriptionTitleLabel->setEnabled(false);
                lastSendDescriptionTextEdit->setEnabled(false);
                wMaxBase = bcgetmaxbase();
                baseForWorkValueBox->setRange(0, wMaxBase);
                deviceMode = KK_DEVICE_MODE;
            } else if (result == TMK_BAD_FUNC) {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Устройство не поддерживает запрашиваемый режим(КК)!");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            } else {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Не удалось установить запрашиваемый режим(КК)!\nПовторите попытку позже...");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            }
    } else if (QObject::sender() == rtModeSelectButton) {
        result = rtreset();
            if(result == 0) {
                rtModeSelectButton->setStyleSheet("QPushButton{color:green;}");
                rtModeSelectButton->setEnabled(false);
                bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                bcModeSelectButton->setEnabled(true);
                mtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                mtModeSelectButton->setEnabled(true);
                selectBaseForWorkTitleLabel->setEnabled(false);
                baseForWorkValueBox->setEnabled(false);
                selectBaseForWorkButton->setEnabled(false);
                inputYourMessageTitleLabel->setEnabled(false);
                lineSentMessageTextEdit->setEnabled(false);
                sendButton->setEnabled(false);
                addrOYTitleLabel->setEnabled(false);
                addrYOValueBox->setEnabled(false);
                subAddrOYTitleLabel->setEnabled(false);
                subAddrYOValueBox->setEnabled(false);
                readDataFromSubaddrTitleLabel->setEnabled(false);
                dataWordNumberLabel->setEnabled(false);
                dataWordValueBox->setEnabled(false);
                readDataFromSubaddrButton->setEnabled(false);
                readDataTextEdit->setEnabled(false);
                sendStatusLabel->setEnabled(false);
                readStatusLabel->setEnabled(false);
                sendStatusLabel->setHidden(true);
                readStatusLabel->setHidden(true);
                cycleSendTitleLabel->setEnabled(false);
                cycleSendButton->setEnabled(false);
                cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
                cycleSendIntervalValueBox->setEnabled(false);
                cycleSendStatusLabel->setEnabled(false);
                cycleSendStatusLabel->setHidden(true);
                lastSendDescriptionTitleLabel->setEnabled(false);
                lastSendDescriptionTextEdit->setEnabled(false);
                wMaxBase = bcgetmaxbase();
                baseForWorkValueBox->setRange(0, wMaxBase);
                deviceMode = OY_DEVICE_MODE;

            } else if (result == TMK_BAD_FUNC) {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Устройство не поддерживает запрашиваемый режим(ОУ)!");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            } else {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Не удалось установить запрашиваемый режим(ОУ)!\nПовторите попытку позже...");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            }
    } else if (QObject::sender() == mtModeSelectButton) {
        result = mtreset();
            if(result == 0) {
                mtModeSelectButton->setStyleSheet("QPushButton{color:green;}");
                mtModeSelectButton->setEnabled(false);
                rtModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                rtModeSelectButton->setEnabled(true);
                bcModeSelectButton->setStyleSheet("QPushButton{color:black;}");
                bcModeSelectButton->setEnabled(true);
                selectBaseForWorkTitleLabel->setEnabled(true);
                baseForWorkValueBox->setEnabled(true);
                selectBaseForWorkButton->setEnabled(true);
                inputYourMessageTitleLabel->setEnabled(false);
                lineSentMessageTextEdit->setEnabled(false);
                sendButton->setEnabled(false);
                addrOYTitleLabel->setEnabled(false);
                addrYOValueBox->setEnabled(false);
                subAddrOYTitleLabel->setEnabled(false);
                subAddrYOValueBox->setEnabled(false);
                readDataFromSubaddrTitleLabel->setEnabled(false);
                dataWordNumberLabel->setEnabled(false);
                dataWordValueBox->setEnabled(false);
                readDataFromSubaddrButton->setEnabled(false);
                readDataTextEdit->setEnabled(false);
                sendStatusLabel->setEnabled(false);
                readStatusLabel->setEnabled(false);
                sendStatusLabel->setHidden(true);
                readStatusLabel->setHidden(true);
                cycleSendTitleLabel->setEnabled(false);
                cycleSendButton->setEnabled(false);
                cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
                cycleSendIntervalValueBox->setEnabled(false);
                cycleSendStatusLabel->setEnabled(false);
                cycleSendStatusLabel->setHidden(true);
                lastSendDescriptionTitleLabel->setEnabled(false);
                lastSendDescriptionTextEdit->setEnabled(false);
                wMaxBase = bcgetmaxbase();
                baseForWorkValueBox->setRange(0, wMaxBase);
                deviceMode = M_DEVICE_MODE;
            } else if (result == TMK_BAD_FUNC) {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Устройство не поддерживает запрашиваемый режим(МТ)!");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            } else {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Не удалось установить запрашиваемый режим(МТ)!\nПовторите попытку позже...");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
            }
    }
}

void MainWindow::singleSendButtonSlot()
{
    int result = bcdefbus(BUS_A);
    if(result != 0) {
        bcdefbus(BUS_B);
    }
    if(result != 0) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
        disconnectDriverButtonSlot();
        return;
    }

    QStringList dataStringList = lineSentMessageTextEdit->toPlainText().split(";");
    lastSendDescriptionTextEdit->clear();
    wLen = 0;
    for(; wLen < dataStringList.count(); wLen++) {
        if(wLen == (dataStringList.count() - 1)) {
            if(!dataStringList.at(wLen).isEmpty()) {
                if(wLen < 32) {
                    std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
                    iss >> std::hex >> awBuf[wLen];
                } else {
                    qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
                    QMessageBox *msgBox = new QMessageBox(this);
                    msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
                    msgBox->setDefaultButton(QMessageBox::Ok);
                    msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                    msgBox->show();
                    return;
                }
            } else {
                break;
            }
        }
        if(wLen < 32) {
            std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
            iss >> std::hex >> awBuf[wLen];
        } else {
            qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
    }

    if(deviceMode == KK_DEVICE_MODE) {
        RT_ADDR = addrYOValueBox->value();
        wSubAddr = subAddrYOValueBox->value();
        QString logStr;

        /* Пытаемся отправить(десять попыток, если что) */
        int tryNumber = 0;
        bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
        bcputblk(1, awBuf, wLen);
        do
        {
          bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
          if (WaitInt(DATA_BC_RT)) {
            qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
            //_______________________________________________________________________________________________________________
            logStr = "";
            logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
            + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
            + " RESULT: INTERRUPT_ERROR";
            lastSendDescriptionTextEdit->setText(logStr);
            //_______________________________________________________________________________________________________________
            sendStatusLabel->setEnabled(true);
            sendStatusLabel->setText(statusList.at(1));
            sendStatusLabel->setStyleSheet("QLabel{color:red;}");
            sendStatusLabel->setHidden(false);
            sendStatusLabel->setToolTip("Ответ от указанного ОУ не получен!\nЗначение tmkError == " + QString::number(tmkError) +
                                        "\nПроверьте введенные вами значения и попытайтесь снова...");
            break;
          }
          if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
              //_______________________________________________________________________________________________________________
              logStr = "";
              logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
              + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
              + " RESULT: OK";
              lastSendDescriptionTextEdit->setText(logStr);
              //_______________________________________________________________________________________________________________
              sendStatusLabel->setEnabled(true);
              sendStatusLabel->setText(statusList.at(0));
              sendStatusLabel->setStyleSheet("QLabel{color:green;}");
              sendStatusLabel->setHidden(false);
              sendStatusLabel->setToolTip("Данные успешно переданы в ОУ!");
              break;
          } else {
              qDebug() << tmkError;
              //_______________________________________________________________________________________________________________
              logStr = "";
              logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
              + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
              + " RESULT: WRONG_ADDRESS/SUBADDRESS_OR_ERROR_CONNECT_LINE/DEVICE";
              lastSendDescriptionTextEdit->setText(logStr);
              //_______________________________________________________________________________________________________________
              sendStatusLabel->setEnabled(true);
              sendStatusLabel->setText(statusList.at(1));
              sendStatusLabel->setStyleSheet("QLabel{color:red;}");
              sendStatusLabel->setHidden(false);
              sendStatusLabel->setToolTip("В ответном слове ОУ установлен бит!"
                                              "\nПроверьте адрес и/или подадрес выбранного ОУ, состояние приемо-передающего тракта и попытайтесь снова...");
              break;
          }
          tryNumber++;
        }
        while (tryNumber < 10);
    } else if (deviceMode == OY_DEVICE_MODE) {

    } else if (deviceMode == M_DEVICE_MODE) {

    } else {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось отправить одиночное сообщение!\nПроверьте передаваемые вами значения и попытайтесь снова...");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
    }
}

void MainWindow::selectBaseValueButtonSlot()
{
    unsigned result;
    if(deviceMode == KK_DEVICE_MODE) {
        wMaxBase = bcgetmaxbase();
        wBase = baseForWorkValueBox->value();
        if(wBase > wMaxBase) {
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(false);
            addrYOValueBox->setEnabled(false);
            subAddrOYTitleLabel->setEnabled(false);
            subAddrYOValueBox->setEnabled(false);
            readDataFromSubaddrTitleLabel->setEnabled(false);
            dataWordNumberLabel->setEnabled(false);
            dataWordValueBox->setEnabled(false);
            readDataFromSubaddrButton->setEnabled(false);
            readDataTextEdit->setEnabled(false);
            sendStatusLabel->setEnabled(false);
            readStatusLabel->setEnabled(false);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(false);
            cycleSendButton->setEnabled(false);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
            cycleSendIntervalValueBox->setEnabled(false);
            cycleSendStatusLabel->setEnabled(false);
            cycleSendStatusLabel->setHidden(true);
            lastSendDescriptionTitleLabel->setEnabled(false);
            lastSendDescriptionTextEdit->setEnabled(false);
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось установить номер базы ДОЗУ!\nВыбранное вами значение превышает максимально допустимое...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
        bcreset();
        result = bcdefbase(wBase);
        if(result == 0) {
            inputYourMessageTitleLabel->setEnabled(true);
            lineSentMessageTextEdit->setEnabled(true);
            sendButton->setEnabled(true);
            addrOYTitleLabel->setEnabled(true);
            addrYOValueBox->setEnabled(true);
            subAddrOYTitleLabel->setEnabled(true);
            subAddrYOValueBox->setEnabled(true);
            readDataFromSubaddrTitleLabel->setEnabled(true);
            dataWordNumberLabel->setEnabled(true);
            dataWordValueBox->setEnabled(true);
            readDataFromSubaddrButton->setEnabled(true);
            readDataTextEdit->setEnabled(true);
            sendStatusLabel->setEnabled(true);
            readStatusLabel->setEnabled(true);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(true);
            cycleSendButton->setEnabled(true);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(true);
            cycleSendIntervalValueBox->setEnabled(true);
            cycleSendStatusLabel->setEnabled(true);
            lastSendDescriptionTitleLabel->setEnabled(true);
            lastSendDescriptionTextEdit->setEnabled(true);
        } else /*if(result == BC_BAD_BASE)*/ {
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(false);
            addrYOValueBox->setEnabled(false);
            subAddrOYTitleLabel->setEnabled(false);
            subAddrYOValueBox->setEnabled(false);
            readDataFromSubaddrTitleLabel->setEnabled(false);
            dataWordNumberLabel->setEnabled(false);
            dataWordValueBox->setEnabled(false);
            readDataFromSubaddrButton->setEnabled(false);
            readDataTextEdit->setEnabled(false);
            sendStatusLabel->setEnabled(false);
            readStatusLabel->setEnabled(false);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(false);
            cycleSendButton->setEnabled(false);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
            cycleSendIntervalValueBox->setEnabled(false);
            cycleSendStatusLabel->setEnabled(false);
            cycleSendStatusLabel->setHidden(true);
            lastSendDescriptionTitleLabel->setEnabled(false);
            lastSendDescriptionTextEdit->setEnabled(false);
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось установить номер базы ДОЗУ!\nПроверьте выбранное вами значение и попытайтесь снова...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
    } else if (deviceMode == OY_DEVICE_MODE) {
        //операция не предусмотрена
    } else if (deviceMode == M_DEVICE_MODE) {
        wMaxBase = mtgetmaxbase();
        wBase = baseForWorkValueBox->value();
        if(wBase > wMaxBase) {
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(false);
            addrYOValueBox->setEnabled(false);
            subAddrOYTitleLabel->setEnabled(false);
            subAddrYOValueBox->setEnabled(false);
            readDataFromSubaddrTitleLabel->setEnabled(false);
            dataWordNumberLabel->setEnabled(false);
            dataWordValueBox->setEnabled(false);
            readDataFromSubaddrButton->setEnabled(false);
            readDataTextEdit->setEnabled(false);
            sendStatusLabel->setEnabled(false);
            readStatusLabel->setEnabled(false);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(false);
            cycleSendButton->setEnabled(false);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
            cycleSendIntervalValueBox->setEnabled(false);
            cycleSendStatusLabel->setEnabled(false);
            cycleSendStatusLabel->setHidden(true);
            lastSendDescriptionTitleLabel->setEnabled(false);
            lastSendDescriptionTextEdit->setEnabled(false);
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось установить номер базы ДОЗУ!\nВыбранное вами значение превышает максимально допустимое...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
        result = mtdefbase(wBase);
        if(result == 0) {
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(true);
            addrYOValueBox->setEnabled(true);
            subAddrOYTitleLabel->setEnabled(true);
            subAddrYOValueBox->setEnabled(true);
        } else /*if(result == BC_BAD_BASE)*/ {
            inputYourMessageTitleLabel->setEnabled(false);
            lineSentMessageTextEdit->setEnabled(false);
            sendButton->setEnabled(false);
            addrOYTitleLabel->setEnabled(false);
            addrYOValueBox->setEnabled(false);
            subAddrOYTitleLabel->setEnabled(false);
            subAddrYOValueBox->setEnabled(false);
            readDataFromSubaddrTitleLabel->setEnabled(false);
            dataWordNumberLabel->setEnabled(false);
            dataWordValueBox->setEnabled(false);
            readDataFromSubaddrButton->setEnabled(false);
            readDataTextEdit->setEnabled(false);
            sendStatusLabel->setEnabled(false);
            readStatusLabel->setEnabled(false);
            sendStatusLabel->setHidden(true);
            readStatusLabel->setHidden(true);
            cycleSendTitleLabel->setEnabled(false);
            cycleSendButton->setEnabled(false);
            cycleSendIntervalValuesBoxTitleLabel->setEnabled(false);
            cycleSendIntervalValueBox->setEnabled(false);
            cycleSendStatusLabel->setEnabled(false);
            cycleSendStatusLabel->setHidden(true);
            lastSendDescriptionTitleLabel->setEnabled(false);
            lastSendDescriptionTextEdit->setEnabled(false);
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Не удалось установить номер базы ДОЗУ!\nПроверьте выбранное вами значение и попытайтесь снова...");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
    } else {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось установить номер базы ДОЗУ!\nПроверьте выбранное вами значение и попытайтесь снова...");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
    }
}

void MainWindow::readDataFromSubAddrServentDeviceSlot()
{
    int result = bcdefbus(BUS_A);
    if(result != 0) {
        bcdefbus(BUS_B);
    }
    if(result != 0) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
        disconnectDriverButtonSlot();
        return;
    }

    if(deviceMode == KK_DEVICE_MODE) {
        RT_ADDR = addrYOValueBox->value();
        wSubAddr = subAddrYOValueBox->value();
        wLen = dataWordValueBox->value();

        /* Пытаемся получить ответ от ОУ до тех пор, пока не получится */
        int tryNumber = 0;
        bcputw(0, CW(RT_ADDR, RT_TRANSMIT, wSubAddr, wLen));
        do
        {
          bcstartx(wBase, DATA_RT_BC | CX_STOP | CX_BUS_A | CX_NOSIG);
          if (WaitInt(DATA_RT_BC)) {
            qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
            readStatusLabel->setEnabled(true);
            readStatusLabel->setText(statusList.at(1));
            readStatusLabel->setStyleSheet("QLabel{color:red;}");
            readStatusLabel->setHidden(false);
            readStatusLabel->setToolTip("Ответ от указанного ОУ не получен!\nЗначение tmkError == " + QString::number(tmkError) +
                                        "\nПроверьте запрашиваемые вами значения и попытайтесь снова...");
            break;
          }
          if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
              readStatusLabel->setEnabled(true);
              readStatusLabel->setText(statusList.at(2));
              readStatusLabel->setStyleSheet("QLabel{color:green;}");
              readStatusLabel->setHidden(false);
              readStatusLabel->setToolTip("Данные из подадреса ОУ успешно получены!");
              /* Check data from RT */
              QString receivedData = "READ: ";
              bcgetblk(2, awBuf, wLen);
              for (int i = 0; i < wLen; ++i)
              {
                if (awBuf[i] != ((wSubAddr<<8) | i)) {
                    qDebug() << "\nCW:" + QString::number(bcgetw(0)) + " Data error " + "[" + QString::number(i) + "]" + " = " + QString::number(awBuf[i]);
                }
                std::ostringstream os;
                os << std::hex << awBuf[i];
                if(i != (wLen - 1)) {
                    receivedData.append(QString::fromStdString(os.str()) + ";");
                } else {
                  receivedData.append(QString::fromStdString(os.str()));
                }
              }
              readDataTextEdit->clear();
              readDataTextEdit->setText(receivedData);
              break;
          } else {
              qDebug() << tmkError;
              readStatusLabel->setEnabled(true);
              readStatusLabel->setText(statusList.at(3));
              readStatusLabel->setStyleSheet("QLabel{color:red;}");
              readStatusLabel->setHidden(false);
              readStatusLabel->setToolTip("В ответном слове ОУ установлен бит!\ntmkError = " + QString::number(tmkError) +
                                          "\nПроверьте запрашиваемые вами значения и/или состояние ОУ и попытайтесь снова...");
              break;
          }
          tryNumber++;
        }
        while (tryNumber < 10);
    }
}

void MainWindow::cycleSendProcessButtonSlot()
{
    if(cycleSendButton->text() == cycleSendButtonNameList.at(0)) {
        cycleSendIsActive = true;
        cycleSendButton->setText(cycleSendButtonNameList.at(1));
        selectModeTitleLabel->setEnabled(false);
        bcModeSelectButton->setEnabled(false);
        rtModeSelectButton->setEnabled(false);
        mtModeSelectButton->setEnabled(false);
        waitAnswerIntervalValueBoxTitleLabel->setEnabled(false);
        waitAnswerIntervalValueBox->setEnabled(false);
        setWaitAnswerIntervalButton->setEnabled(false);
        selectBaseForWorkTitleLabel->setEnabled(false);
        baseForWorkValueBox->setEnabled(false);
        selectBaseForWorkButton->setEnabled(false);
        inputYourMessageTitleLabel->setEnabled(false);
        lineSentMessageTextEdit->setEnabled(false);
        sendButton->setEnabled(false);
        addrOYTitleLabel->setEnabled(false);
        addrYOValueBox->setEnabled(false);
        subAddrOYTitleLabel->setEnabled(false);
        subAddrYOValueBox->setEnabled(false);
        readDataFromSubaddrTitleLabel->setEnabled(false);
        dataWordNumberLabel->setEnabled(false);
        dataWordValueBox->setEnabled(false);
        readDataFromSubaddrButton->setEnabled(false);
        readDataTextEdit->setEnabled(false);
        sendStatusLabel->setEnabled(false);
        readStatusLabel->setEnabled(false);
        sendStatusLabel->setHidden(true);
        readStatusLabel->setHidden(true);
        cycleSendStatusLabel->setHidden(false);
        lastSendDescriptionTitleLabel->setEnabled(false);
        lastSendDescriptionTextEdit->setEnabled(false);
        lastSendDescriptionTextEdit->clear();
        cycleSendStatusLabel->setText(statusList.at(4));
        emit startCycleSendProcessSignal();
    } else if (cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
        cycleSendIsActive = false;
        cycleSendButton->setText(cycleSendButtonNameList.at(0));
        selectModeTitleLabel->setEnabled(true);
        bcModeSelectButton->setEnabled(true);
        rtModeSelectButton->setEnabled(true);
        mtModeSelectButton->setEnabled(true);
        waitAnswerIntervalValueBoxTitleLabel->setEnabled(true);
        waitAnswerIntervalValueBox->setEnabled(true);
        setWaitAnswerIntervalButton->setEnabled(true);
        selectBaseForWorkTitleLabel->setEnabled(true);
        baseForWorkValueBox->setEnabled(true);
        selectBaseForWorkButton->setEnabled(true);
        inputYourMessageTitleLabel->setEnabled(true);
        lineSentMessageTextEdit->setEnabled(true);
        sendButton->setEnabled(true);
        addrOYTitleLabel->setEnabled(true);
        addrYOValueBox->setEnabled(true);
        subAddrOYTitleLabel->setEnabled(true);
        subAddrYOValueBox->setEnabled(true);
        readDataFromSubaddrTitleLabel->setEnabled(true);
        dataWordNumberLabel->setEnabled(true);
        dataWordValueBox->setEnabled(true);
        readDataFromSubaddrButton->setEnabled(true);
        readDataTextEdit->setEnabled(true);
        sendStatusLabel->setEnabled(true);
        readStatusLabel->setEnabled(true);
        sendStatusLabel->setHidden(true);
        readStatusLabel->setHidden(true);
        cycleSendStatusLabel->setHidden(true);
        lastSendDescriptionTitleLabel->setEnabled(true);
        lastSendDescriptionTextEdit->setEnabled(true);
        emit cycleSendProcessFinish();
    }
}

void MainWindow::cycleSendProcessHandlerSlot()
{
    int result = bcdefbus(BUS_A);
    if(result != 0) {
        bcdefbus(BUS_B);
    }
    if(result != 0) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Не удалось пропинговать ни основную, ни резервную ЛПИ");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
        disconnectDriverButtonSlot();
        return;
    }

    QStringList dataStringList = lineSentMessageTextEdit->toPlainText().split(";");
    wLen = 0;
    for(; wLen < dataStringList.count(); wLen++) {
        if(wLen == (dataStringList.count() - 1)) {
            if(!dataStringList.at(wLen).isEmpty()) {
                if(wLen < 32) {
                    std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
                    iss >> std::hex >> awBuf[wLen];
                } else {
                    qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
                    QMessageBox *msgBox = new QMessageBox(this);
                    msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
                    msgBox->setDefaultButton(QMessageBox::Ok);
                    msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                    msgBox->show();
                    return;
                }
            } else {
                break;
            }
        }
        if(wLen < 32) {
            std::istringstream iss(dataStringList.at(wLen).trimmed().toStdString());
            iss >> std::hex >> awBuf[wLen];
        } else {
            qDebug() << "Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]";
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText("Размер отправляемых данных превышает допускаемый для одной транзакции\n[одной транзакцией не более 32-ух 16-битных слов]");
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setWindowModality(Qt::WindowModality::WindowModal);
            msgBox->show();
            return;
        }
    }

    RT_ADDR = addrYOValueBox->value();
    wSubAddr = subAddrYOValueBox->value();
    int tryNumber;
    int timeout = cycleSendIntervalValueBox->value();
    QString logStr;

    if(deviceMode == KK_DEVICE_MODE && RT_ADDR != 0) {
        fileCycleSendLogs.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if(!fileCycleSendLogs.isOpen())
            qDebug() << "Ошибка предварительной очистки файла fileCycleSendLogs.txt...";
        fileCycleSendLogs.close();
        fileCycleSendLogs.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
        if(!fileCycleSendLogs.isOpen())
            qDebug() << "Ошибка открытия файла fileCycleSendLogs.txt...";
        QTextStream stream(&fileCycleSendLogs);

        /* Пытаемся отправить(десять попыток, если что) */
        bcputw(0, CW(RT_ADDR, RT_RECEIVE, wSubAddr, wLen));
        bcputblk(1, awBuf, wLen);
        do
        {
            tryNumber = 0;
            do
            {
              bcstartx(wBase, DATA_BC_RT | CX_STOP | CX_BUS_A | CX_NOSIG);
              qDebug() << tryNumber;
              if (WaitInt(DATA_BC_RT)) {
                qDebug() <<  "\rGood:" +  QString::number(dwGoodStarts) + "Busy:" + QString::number(dwBusyStarts) + "Error:" + QString::number(dwErrStarts) + "Status:" + QString::number(dwStatStarts);
                //_______________________________________________________________________________________________________________
                logStr = "";
                logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
                + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
                + " RESULT: INTERRUPT_ERROR";
                stream << (logStr + "\n");
                //_______________________________________________________________________________________________________________
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setText("Выбранное ОУ не отвечает!\nЗначение tmkError == " + QString::number(tmkError) +
                                "\nПроверьте введенные вами значения и/или номер ОУ и попытайтесь запустить циклическую отправку снова...");
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowModality(Qt::WindowModality::WindowModal);
                msgBox->show();
                cycleSendProcessButtonSlot();
                break;
              }
              if((tmkEvD.bcx.wResultX & (SX_ERR_MASK | SX_IB_MASK)) == 0) {
                  //_______________________________________________________________________________________________________________
                  logStr = "";
                  logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
                  + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
                  + " RESULT: OK";
                  stream << (logStr + "\n");
                  //_______________________________________________________________________________________________________________
                  cycleSendStatusLabel->setEnabled(true);
                  cycleSendStatusLabel->setText(statusList.at(4));
                  cycleSendStatusLabel->setStyleSheet("QLabel{color:green;}");
                  cycleSendStatusLabel->setHidden(false);
                  cycleSendStatusLabel->setToolTip("Данные успешно передаются в ОУ!");
              } else {
                  //_______________________________________________________________________________________________________________
                  logStr = "";
                  logStr = "WRITE: (Addr:" + QString::number(RT_ADDR) + ";" + "SubAddr:" + QString::number(wSubAddr) + ";"
                  + "data:[" + lineSentMessageTextEdit->toPlainText() + "]" + ";" + QDateTime::currentDateTime().toString() + ")"
                  + " RESULT: WRONG_ADDRESS/SUBADDRESS_OR_ERROR_CONNECT_LINE/DEVICE";
                  stream << (logStr + "\n");
                  //_______________________________________________________________________________________________________________
                  cycleSendStatusLabel->setEnabled(true);
                  cycleSendStatusLabel->setText(statusList.at(5));
                  cycleSendStatusLabel->setStyleSheet("QLabel{color:red;}");
                  cycleSendStatusLabel->setHidden(false);
                  cycleSendStatusLabel->setToolTip("В ответном слове ОУ установлен бит!"
                                              "\nПроверьте адрес и/или подадрес выбранного ОУ, состояние приемо-передающего тракта и попытайтесь запустить циклическую отправку снова...");
              }
              sleepCurrentThread(timeout);
              tryNumber++;
            }
            while (tryNumber < 10 && cycleSendIsActive);
        }
        while(cycleSendIsActive);
        fileCycleSendLogs.close();
        if(stream.status() != QTextStream::Ok)
            qDebug() << "File write textStream error...";
    } else {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setText("Циклическая отправка исключает использование группового адреса ОУ!\nПожалуйста, укажите адрес конкретного ОУ и попытайтесь запустить циклическую отправку снова...");
        msgBox->setDefaultButton(QMessageBox::Ok);
        msgBox->setWindowModality(Qt::WindowModality::WindowModal);
        msgBox->show();
        cycleSendProcessButtonSlot();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    if(cycleSendButton->text() == cycleSendButtonNameList.at(1)) {
       cycleSendProcessButtonSlot();
    }
    bcreset();
    CloseHandle(hBcEvent);
    tmkdone(ALL_TMKS);
    TmkClose();
    if(connectionStatusLabel->text() == connectionStatusVariants.at(1))
        connectionUARTButtonSlot();
    this->close();
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
    if(connectionStatusLabel->text() == connectionStatusVariants.at(0)) {
        qint64 baudRate = baudRatesBox->currentText().toInt();
        QString portName = serialPortsBox->currentText();

        uartTransfer = new UartTransfer();
        uartTransfer->init(portName, baudRate);

        if(uartTransfer->isInit()) {
            qDebug() << "UART-соединение активно";
            connect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
            connectButton->setText("отключить");
            connectionStatusLabel->setText(connectionStatusVariants.at(1));
            connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
            sendUARTDataButton->setEnabled(true);
        } else {
            qDebug() << "UART-соединение не активно";
            sendUARTDataButton->setEnabled(false);
            disconnect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
            connectionStatusLabel->setText(connectionStatusVariants.at(0));
            connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
        }
    } else {
        if(uartTransfer != nullptr) {
            disconnect(uartTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
            uartTransfer->~UartTransfer();
            uartTransfer = nullptr;
            connectionStatusLabel->setText(connectionStatusVariants.at(0));
            connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
            connectButton->setText("подключить");
            sendUARTDataButton->setEnabled(false);
        }
    }
}

void MainWindow::updateCOMListSlot(int index) {
    serialPortsBox->blockSignals(true);
    serialPortsBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        serialPortsBox->addItem(info.portName());
    }
    serialPortsBox->setCurrentIndex(index);
    serialPortsBox->blockSignals(false);
}

void MainWindow::receivedDataSlot(QByteArray data)
{
    QString dataStrForView = "<<<:" + QString::fromUtf8(data) + "\n";
    receivedTransmittedUARTDataTextEdit->setText(receivedTransmittedUARTDataTextEdit->toPlainText() + dataStrForView);
    QTextCursor cursor = receivedTransmittedUARTDataTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    receivedTransmittedUARTDataTextEdit->setTextCursor(cursor);
}

void MainWindow::sendByUartDataButtonSlot()
{
    QString dataString = lineDForTransmittedUARTDataTextEdit->toPlainText().trimmed();

    QByteArray ba;
    ba += dataString;

    if(uartTransfer->isInit()) {
        uartTransfer->write(&ba);
        qDebug() << "Transmitted data:" +  QString::fromUtf8(ba);
        QString dataStrForView = ">>>:" + QString::fromUtf8(ba) + "\n";
        receivedTransmittedUARTDataTextEdit->setText(receivedTransmittedUARTDataTextEdit->toPlainText() + dataStrForView);
        QTextCursor cursor = receivedTransmittedUARTDataTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        receivedTransmittedUARTDataTextEdit->setTextCursor(cursor);
    } else {
        qDebug() << "Send error! UART is not initialized..";
    }
}

void MainWindow::clearUARTDataTextEditButtonSlot()
{
    receivedTransmittedUARTDataTextEdit->clear();
}














