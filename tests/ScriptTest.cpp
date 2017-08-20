
#include "ScriptTest.h"
#include <QtTest/QTest>
#include <BytecodeVM.h>
#include <StadardLibrary.h>
#include <ScriptVM.h>

#include <CompilerFrontend.h>
#include <ast.h>
#include <QDebug>
#include <TreeVariant.h>
#include <functional>

using namespace PascalLike;
using namespace QTest;
ScriptTest::ScriptTest(QObject *parent)
	:QObject(parent)
{
	_qdebugDebugoutput = true;
	_qdebugStdoutput = true;

	_parser = new CompilerFrontend();
	_parser->setDebugFlags(CompilerFrontend::dNone
//            | PascalLikeWrapper::dOpcode
//            | PascalLikeWrapper::dStack
//            | PascalLikeWrapper::dExternalVars
//            | PascalLikeWrapper::dStaticVars
//            | PascalLikeWrapper::dCallStack
//            | PascalLikeWrapper::dOperations

//            | PascalLikeWrapper::dAstDump
//            | PascalLikeWrapper::dNameTable
//            | PascalLikeWrapper::dBytecode
			| CompilerFrontend::dCompileMessages

//            | PascalLikeWrapper::dEmergencyMode
			);


	_skip
			<< ""
   //         << "all"
			   ;

	_run
			<< ""
 //          << "dynamic_array"
 //          << "dynamic_map"
//             << "with"
			   ;
}

ScriptTest::~ScriptTest()
{

}

void ScriptTest::init()
{
	_parser->clearBindings();
	_parser->addFuncs( SciptRuntimeLibrary::allStandardProtoTypes());
	_firstRun = true;
}
#define SKIP_CHECK(name) \
	   if (_DO_SKIP(name)) return;

//QSKIP(name, SkipAll);

#define PASCAL_PARSE(name) \
	SKIP_CHECK(name);\
	QVERIFY2(_PASCAL_PARSE(name), name)

#define ST_PARSE(name) \
	SKIP_CHECK(name);\
	QVERIFY2(_ST_PARSE(name), name)

#define ST_PARSE_INVALID(name) \
	SKIP_CHECK(name);\
	QVERIFY2(_ST_PARSE_INVALID(name), name)

#define VM_RUN \
	QVERIFY(_VM_RUN())

#define QCOMPARE_OUT(output) QCOMPARE(QString::fromUtf8(_parser->getOutput()), QString(output));

bool ScriptTest::_PASCAL_PARSE(QString name)
{
	if (_skip.contains(name)) return false;
	_parser->setSemantic( CompilerFrontend::smPascal );
	return _parser->parseText(testFile(name, "pascal"), false);
}

bool ScriptTest::_VM_RUN()
{
	bool resultRun =_parser->run(_firstRun);
	_firstRun = false;
	if (!resultRun) qDebug() << _parser->getOutput(CompilerFrontend::ocError);\
	parserOutput();
	return resultRun;
}

bool ScriptTest::_DO_SKIP(const QString &name)
{
	if (_run.contains(name)) return false;
	if (_skip.contains("all")) return true;
	return _skip.contains(name);
}

void ScriptTest::parserOutput()
{
	QByteArray out;
	if (_qdebugDebugoutput && _parser->getOutput(CompilerFrontend::ocDebug).size())
		out += QByteArray("DEBUG:\n") + _parser->getOutput(CompilerFrontend::ocDebug);
	if (_qdebugStdoutput && _parser->getOutput(CompilerFrontend::ocStd).size())
		out += QByteArray("STD:\n") + _parser->getOutput(CompilerFrontend::ocStd);
	if (_parser->getOutput(CompilerFrontend::ocError).size())
		out += QByteArray("ERRORS:\n") + _parser->getOutput(CompilerFrontend::ocError) ;

	if (!out.isEmpty())
	 qDebug().nospace() << "\n" << out.constData();
}


struct TestVarTable
{
	 QList<QPair<QString, QString> > idents;
	 struct VarRecord {
		   QString name;
		   std::vector<ScriptVariant> data;
	 };
	 QList<VarRecord> records;

		template<class T>
		void addValue(QString name, QString type, const T& val){

			idents.push_back(QPair<QString, QString>(name, type));
			VarRecord r;
			ScriptVariant v;
			v.setValue(val, ScriptVariant::string2type(type.toStdString()));
			r.data.push_back(v);
			r.name = name;
			records << r;
		}

		void bindVars(CompilerFrontend* parser){

			for (int i=0;i<records.size();i++){

			   // for (int k=0;k<records[i].data.size();k++)
			   //     dataPtr.push_back(&());
				QString name = records[i].name;
				if (parser->semantic() == CompilerFrontend::smAssignment){
					name.replace('#','_');
				}
				parser->vm()->bindVariable(name.toStdString(), records[i].data);
			}
		}

