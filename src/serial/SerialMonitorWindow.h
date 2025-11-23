#pragma once

#include <QWidget>

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

signals:
    void connectionGuiSignal(const bool connected);

public slots:
    void retranslateUiSlot();

private slots:
    void connectionGuiSlot(const bool connected);
    void updatePortNameListSlot(const int index);
    void receivedDataSlot(QByteArray data);

    void on_connectionButton_clicked();
    void on_clearButton_clicked();
    void on_sendButton_clicked();

private:
    Ui::SerialMonitorWindow *ui;
    SerialTransfer *m_serialTransfer;
    QList<QString> m_connectionStatusVariants {tr("Готов"), tr("Не готов"), tr("Ready"), tr("Not ready")};
};
