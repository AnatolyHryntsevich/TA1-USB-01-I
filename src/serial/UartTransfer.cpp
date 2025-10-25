#include "UartTransfer.h"

#include <QObject>
#include <QDebug>
#include <QtEndian>

#include <cstdint>

UartTransfer::UartTransfer(QObject *parent) : QObject(parent)
{
    memset(msgBuf, 0, UART_MAX_MSG_LEN);
    memset(tmpBuf, 0, UART_MAX_MSG_LEN);
}

UartTransfer::~UartTransfer()
{
    if(initFlag) {
        serial.close();
        initFlag = false;
    }
}

void UartTransfer::errorOccurred(QSerialPort::SerialPortError error)
{
    Q_UNUSED(error);
    qDebug() << "Error occured on serial port" << serial.portName() << "Error:" << serial.errorString();
    initFlag = false;
    this->disconnect();
    serial.close();
    emit serialPortError(serial.errorString());
}

void UartTransfer::handleReadyRead()
{
    if(doCheck) {
        int tmp = indexBuffUart;
        indexBuffUart += serial.read(&tmpBuf[indexBuffUart], UART_MAX_MSG_LEN - indexBuffUart);
        QByteArray rrr = QByteArray(&tmpBuf[tmp], indexBuffUart - tmp);
        indexBuffUart = 0;
        emit receivedNewData(rrr);
    }
}

void UartTransfer::processSendData(QByteArray data)
{
    if(!initFlag)
        return;

    qint64 sendedData = writeSR(data.data(), data.size());
    qDebug() << "<<< writeSR >>>" << sendedData;
    // TODO: emit signal with information about written data
}

/**
 * @brief UartTransfer::init Метод инициализации порта UART
 * @param portName Имя устройства
 * @param baudRate Скорость передачи данных, бод
 * @return Результат инициализации
 */
bool UartTransfer::init(const QString &portName, qint32 baudRate) //-V688
{
    try {
        qDebug() << "connecting to " << portName;
        if(initFlag)
            serial.close();
        serial.setPortName(portName);
        serial.setBaudRate(baudRate);
        if(!serial.open(QIODevice::ReadWrite)) {
            throw QString("Couldn't open serial port on device: '%1'").arg(portName);
        }

        initFlag = true;
        this->portName = portName;
        connect(this, &UartTransfer::sendData, this, &UartTransfer::processSendData);
        connect(&serial, &QSerialPort::errorOccurred, this, &UartTransfer::errorOccurred);
        connect(&serial, &QSerialPort::readyRead, this, &UartTransfer::handleReadyRead);
        return initFlag;
    }
    catch (const QString &errorMsg) {
        qDebug() << errorMsg;
        initFlag = false;
        emit serialPortError(errorMsg);
        return initFlag;
    }
}

/**
 * @brief UartTransfer::isInit Метод, который возвращает состояние инициализации порта UART
 * @return Флаг инициализации порта UART
 */
bool UartTransfer::isInit()
{
    return initFlag;
}

/**
 * @brief UartTransfer::setDataBits Метод установки бита данных
 * @param dataBits Бит данных
 * @return Результат установки
 */
bool UartTransfer::setDataBits(QSerialPort::DataBits dataBits)
{
    if(!initFlag)
        return false;

    if(serial.setDataBits(dataBits))
        return true;

    initFlag = false;
    return initFlag;
}

/**
 * @brief UartTransfer::setParity Метод установки четности
 * @param parity Четность
 * @return Результат установки
 */
bool UartTransfer::setParity(QSerialPort::Parity parity)
{
    if(!initFlag)
        return false;

    if(serial.setParity(parity))
        return true;

    initFlag = false;
    return initFlag;
}

/**
 * @brief UartTransfer::setStopBits Метод установки стопового бита
 * @param stopBits Стоповый бит
 * @return Результат установки
 */
bool UartTransfer::setStopBits(QSerialPort::StopBits stopBits)
{
    if(!initFlag)
        return false;

    if(serial.setStopBits(stopBits))
        return true;

    initFlag = false;
    return initFlag;
}