		ScriptVariant *getVar(QString name, int offset = 0){
			 for (int i=0;i<records.size();i++){
				 if (records[i].name == name){
					 return &(records[i].data[offset]);
				 }
			 }
			 return nullptr;
		}
};


void ScriptTest::test1()
{
	TestVarTable v;
	v.addValue("a", "float64", 0.0);
	v.addValue("b", "float64", 0.5);
	v.addValue("c", "float64", 4.5);

	_parser->addVars(v.idents);

	PASCAL_PARSE("test1");

	v.bindVars(_parser);
	VM_RUN;
	float a = v.getVar("a")->getValue<float>();

	QCOMPARE(a, float(5.0));

}

void ScriptTest::test1a()
{
	PASCAL_PARSE("test1a");
	VM_RUN;
}


void ScriptTest::test1b()
{

	QStringList  CommonFunctions;
	CommonFunctions << "sum(a:int;b:int):int"
					<< "inc1(var a:int)"
					<< "inc2(a:int)"
					   ;
	_parser->addFuncs(CommonFunctions);
	PASCAL_PARSE("test1b");

	QMap<QString,  ScriptVM::FuncNameRecord::funCallback  > funcs;
	funcs["sum"] = []( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		// arg[0] - Return value of function = Result.
		// arg[1] - first arg  etc.
		result[0]->setValue(args[0]->getValue<int>() + args[1]->getValue<int>());
	};

	funcs["inc1"] = [](std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		args[0]->setValue(args[0]->getValue<int>() + 1);
	};
	funcs["inc2"] = []( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		args[0]->setValue(args[0]->getValue<int>() + 1);
	};
	_parser->bindFuncs(funcs);
	VM_RUN;
}

void ScriptTest::test2()
{
	QStringList  CommonFunctions;
	CommonFunctions << "InitSequence(var a:word[20];b:int)"
					<< "OutputSequence(var a:word[20];b:int)";
	_parser->addFuncs(CommonFunctions);
	PASCAL_PARSE("test2");

	QMap<QString,  ScriptVM::FuncNameRecord::funCallback  > funcs;
	funcs["InitSequence"] = []( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		int count = args[1]->getValue<int>() ;
		for (int i=0;i<count;i++)
			args[0]->getReferenced(i)->setValue(700 + i);
	};
	QString seqOut;
	funcs["OutputSequence"] = [&seqOut]
			( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		QStringList r;
		int count = args[1]->getValue<int>() ;
		 for (int i=0;i<count;i++)
			r << QString::number( args[0]->getReferenced(i)->getValue<int>() );
		seqOut = r.join(", ");
	};
	_parser->bindFuncs(funcs);
	VM_RUN;

	QCOMPARE(seqOut, QString("700, 701, 702, 703, 704, 705, 706, 707, 708, 709"));

}

void ScriptTest::test2a()
{

	_parser->addFuncs( QStringList ()<< "ReadModbusRegister(a:string;b:int):word" );
	PASCAL_PARSE("test2a");

	_parser->bindFunc("ReadModbusRegister", []( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args)  {
		result[0]->setValue(42);
	});
	VM_RUN;
}

void ScriptTest::testString()
{
	_parser->addFuncs( QStringList ()<< "ReadModbusRegister(a:string;b:int):word" );
	PASCAL_PARSE("testString");

	_parser->bindFunc("ReadModbusRegister", []( std::vector<ScriptVariant*>  & result, std::vector<ScriptVariant*>  & args ) {
		result[0]->setValue(5);
	});
	VM_RUN;
	QCOMPARE_OUT("five:5 \n");
}

void ScriptTest::testConvert()
{
	PASCAL_PARSE("testConvert");
	VM_RUN;
}

void ScriptTest::classesScopesTest()
{
	PASCAL_PARSE("classesScopesTest");
	VM_RUN;
}

void ScriptTest::classesFieldsAndMembersTest()
{
	PASCAL_PARSE("classesFieldsAndMembersTest");
	VM_RUN;
	QCOMPARE_OUT("545646 \n"
				 "111.111 \n"
				 "0 \n123 \n"
				 "456 \n"
				 "1.5 \n"
				 "123 \n");
}

void ScriptTest::minimum()
{
	PASCAL_PARSE("minimum");
	VM_RUN;
}

void ScriptTest::nbody()
{
	PASCAL_PARSE("nbody");
	VM_RUN;
	QCOMPARE_OUT("-0.169075163828524 \n"
				 "-0.169087605234606 \n");
}

void ScriptTest::cycles()
{

	PASCAL_PARSE("cycles");
	VM_RUN;
}

void ScriptTest::forwardDeclaration()
{
	PASCAL_PARSE("forwardDeclaration");
	VM_RUN;
	QCOMPARE_OUT("a=2 \n");
}


