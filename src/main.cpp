#include "mymainwindow.h"
#include <QApplication>
#include <QtGlobal>
#include <QUnhandledException>

#include <iostream>

#include "sodium.h"

int main(int argc, char *argv[])
{
	// initialize libsodium
	if(sodium_init() == -1)
		return 1;

	QApplication a(argc, argv);

	MyMainWindow w;
	w.session();

	return a.exec();
}