/**
 * @brief UartTransfer::setNumLenByte Метод установки номера байта, который
 * содержит информацию о длине пакета
 * @param byteNum Номер байта (нумерация начиная с нуля)
 */
void UartTransfer::setNumLenByte(int byteNum)
{
    if(byteNum == 2 || byteNum == 3)
        numLenByte = byteNum;
}

/**
 * @brief UartTransfer::write Метод записи данных по UART без привязки к протоколам БППА
 * @param data Указатель на данные для отправки по UART
 * @param len Длина отправляемых данных
 * @return Количество записанных байт
 */
qint64 UartTransfer::write(const char *data, qint64 len)
{
    if(!initFlag) {
        qDebug() << "Serial port wasn't initialized, cant write data to it";
        return 0;
    }

    qint64 res = serial.write(data, len);
    if(serial.flush())
        return res;

    return 0;
}

qint64 UartTransfer::write(const QByteArray *data)
{
    if(!initFlag) {
        qDebug() << "Serial port wasn't initialized, cant write data to it";
        return 0;
    }
    qint64 res = serial.write(*data);
    if(serial.flush())
        return res;

    return 0;
}

/**
 * @brief UartTransfer::writeSR Метод отправки данных по UART с привязкой к протоколам БППА.
 * (все встречающиеся байты 0x77 будут заменены на 0x77, 0x00)
 * @param data Указатель на данные для отправки по UART
 * @param len Длина отправляемых данных
 * @return Количество записанных байт
 */
qint64 UartTransfer::writeSR(char *data, qint64 len)
{
    if(!initFlag) {
        qDebug() << "Serial port wasn't initialized, cant write data to it";
        return 0;
    }

    if(len < 2)
        return 0;

    int bufLen = sizeof(sendBuf);

    if(len > bufLen)
        return 0;

    memcpy(sendBuf, data, (unsigned int)len);

    int countWords = 0;
    int countWritedWords = 0;
    for(int i = 0; i < len; i++)
    {
        if(i == 0 && sendBuf[i] == 0x77 && sendBuf[i + 1] == 0x21)
            continue;
        if(sendBuf[i] == 0x77)
            countWords++;
    }

    if(countWords > 0)
    {
        int newLen = len + countWords;
        if(newLen > bufLen)
            return 0;
        for(int i = 0; i < len; i++)
        {
            if(i == 0 && sendBuf[i] == 0x77 && sendBuf[i + 1] == 0x21)
                continue;
            else if(sendBuf[i] == 0x77)
            {
                if(i + 2 < newLen)
                    memmove(&sendBuf[i + 2], &sendBuf[i + 1], (unsigned int)(len - (i + 1) + countWritedWords));

                sendBuf[i + 1] = 0x00; //-V557
                countWritedWords++;
            }
        }
        len = newLen;
    }
    return write(sendBuf, len);
}

/**
 * @brief UartTransfer::read Метод чтения данных по UART без привязки к протоколам БППА
 * @param data Указатель на буфер для записи туда данных, прочитанных по UART
 * @param len Максимальная длина буфера
 * @return Количество записанных байт данных
 */
qint64 UartTransfer::read(char *data, qint64 len)
{
    if(!initFlag) {
        qDebug() << "Serial port wasn't initialized, cant read anything from it";
        return 0;
    }

    return serial.read(data, len);
}

/**
 * @brief UartTransfer::readSR Метод разбора полученного буфера по UART.
 * В результате вызова этого метода, переданный буфер @data будет перезаписан таким образом,
 * что в нем будут находиться прошедшие проверку пакеты БППА один за одним, оставшиеся лишние
 * байты будут занулены.
 * @param data Указатель на буфер для записи туда данных, прочитанных по UART
 * @param len Максимальная длина буфера
 * @return Количество записанных полезный байт данных
 */
qint64 UartTransfer::readSR(char *data, qint64 len)
{
    if(!initFlag) {
        qDebug() << "Serial port wasn't initialized, cant read anything from it";
        return 0;
    }
    return -1;
}
