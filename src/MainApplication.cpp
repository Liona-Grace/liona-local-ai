#include "MainApplication.h"

MainApplication::MainApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{
    mainWindow_.show();
}
