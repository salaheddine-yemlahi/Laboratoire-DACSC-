#include "mainwindowclientconsultationbooker.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindowClientConsultationBooker w(argv[1], argv[2]);
    w.show();
    return a.exec();
}