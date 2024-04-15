#include <QDebug>
#include <QtEndian>
#include "uarttransfer.h"

UartTransfer::UartTransfer(QObject *parent) : QObject(parent) //QThread(parent)
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
//    if(doCheck)
//    {
//        /*QByteArray byt = serial.read(UART_MAX_MSG_LEN);
//        qDebug() << printHexValue(byt.data(), byt.size());
//        memcpy(&tmpBuf[indexBuffUart], byt.data(), byt.size());
//        indexBuffUart += byt.size();*/
//        int tmp = indexBuffUart;
//        indexBuffUart += serial.read(&tmpBuf[indexBuffUart], UART_MAX_MSG_LEN - indexBuffUart);
//        QByteArray rrr = QByteArray(&tmpBuf[tmp], indexBuffUart - tmp);
//        //qDebug() << "-- INPUT --" << printHexValue(rrr.data(), indexBuffUart - tmp);
//    }

//    for(;;)
//    {
//        qint64 receivedDataLen = doCheck ? readSR(msgBuf, sizeof(msgBuf)) : read(msgBuf, sizeof(msgBuf));
//        if(receivedDataLen != 0) {
//            emit receivedNewData(QByteArray(msgBuf, static_cast<int>(receivedDataLen)));
//            if(!doCheck)
//                break;
//        }
//        else
//            break;
//    }
    if(doCheck) {
        int tmp = indexBuffUart;
        indexBuffUart += serial.read(&tmpBuf[indexBuffUart], UART_MAX_MSG_LEN - indexBuffUart);
        QByteArray rrr = QByteArray(&tmpBuf[tmp], indexBuffUart - tmp);
        emit receivedNewData(rrr);
    }
}

