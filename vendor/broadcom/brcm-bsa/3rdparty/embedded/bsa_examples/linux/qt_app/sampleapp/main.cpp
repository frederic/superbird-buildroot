#include "bsa.h"
#include <QApplication>


int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    BSA w;
    w.show();
    return a.exec();
}