void ScriptTest::expr()
{
	SKIP_CHECK("expr");
	TestVarTable v;
	v.addValue("#2", "float64", 10.0);
	v.addValue("#3", "float64", 16.4);
	v.addValue("#4", "int32", 16);
	v.addValue("#8", "int32", 0);
	v.addValue("#123", "float64", 500);
	v.addValue("result", "float64", 0);

	_parser->addVars(v.idents);
	_parser->setSemantic( CompilerFrontend::smAssignment );

	QFETCH(QString, expression);
	QFETCH(double, result);

	expression = "result:=" + expression;
	bool resultParse =_parser->parseText(expression);

   //  if (!resultParse) qDebug() << _parser->_vmErr;
	QVERIFY(resultParse);

	v.bindVars(_parser);

	VM_RUN;
	double result1 = v.getVar("result")->getValue<double>();
  //  qDebug() << "result1="<<result1;
	QCOMPARE( result1, result);
}

void ScriptTest::expr_data()
{
	QTest::addColumn<QString>("expression");
	QTest::addColumn<double>("result");

	typedef QPair<QString, double> p;
	QList<p > results;

	results << p ("3.6*2", 7.2);
	results << p ("3,6*2", 7.2);
	results << p ("(3+1)", 4.0);
	results << p ("2+3*4-5", 9.0);
	results << p ("2+3*4*(3+1)-5", 45.0);
	results << p ("4*#2", 40.0);
	results << p ("(3+1)*#2", 40.0);
	results << p ("(3+#4)*#2", 190.0);
	results << p ("#2 || #3", 1.0);
	results << p ("#2 && #8", 0.0);    // 0, 8 undefined
	results << p ("#123 && !#8", 1.0);
	results << p ("3 > 0",  1.0);
	results << p ("3 < 0", 0.0);
	results << p ("3 > 8",  0.0);
	results << p ("3 < 8", 1.0);
	results << p ("3 >= 8",  0.0);
	results << p ("8 <= 8", 1.0);
	results << p ("0 >= 0", 1.0);
	results << p ("1 <= 0",  0.0);
	results << p ("(3+1) > 0", 1.0);
	results << p ("(3+1) == 0",  0.0);
	results << p ("(#2+#3) == (#3+#2)", 1.0);
	results << p ("(#2+#3) != (#3+#2)", 0.0);

	results << p ("-#2+#4", 6.0);
	results << p ("3*(-5)", -15.0);

	results << p ("4*#2", 40.0);
	results << p ("(3+1)*#2 ", 40.0);
   // results << p ("(3+#4)*#2 ", 191.0);
	results << p ("(3+#4) * #2 ", 190.0);


	results << p ("#4 & 1 ", 0);
	results << p ("#4 & 16 ", 16);
	results << p ("#4 | 1 ", 17);
	results << p ("#4 | 0 ", 16);
	results << p ("1 << 3 ", 8);
	results << p ("18 >> 3 ", 2);
	results << p ("(~#4) & 255 ", 239);
	results << p ("#4 xor 2 ", 18);
	results << p ("#2 || #3 || #4 ", 1);

//results.clear();
 // results["#4 xor 2" ] = 256;
  //  results["(3+1) == 0"] =  0.0;


	foreach (auto pair, results)
		QTest::newRow(pair.first.toUtf8().constData()) <<  pair.first        <<  pair.second;

}

void ScriptTest::pascal_with()
{
	PASCAL_PARSE("with");

	VM_RUN;

	QCOMPARE_OUT("n[4].y=2 \n");
}

void ScriptTest::pascal_pointers()
{
	PASCAL_PARSE("pointers");

	VM_RUN;

	QCOMPARE_OUT("PX=5 \n"
				 "X[0]=7 \n"
				 "PX2=7 \n"
				 "X[0]=7 \n"
				 "X[1]=8 \n"
				 "point.x=4 \n");
}

void ScriptTest::pascal_breakContinue()
{

	PASCAL_PARSE("breakContinue");

	VM_RUN;

	QCOMPARE_OUT("i=1 \n"
				 "i=2 \n"
				 "i=1 \n"
				 "i=2 \n"
				 "i=4 \n"
				 "i=5 \n");
}

void ScriptTest::ast_test()
{
	return;
	SKIP_CHECK("ast_test");
	AST::ident id_test;
	id_test._loc = AST::CodeLocation(5, 16);
	id_test._ident = "asd";
	TreeVariant data = id_test.save();
	qDebug() << data;
	data["_ident"].setValue(QString("test"));
	id_test.load(data);
	qDebug() << id_test._ident;

	AST::primary ex;
	ex._expr = id_test;

	qDebug() <<  ex.save();


	_PASCAL_PARSE("test1a");

}
//--------------------

QString ScriptTest::testFile(QString name, QString folder)
{
	QFile source(QString(":/ru/%1/%2.pas").arg(folder).arg(name));
	source.open(QIODevice::ReadOnly);
  //  qDebug() << "U:" << source.readAll();
	return QString::fromUtf8(source.readAll());
}
