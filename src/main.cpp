#include <QApplication>
#include "view.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    View v;
    v.setWindowTitle("Raycasting 2 Qt/C++");
    v.show();
    a.exec();
}
