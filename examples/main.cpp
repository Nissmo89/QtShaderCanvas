#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // In Qt6, High DPI scaling is enabled by default.
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
