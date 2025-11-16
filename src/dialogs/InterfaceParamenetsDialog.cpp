#include "InterfaceParamenetsDialog.h"
#include "ui_InterfaceParamenetsDialog.h"

InterfaceParamenetsDialog::InterfaceParamenetsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InterfaceParamenetsDialog)
{
    ui->setupUi(this);

    ui->fontSizeComboBox->addItems(QString("8 9 10 11 12 13 14 15 16").split(" "));
    ui->fontSizeComboBox->setCurrentIndex(2);
    QApplication::setFont(QFont("Arial", ui->fontSizeComboBox->currentText().toInt()));
    QStringList languagesList {"Russian", "English"};
    foreach(QString language, languagesList)
    {
        ui->languageComboBox->addItem(language);
    }
    ui->languageComboBox->setCurrentIndex(0);

}

InterfaceParamenetsDialog::~InterfaceParamenetsDialog()
{
    delete ui;
}

void InterfaceParamenetsDialog::reloadSettingsBeforeView()
{
    ui->fontSizeComboBox->setCurrentText(QString::number(currentInterfaceSettings.fontSize));
    ui->languageComboBox->setCurrentText(currentInterfaceSettings.language == Russian_language ? "Russian" : "English");
}

void InterfaceParamenetsDialog::on_okButton_clicked()
{
    currentInterfaceSettings.fontSize = ui->fontSizeComboBox->currentText().toInt();
    currentInterfaceSettings.language = ui->languageComboBox->currentText() == "Russian" ? Russian_language : English_language;
    emit setInterfaceSettingsSignal(currentInterfaceSettings);
}

