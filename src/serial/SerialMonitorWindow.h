#pragma once

#include <QWidget>

#define SERIAL_LOG_DATA_LINE_LIMIT 10000

namespace Ui {
class SerialMonitorWindow;
}

class SerialTransfer;

class SerialMonitorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SerialMonitorWindow(QWidget *parent = nullptr);
    ~SerialMonitorWindow();

    void putLogLine(const QString &logData);

signals:
    void connectionGuiSignal(const bool connected);

public slots:
    void retranslateUiSlot();
    void updatePortNameListSlot(const int index);

private slots:
    void connectionGuiSlot(const bool connected);
    void receivedDataSlot(QByteArray data);

    void on_connectionButton_clicked();
    void on_clearButton_clicked();
    void on_sendButton_clicked();

private:
    Ui::SerialMonitorWindow *ui;
    SerialTransfer *m_serialTransfer;
    QList<QString> m_connectionStatusVariants {tr("Готов"), tr("Не готов"), tr("Ready"), tr("Not ready")};
};
