#ifndef UARTTRANSFER_H
#define UARTTRANSFER_H

#include <cstdint>
#include <QObject>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QSerialPort>
#include <QTextEdit>

/*#ifdef BPPA_CMD_HEADER
typedef struct {
    uint8_t syncBytes[2];   ///< Байты синхронизации
    uint8_t addr;           ///< Адрес
    uint8_t packetSize;     ///< Размер пакета
    uint8_t packetType;     ///< Тип пакета
    uint8_t packetId;       ///< Идентификатор пакета
} headerBPPA;
#else
typedef struct {
    uint8_t syncBytes[2];           ///< Байты синхронизации
    uint8_t packetSize;             ///< Размер пакета
    uint8_t packetType;             ///< Тип пакета
    uint16_t cyclicPacketNumber;    ///< Циклический номер пакета
} headerBPPA;
#endif*/

#define UART_MAX_MSG_LEN 1500

class UartTransfer : public QObject //QThread
{
    Q_OBJECT
public:
    UartTransfer(QObject *parent = nullptr);
    ~UartTransfer() override;
    //void run() override;

    bool init(const QString &portName, qint32 baudRate);
    bool isInit();
    bool setDataBits(QSerialPort::DataBits dataBits = QSerialPort::Data8);
    bool setParity(QSerialPort::Parity parity = QSerialPort::NoParity);
    bool setStopBits(QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    //void setReadTimeout(int msecs);
    void setNumLenByte(int byteNum);

    qint64 write(const char *data, qint64 len);
    qint64 write(const QByteArray *data);
    qint64 writeSR(char *data, qint64 len);

    qint64 read(char *data, qint64 len);
    //qint64 readSR(char *data, qint64 len);
    qint64 readSR(char *data, qint64 len);
    QString printHexValue(const char *data, qint64 len);

    static unsigned short crc16(unsigned char *pcBlock, unsigned short len);
    static void setCrc16(char *packet, unsigned int packetLen);

    bool getDoCheck() const;
    void setDoCheck(bool value);

    uint16_t net_convert16(uint16_t val);
    bool checkCrc(char *buf, uint16_t len);
    int checkBuff(char *data, qint64 len);

signals:
    void receivedNewData(QByteArray data); ///< Сигнал, эмитируемый при получении данных по UART
    void sendData(QByteArray data); ///< Сигнал, эмитируемый пользователем для отправки данных по UART
    void serialPortError(QString errorString); ///< Сигнал, эмитируемый при получении ошибки на порте UART

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

    bool checkLen(char *buf, uint16_t bufLen);

private slots:
    void processSendData(QByteArray data);
    void errorOccurred(QSerialPort::SerialPortError error);
    void handleReadyRead();
};

#endif // UARTTRANSFER_H