/*void UartTransfer::run()
{
    char buf[1500];
    qint64 bufLen = sizeof(buf);
    memset(buf, 0, static_cast<size_t>(bufLen));
    qint64 receivedDataLen = 0;

    for(;;) {
        QThread::msleep(1);

        if(isInterruptionRequested())
            return;

        if(initFlag && !serial.isOpen()) {
            qDebug() << "Serial port on device" << serial.portName() << "was closed!";
            initFlag = false;
            serial.close();
        }

        if(!initFlag)
            continue;

        receivedDataLen = readSR(buf, bufLen);
        if(receivedDataLen != 0){
            emit receivedNewData(QByteArray(buf, static_cast<int>(receivedDataLen)));
            memset(buf, 0, static_cast<size_t>(receivedDataLen));
        }
    }
}*/

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
bool UartTransfer::init(const QString &portName, qint32 baudRate)
{
    try {
        qDebug() << "connecting to " << portName;
        if(initFlag)
            serial.close();
        serial.setPortName(portName);
        serial.setBaudRate(baudRate);
        if(!serial.open(QIODevice::ReadWrite))
            throw QString("Couldn't open serial port on device: '%1'").arg(portName);

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

/*void UartTransfer::setReadTimeout(int msecs)
{
    if(msecs >= -1)
        readTimeout = msecs;
}*/

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

    //qDebug() <<  "<<<<<<<< QByteArray >>>>>>>" << QByteArray(data, len);
    //qDebug() <<  "<<<<<<<< QByteArrayHex >>>>>>>" << printHexValue(data, len);

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

                sendBuf[i + 1] = 0x00;
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

/*qint64 UartTransfer::readSR(char *data, qint64 len)
{
    if(!isInit) {
        qDebug() << "Serial port wasn't initialized, cant read anything from it";
        return 0;
    }

    qint64 gotSize = Read(data, len);
    if(gotSize <= 0)
        return gotSize;

    int countWords = 0;
    for(qint64 i = 0; i < gotSize - 1; i++) {
        if(data[i] == 0x77 && data[i + 1] == 0x00)
            countWords++;
    }

    if(countWords > 0) {
        qint64 newLen = gotSize - countWords;
        char *tmpBuf = static_cast<char *>(malloc(static_cast<size_t>(newLen)));
        qint64 dataBufPos = 0, tmpBufPos = 0, lenCopy = 0;
        for(qint64 i = 0; i < gotSize - 1; i++) {
            if(data[i] == 0x77 && data[i + 1] == 0x00) {
                lenCopy = i + 1 - dataBufPos;
                memcpy(&tmpBuf[tmpBufPos], &data[dataBufPos], static_cast<size_t>(lenCopy));
                tmpBufPos += lenCopy;
                dataBufPos = i + 2;
            }
        }
        lenCopy = len - dataBufPos;
        if(lenCopy > 0)
            memcpy(&tmpBuf[tmpBufPos], &data[dataBufPos], static_cast<size_t>(lenCopy));
        memset(data, 0, static_cast<size_t>(gotSize));
        memcpy(data, tmpBuf, static_cast<size_t>(newLen));
        free(tmpBuf);
        gotSize = static_cast<ssize_t>(newLen);
    }

    return gotSize;
}*/

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

    /// Тут проверка на целое сообщение
    /// Было read(data, len);
    return checkBuff(data, len);

    /*if(gotSize <= 0)
        return gotSize;

    if(data[0] == 0x77 && data[1] == 0x21 && data[4] == 0x0d)
        return gotSize;

    //qint64 gotSize = len;
    qint64 prevGotSize = gotSize;

    bool foundSyncBits = false;
    //int numLenBit = 3;
    int startPacketIndex = -1, packetLen = 0, currPacketLen = 0;
    int writedLen = 0;
    for(int i = 0; i < gotSize; i++) {
        if(i != 0 && data[i - 1] == 0x77 && data[i] == 0x00) {
            if(i + 1 < gotSize)
                memcpy(&data[i], &data[i + 1], static_cast<size_t>(gotSize - (i + 1)));
            gotSize--;
        }

        if(i + 1 < gotSize && data[i] == 0x77 && data[i + 1] == 0x21) {
            if(startPacketIndex == -1 && i != 0) {
                memcpy(&data[0], &data[i], static_cast<size_t>(gotSize - i));
                gotSize -= i;
                i = 0;
            }
            if(startPacketIndex != -1 &&
                    ((packetLen != 0 && currPacketLen != 0 && currPacketLen < packetLen) ||
                     (i != 0 && packetLen == 0 && currPacketLen == 0))) {
                //if(packetLen != 0 && currPacketLen != 0 && currPacketLen < packetLen) {
                memcpy(&data[startPacketIndex], &data[i], static_cast<size_t>(gotSize - i));
                gotSize -= i - startPacketIndex;
                i = startPacketIndex;

                if(gotSize - i < numLenByte + 1) {
                    gotSize -= gotSize - i;
                    break;
                }
            }
            foundSyncBits = true;
            startPacketIndex = i;
            packetLen = currPacketLen = 0;
            i++;
            continue;
        }

        if(foundSyncBits) {
            if(packetLen != 0 && packetLen > gotSize - startPacketIndex) {
                gotSize -= gotSize - startPacketIndex;
                break;
            }

            if(packetLen == 0 && numLenByte == i - startPacketIndex) {
                packetLen = data[i];
                currPacketLen = numLenByte;
                if(packetLen < 8 || packetLen > 136) {
                    foundSyncBits = false;
                    packetLen = currPacketLen = 0;
                    continue;
                }
            }

            if(currPacketLen != 0) {
                currPacketLen++;
                if(currPacketLen == packetLen) {
                    //unsigned short *crc = reinterpret_cast<unsigned short *>(&data[i - 1]);
                    unsigned short convertedCrc = 0;
                    char *convertedCrcPtr = reinterpret_cast<char *>(&convertedCrc);
                    memcpy(&convertedCrcPtr[0], &data[i], sizeof(char));
                    memcpy(&convertedCrcPtr[1], &data[i - 1], sizeof(char));
                    unsigned short dataCrc = crc16(reinterpret_cast<unsigned char *>(&data[startPacketIndex + 2]),
                            static_cast<unsigned short>(packetLen - 4));
                    if(dataCrc != convertedCrc) {
                        if(i + 1 < gotSize)
                            memcpy(&data[startPacketIndex], &data[i + 1], static_cast<size_t>(gotSize - i + 1));
                        gotSize -= i + 1 - startPacketIndex;
                        i = startPacketIndex;
                    }
                    else
                        writedLen += packetLen;
                    foundSyncBits = false;
                    packetLen = currPacketLen = 0;
                }
            }
        }
    }

    gotSize = writedLen;

    if(gotSize < prevGotSize)
        memset(&data[gotSize], 0, static_cast<size_t>(prevGotSize - gotSize));

    return gotSize;*/
}

QString UartTransfer::printHexValue(const char *data, qint64 len)
{
    QString line;
    if(len != 0 /*&& portName == "ttyUSB0"*/)
    {
        if(len == 1)
            qDebug() << "";
        for(int i = 0; i < len; i++)
        {
            QString num = QString::number(data[i], 16);

            if(num.size() == 1)
                num.insert(0,'0');

            if(num.size() > 2 && num.contains("f"))
            {
                int c = num.count('f');
                if(c == 1 && num.size() == 3)
                    num.remove("f");
                else if( c > 2) {
                    int del_count = 0;
                    if(num.size() - c == 1)
                        del_count = 1;
                    num.remove(0, c - del_count);
                }
            }

            line += (tr("0x") + num + " ");
        }
    }
    if(line != "")
        return portName + " --- " + line;
    else
        return "";
}

/**
 * @brief UartTransfer::crc16 Метод расчета контрольной суммы (не привязан к структуре пакета БППА)
 * @param pcBlock Указатель на данные для расчета контрольной суммы
 * @param len Длина данных
 * @return Значение расчитанной контрольной суммы
 */
unsigned short UartTransfer::crc16(unsigned char *pcBlock, unsigned short len)
{
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while (len--)
    {
        crc ^= *pcBlock++ << 8;

        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? static_cast<unsigned short>((crc << 1) ^ 0x1021) :
                                 static_cast<unsigned short>(crc << 1);
    }
    return crc;
}

/**
 * @brief UartTransfer::setCrc16 Метод расчета и записи контрольной суммы в пакет БППА.
 * @param packet Указатель на начало пакета (начиная с байтов синхронизации)
 * @param packetLen Длина пакета БППА с учетом заголовка и контрольной суммы (последние 2 байта пакета)
 */
void UartTransfer::setCrc16(char *packet, unsigned int packetLen)
{
    unsigned short crc = UartTransfer::crc16(reinterpret_cast<unsigned char *>(&packet[2]),
            static_cast<unsigned short>(packetLen - 4));
    unsigned short *packetCrc = reinterpret_cast<unsigned short *>(&packet[packetLen - 2]);
    *packetCrc = ((crc << 8) & 0xff00) | ((crc >> 8) & 0x00ff);
    //char *crcPtr = reinterpret_cast<char *>(&crc);
    //char *packetCrcPtr = &packet[packetLen - 2];
    //memcpy(&packetCrcPtr[0], &crcPtr[1], sizeof(char));
    //memcpy(&packetCrcPtr[1], &crcPtr[0], sizeof(char));
}

bool UartTransfer::getDoCheck() const
{
    return doCheck;
}

void UartTransfer::setDoCheck(bool value)
{
    doCheck = value;
}

uint16_t UartTransfer::net_convert16(uint16_t val)
{
    return ((val << 8) & 0xff00) | ((val >> 8) & 0x00ff);
}

bool UartTransfer::checkCrc(char *buf, uint16_t len)
{
    //uint16_t convertedCrc = net_convert16(*((uint16_t *)(buf + len - 2)));
    uint16_t convertedCrc = net_convert16(*(reinterpret_cast<uint16_t*>(&buf[len - sizeof(uint16_t)])));
    uint16_t dataCrc = crc16(reinterpret_cast<unsigned char *>(&buf[2]), (unsigned short)(len - 4));
    //uint16_t dataCrc = crc16((unsigned char *)(buf + 2), (unsigned short)(len - 4));
    if(dataCrc == convertedCrc) {
        return true;
    }
    //qDebug() << "<<<<< convertedCrc >>>>> " << convertedCrc;
    //qDebug() << "<<<<< dataCrc >>>>> " << dataCrc;
    return false;
}

#define SYNCHRO_0 0x77
#define SYNCHRO_1 0x21

int UartTransfer::checkBuff(char *data, qint64 len)
{
    int processed = 0;
    int resProcessed = 0;

    if(indexBuffUart <= 0)
        return 0;

    for(int i = 0; i < indexBuffUart; ++i) {
        if(tmpBuf[i] == SYNCHRO_0) {
            if(i + 1 >= indexBuffUart)
                return 0;
            if(tmpBuf[i + 1] == SYNCHRO_1)
                currMsgIndexUart = 0;
            else if((currMsgIndexUart != -1) && (tmpBuf[i + 1] == 0x00)) {
                data[currMsgIndexUart++] = tmpBuf[i];
                i++;
                continue;
            }
        }
        if(currMsgIndexUart < 0)
            continue;
        data[currMsgIndexUart++] = tmpBuf[i];
        if(checkLen(data, len))
        {
            if(checkCrc(data, (uint16_t)currMsgIndexUart))
            {
                resProcessed = currMsgIndexUart;
                processed = i + 1;
                memmove(tmpBuf, tmpBuf + processed, indexBuffUart - (unsigned int)processed);
                indexBuffUart = indexBuffUart - (uint16_t)processed;
                memset(&tmpBuf[indexBuffUart], 0, processed);
                currMsgIndexUart = -1;
                return resProcessed;
            }
            else
                qDebug() << "SRC" << printHexValue(data, len);

            processed = i + 1;
            currMsgIndexUart = -1;
        }
    }

    if(currMsgIndexUart == -1)
        processed = indexBuffUart;

    if(processed > 0)
    {
        memmove(tmpBuf, tmpBuf + processed, (unsigned int)processed);
        indexBuffUart = indexBuffUart - (uint16_t)processed;
        currMsgIndexUart = -1;
        memset(&tmpBuf[indexBuffUart], 0, processed);
    }

    return resProcessed;
}

bool UartTransfer::checkLen(char *buf, uint16_t bufLen)
{
    return (currMsgIndexUart > numLenByte + 1) && (buf[numLenByte] <= bufLen) && (buf[numLenByte] == currMsgIndexUart);
}
