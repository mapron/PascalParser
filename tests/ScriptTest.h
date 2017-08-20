#pragma once

#include <QObject>
#include <QSet>

class CompilerFrontend;
class ScriptTest : public QObject
{
	Q_OBJECT
public:
	explicit ScriptTest(QObject *parent = 0);
	~ScriptTest();
private slots:
	void test1();
	void test1a();
	void test1b();
	void test2();
	void test2a();
	void testString();
	void testConvert();
	void classesScopesTest();
	void classesFieldsAndMembersTest();
	void minimum();
	void nbody();
	void cycles();
	void forwardDeclaration();

	void expr();
	void expr_data();

	void pascal_with();
	void pascal_pointers();
	void pascal_breakContinue();

	void ast_test();


	void init();


private:
	bool _PASCAL_PARSE(QString name);
	bool _VM_RUN();
	bool _DO_SKIP(const QString &name);
	CompilerFrontend* _parser;
	QSet<QString> _skip;
	QSet<QString> _run;
	bool _qdebugStdoutput;
	bool _qdebugDebugoutput;
	bool _firstRun;
	void parserOutput();
	QString testFile(QString name, QString folder = "pascal");


};

