#include "ctviewer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CTViewer w;
    w.show();
    return a.exec();
}
