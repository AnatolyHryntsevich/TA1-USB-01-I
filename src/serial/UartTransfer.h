#pragma once

#include <QSerialPort>
#include <QByteArray>

#define UART_MAX_MSG_LEN 1500

class UartTransfer : public QObject
{
    Q_OBJECT

public:
    UartTransfer(QObject *parent = nullptr);
    ~UartTransfer() override;

    bool init(const QString &portName, qint32 baudRate);
    bool isInit();
    bool setDataBits(QSerialPort::DataBits dataBits = QSerialPort::Data8);
    bool setParity(QSerialPort::Parity parity = QSerialPort::NoParity);
    bool setStopBits(QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    void setNumLenByte(int byteNum);

    qint64 write(const char *data, qint64 len);
    qint64 write(const QByteArray *data);
    qint64 writeSR(char *data, qint64 len);

    qint64 read(char *data, qint64 len);
    qint64 readSR(char *data, qint64 len);

signals:
    void receivedNewData(QByteArray data);
    void sendData(QByteArray data);
    void serialPortError(QString errorString);

private:
    QString portName;
    bool doCheck = true;
    bool initFlag = false;
    int numLenByte = 3;
    int readTimeout = 0;
    int poss = 0;
    QSerialPort serial;
    int indexBuffUart = 0;
    int currMsgIndexUart = -1;
    char msgBuf[UART_MAX_MSG_LEN];
    char tmpBuf[UART_MAX_MSG_LEN];
    char sendBuf[UART_MAX_MSG_LEN];

private slots:
    void processSendData(QByteArray data);
    void errorOccurred(QSerialPort::SerialPortError error);
    void handleReadyRead();
};
