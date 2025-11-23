#include "SerialMonitorWindow.h"
#include "ui_SerialMonitorWindow.h"

#include <QSerialPortInfo>
#include <QDebug>
#include <QDateTime>

#include "SerialTransfer.h"

SerialMonitorWindow::SerialMonitorWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SerialMonitorWindow),
    m_serialTransfer(nullptr)
{
    ui->setupUi(this);

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) { ui->portNameComboBox->addItem(info.portName()); }
    foreach(int rate, QSerialPortInfo::standardBaudRates()) { ui->baudRateComboBox->addItem(QString::number(rate)); }

    ui->logTextEdit->setUndoRedoEnabled(false);
    ui->logTextEdit->setReadOnly(true);
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(
                QRegularExpression("^([0-9A-Fa-f]{2}( [0-9A-Fa-f]{2})*)?$"),
                parent
                );
    ui->sendInputLineEdit->setValidator(validator);

    connectionGuiSlot(false);

    connect(this, &SerialMonitorWindow::connectionGuiSignal, this, &SerialMonitorWindow::connectionGuiSlot);
    connect(ui->portNameComboBox, QOverload<int>::of(&QComboBox::activated), this, &SerialMonitorWindow::updatePortNameListSlot);
    connect(ui->sendInputLineEdit, &QLineEdit::returnPressed, this, &SerialMonitorWindow::on_sendButton_clicked);
}

SerialMonitorWindow::~SerialMonitorWindow()
{
    delete ui;
    delete m_serialTransfer;
}

void SerialMonitorWindow::retranslateUiSlot()
{
    ui->retranslateUi(this);
    if(ui->clearButton->text() == tr("Clear"))
    {
        if(m_serialTransfer)
        {
            ui->connectionButton->setText(tr("Disable"));
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(2));
            ui->connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
        }
        else
        {
            ui->connectionButton->setText(tr("Connect"));
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(3));
            ui->connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
        }
    }
    else if (ui->clearButton->text() == tr("Очистить"))
    {
        if(m_serialTransfer)
        {
            ui->connectionButton->setText(tr("Отключить"));
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(0));
            ui->connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
        }
        else
        {
            ui->connectionButton->setText(tr("Подключить"));
            ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(1));
            ui->connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
        }
    }
}

void SerialMonitorWindow::connectionGuiSlot(const bool connected)
{
    if(connected)
    {
        ui->connectionButton->setText(tr("Отключить"));
        ui->connectionButton->setToolTip(tr("Деактивировать линию последовательной передачи"));
        ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(0));
        ui->connectionStatusLabel->setStyleSheet("QLabel{color:green;}");
        ui->sendButton->setEnabled(true);
        ui->baudRateComboBox->setEnabled(false);
        ui->portNameComboBox->setEnabled(false);
    }
    else
    {
        ui->connectionButton->setText(tr("Подключить"));
        ui->connectionButton->setToolTip(tr("Активировать линию последовательной передачи"));
        ui->connectionStatusLabel->setText(m_connectionStatusVariants.at(1));
        ui->connectionStatusLabel->setStyleSheet("QLabel{color:red;}");
        ui->sendButton->setEnabled(false);
        ui->baudRateComboBox->setEnabled(true);
        ui->portNameComboBox->setEnabled(true);
    }
    retranslateUiSlot();
}

void SerialMonitorWindow::updatePortNameListSlot(const int index)
{
    ui->portNameComboBox->blockSignals(true);
    ui->portNameComboBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->portNameComboBox->addItem(info.portName());
    }
    ui->portNameComboBox->setCurrentIndex(index);
    ui->portNameComboBox->blockSignals(false);
}

void SerialMonitorWindow::receivedDataSlot(QByteArray data)
{
    QString dataStrForView = "<<<" + QDateTime::currentDateTime().toString("hh:mm:ss") + ": "
            + data.toHex(' ').toUpper() + "\n";
    ui->logTextEdit->setText(ui->logTextEdit->toPlainText() + dataStrForView);
    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}

void SerialMonitorWindow::on_connectionButton_clicked()
{
    if(!m_serialTransfer) {
        qint64 baudRate = ui->baudRateComboBox->currentText().toInt();
        QString portName = ui->portNameComboBox->currentText();

        m_serialTransfer = new SerialTransfer();
        m_serialTransfer->init(portName, baudRate);

        if(m_serialTransfer->isInit()) {
            qDebug() << "Serial port enabled";
            connect(m_serialTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
            emit connectionGuiSignal(true);

        } else {
            if(m_serialTransfer) {
                qDebug() << "Serial port is disabled";
                disconnect(m_serialTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
                m_serialTransfer->~SerialTransfer();
                m_serialTransfer = nullptr;
            }
            emit connectionGuiSignal(false);
        }
        return;
    }

    qDebug() << "Serial port is disabled";
    disconnect(m_serialTransfer, SIGNAL(receivedNewData(QByteArray)), this, SLOT(receivedDataSlot(QByteArray)));
    m_serialTransfer->~SerialTransfer();
    m_serialTransfer = nullptr;
    emit connectionGuiSignal(false);
}


void SerialMonitorWindow::on_clearButton_clicked()
{
    ui->logTextEdit->clear();
}


void SerialMonitorWindow::on_sendButton_clicked()
{
    QString dataString = ui->sendInputLineEdit->text().trimmed();

    if(!dataString.isEmpty())
    {
        QByteArray sendData = QByteArray::fromHex(dataString.remove(' ').toLatin1());

        if(m_serialTransfer)
        {
            if(m_serialTransfer->isInit()) {
                m_serialTransfer->write(&sendData);
                qDebug() << "Transmitted data:" +  sendData.toHex(' ').toUpper();
                QString dataStrForView = ">>>" + QDateTime::currentDateTime().toString("hh:mm:ss") + ": "
                        + sendData.toHex(' ').toUpper() + "\n";
                ui->logTextEdit->setText(ui->logTextEdit->toPlainText() + dataStrForView);
                QTextCursor cursor = ui->logTextEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                ui->logTextEdit->setTextCursor(cursor);
            } else {
                qDebug() << "Send error! SerialTransfer is not initialized..";
            }
        }
    }
}

