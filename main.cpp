#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);
        MainWindow window;
        window.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Ошибка", QString("Исключение: %1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Ошибка", "Неизвестная ошибка");
        return 1;
    }
}

