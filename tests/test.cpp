#include <QCoreApplication>
#include <QTimer>
#include <QtTest/QTest>

#include "ScriptTest.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

int main(int argc, char *argv[])
{

	QCoreApplication application( argc, argv );

	QTest::qExec(new ScriptTest, argc, argv);

	QTimer::singleShot(100, &application, SLOT(quit()));
	return application.exec();
}
