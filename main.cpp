#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowIcon(QIcon("PDMeter.icns"));

    w.show();

    return a.exec();
}
/* Novità versione 5
 * - aggiunte modalità BM1, BM2 e PC1
 */
