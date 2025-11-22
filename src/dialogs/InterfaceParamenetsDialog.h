#pragma once

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

    void reloadSettingsUiBeforeView();

    InterfaceSettingsStruct getCurrentInterfaceSettings() const;

signals:
    void setInterfaceSettingsSignal();

public slots:
    void retranslateUiSlot();

private slots:
    void on_okButton_clicked();

private:
    Ui::InterfaceParamenetsDialog *ui;
    InterfaceSettingsStruct m_currentInterfaceSettings;
};
