#pragma once

#include "MainWindow.h"

#include <QApplication>

class MainApplication final : public QApplication
{
public:
    MainApplication(int& argc, char** argv);

private:
    MainWindow mainWindow_;
};
