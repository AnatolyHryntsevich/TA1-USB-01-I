#ifndef INTERFACEPARAMENETSDIALOG_H
#define INTERFACEPARAMENETSDIALOG_H

#include <QDialog>

namespace Ui {
class InterfaceParamenetsDialog;
}

class InterfaceParamenetsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InterfaceParamenetsDialog(QWidget *parent = nullptr);
    ~InterfaceParamenetsDialog();

    typedef enum {
        Russian_language = 0,
        English_language
    } LanguageEnum;

    typedef struct {
        int fontSize;
        LanguageEnum language;
    } InterfaceSettingsStruct;

    void reloadSettingsBeforeView();

signals:
    void setInterfaceSettingsSignal(const InterfaceSettingsStruct newInterfaceSettings);

private slots:
    void on_okButton_clicked();

private:
    Ui::InterfaceParamenetsDialog *ui;
    InterfaceSettingsStruct currentInterfaceSettings;
};

#endif // INTERFACEPARAMENETSDIALOG_H
