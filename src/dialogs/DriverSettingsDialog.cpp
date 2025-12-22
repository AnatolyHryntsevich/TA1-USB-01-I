#include "DriverSettingsDialog.h"
#include "ui_DriverSettingsDialog.h"

#ifdef __unix__
#include "ltmk.h"
#endif

#ifdef _WIN32
#define MAX_TMK_NUMBER 71
#endif

DriverSettingsDialog::DriverSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DriverSettingsDialog)
{
    ui->setupUi(this);

    this->setWindowModality(Qt::WindowModality::WindowModal);

    QList<int> devicesNumbersList;
    for(int i = 1; i < MAX_TMK_NUMBER; i++)
    {
        devicesNumbersList << i;
    }
    foreach(int number, devicesNumbersList)
    {
        ui->deviceNumberComboBox->addItem(QString::number(number));
    }
    ui->answerTimeoutComboBox->addItem(QString::number(14));
    ui->answerTimeoutComboBox->addItem(QString::number(18));
    ui->answerTimeoutComboBox->addItem(QString::number(26));
    ui->answerTimeoutComboBox->addItem(QString::number(63));
    ui->workModeComboBox->addItem(tr("КК"));
    ui->workModeComboBox->addItem(tr("ОУ"));
    ui->workModeComboBox->addItem(tr("МТ"));
    ui->workModeComboBox->setCurrentIndex(0);
    ui->workModeComboBox->setEnabled(false);
    ui->memNumberComboBox->addItems(QString("1 2 3 4 5 6 7 8 9 10").split(" "));
}

DriverSettingsDialog::~DriverSettingsDialog()
{
    delete ui;
}

void DriverSettingsDialog::reloadSettingsUiBeforeView()
{
    ui->answerTimeoutComboBox->setCurrentText(QString::number(m_currentDriverSettings.answerWaitTimeout));
    ui->workModeComboBox->setCurrentText(m_currentDriverSettings.workModeName);
    ui->memNumberComboBox->setCurrentText(QString::number(m_currentDriverSettings.memBaseNumber));
    ui->deviceNumberComboBox->setCurrentText(QString::number(m_currentDriverSettings.deviceNumber));
}

void DriverSettingsDialog::setGuiSate(const bool factor)
{
    ui->settingsGroupBox->setEnabled(factor);
}

const DriverSettingsDialog::DriverSettingsStruct &DriverSettingsDialog::currentDriverSettings() const
{
    return m_currentDriverSettings;
}

void DriverSettingsDialog::on_okButton_clicked()
{
    m_currentDriverSettings.deviceNumber = ui->deviceNumberComboBox->currentText().toInt();
    m_currentDriverSettings.memBaseNumber = ui->memNumberComboBox->currentText().toInt();
    m_currentDriverSettings.answerWaitTimeout = ui->answerTimeoutComboBox->currentText().toInt();
    m_currentDriverSettings.workModeName = ui->workModeComboBox->currentText().toInt();
    emit setDriverSettingsSignal(m_currentDriverSettings);
}

void DriverSettingsDialog::retranslateUiSlot()
{
    ui->retranslateUi(this);
}

