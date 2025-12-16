#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SynapseVisionLab");
    app.setOrganizationName("SynapseVisionLab");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}