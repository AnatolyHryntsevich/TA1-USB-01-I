#include "MainWindow.h"

#include <QApplication>
//#include <QTranslator>
//#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    QTranslator translator;
//    QSettings settings;
//    QString lang = settings.value("language", "en").toString();

//    if (lang == "ru") {
//        translator.load("translations/app_ru.qm");
//        a.installTranslator(&translator);
//    }

    MainWindow w;
    w.show();
    return a.exec();
}
