/*
 * Copyright (C) 2017 Smirnov Vladimir mapron1@gmail.com
 * Source code licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 or in file COPYING-APACHE-2.0.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.h
 */
#include "CompilerFrontend.h"
#include "SymTable.h"
#include "Parser.h"
#include "Scanner.h"
#include "CodeGenerator.h"
#include "Scanner.h"
#include "StringVisitor.h"
#include "ast.h"

#include <TreeVariant.h>
#include <ByteOrderStream.h>
#include <StadardLibrary.h>
#include <ScriptVM.h>

#include <QString>
#include <QDebug>

#include <iostream>
#include <fstream>
#include <sstream>

struct CompilerFrontendPrivate
{
	PascalLike::Scanner *_scanner;
	PascalLike::Parser *_parser ;
	QByteArray _vmDebug;
	QByteArray _vmStd;
	QByteArray _vmErr;

	OpcodeSequence _output;

	QByteArray _outData;

	TreeVariant _vars;
	TreeVariant _classes;
	TreeVariant _functions;
	TreeVariant _functionsDescr;
	TreeVariant _librariesTexts;
	CodeGenerator* _gen;
	ScriptVM* _vm;

	QHash<QString, QString> _defines;

	AST::CodeMessages _messages;
	int _debugFlags;
	int _executeLimit;
	CompilerFrontend::Semantic _semantic;
};

using namespace PascalLike;
QString CompilerFrontend::preprocess(const QString &data)
{
	QStringList dataLines = data.split(QRegExp("(\r\n|\r|\n)"));
	QString processedData;

	bool skip=false;
	bool ifdef=false;
	int p1, p2;
	foreach (QString line, dataLines)
	{
		while ((p1 = line.indexOf("{$")) > -1)
		{
			p2 = line.indexOf('}',p1);
			QString pragma = line.mid(p1+2, p2-p1-2);
			line.remove(p1, p2-p1+1);
			if (pragma.startsWith("IFDEF ", Qt::CaseInsensitive))
			{
				skip = !d->_defines.contains(pragma.mid(6));
				ifdef = true;
			}
			else if (pragma.startsWith("IFNDEF ", Qt::CaseInsensitive))
			{
				skip = d->_defines.contains(pragma.mid(7));
				ifdef = true;
			}
			else if (pragma.startsWith("ELSE", Qt::CaseInsensitive))
			{
				if (ifdef)
					skip = !skip;
			}
			else if (pragma.startsWith("IFEND", Qt::CaseInsensitive) || pragma.startsWith("ENDIF", Qt::CaseInsensitive))
			{
				skip = false;
				ifdef = false;
			}
		}
		if (skip) line ="";
		line.replace(QChar(0x1A), "");
		processedData.append(line + "\r\n");
	}
	return processedData;
}

CompilerFrontend::CompilerFrontend() : d(new CompilerFrontendPrivate)
{
	d->_messages.clear();
	d->_scanner= new Scanner();
	d->_parser= new PascalLike::Parser(d->_scanner, &d->_messages);
	d->_vm  = new ScriptVM();
	d->_gen = new CodeGenerator(&(d->_messages), d->_vm);
	d->_debugFlags = dNone;
	d->_executeLimit = -1;

	d->_semantic = smPascal;
}

CompilerFrontend::~CompilerFrontend()
{
	delete d->_vm;
	delete d->_gen;
	delete d->_parser;
	delete d->_scanner;

	delete d;// TODO: smartpointers, neh?
}

bool CompilerFrontend::parseText(const QString &text, bool emptyTextIsValid)
{
	if (text.isEmpty()) return emptyTextIsValid;
	TreeVariant dataObjects;

	foreach (const TreeVariant &text, d->_librariesTexts.asList())
		dataObjects.append(text);

	TreeVariant data;
	data["text"] = text;
	dataObjects.append(data);
	return parseDataObjectsList(dataObjects);
}

bool CompilerFrontend::parseDataObjectsList(const TreeVariant &data)
{
	d->_messages.clear();
	d->_gen->clear();
	registerSymTable();
	d->_vm->_startPC = 0;
	QString processedData;
	if (d->_semantic == smPascal){

		OpcodeSequence code;
		int i=0;
		d->_messages.clear();
		foreach (const TreeVariant &scriptPart, data.asList())
		{
			processedData = preprocess(scriptPart["text"].toString());
			if (processedData.trimmed().isEmpty()) continue;

			d->_parser->_currentFile = i++;
			d->_scanner->_buf = processedData.toStdWString();
			d->_scanner->ReInit();
			d->_parser->Parse();
			OpcodeSequence code1 = d->_gen->compile(d->_parser->_pascal );
			if (d->_messages.errorsCount) continue;
			code << code1;
		}
		if (!d->_messages.errorsCount) {
			d->_vm->_code = code;
		}
	}
	else if (d->_semantic == smAssignment){

		processedData = data.asList().first()["text"].toString();
		processedData.replace('#', '_');
		if (!processedData.contains('(')){
			processedData.replace(',','.');
		}
		d->_scanner->_buf = processedData.toStdWString();
		d->_scanner->ReInit();
		AST::assignmentst assignmentst;
		d->_parser->_currentFile = 0;
		d->_parser->ParseAssignment(assignmentst);
		d->_vm->_code = d->_gen->compile(assignmentst );
	}
	if (d->_debugFlags & dAstDump){
		QString yaml;
		d->_parser->_pascal.save().toYamlString(yaml);
		qDebug() << "ast=\n" << yaml.replace("\r", "");
	}

	if (d->_debugFlags & dNameTable)
		d->_gen->_tab->debug();
	QMap<std::string, int> externalAddresses;
	QMap<std::string, int> internalAddresses;

	for (size_t i=0; i< d->_vm->_code.size();i++){
		BytecodeVM &o = d->_vm->_code[i];
		if (o.symbolLabel.size())
			internalAddresses[o.symbolLabel]  = i;
	}

	for (size_t i=0; i< d->_vm->_code.size();i++){
		BytecodeVM &o = d->_vm->_code[i];
		if (o.gotoLabel.size()){
			if (o.op == BytecodeVM::CALL) {
				int address = internalAddresses.value(o.gotoLabel, -1);
				if (address == -1) {
					d->_messages.Error(AST::CodeLocation(), QString("Unresolved function:") + QString::fromStdString(o.gotoLabel));
				}
				o.values[0].setValue(address);
			}
			if (o.op == BytecodeVM::CALLEXT) {
				int address = externalAddresses.value(o.gotoLabel, -1);
				if (address == -1) {
					address = d->_vm->addFunction(o.gotoLabel);
					externalAddresses[o.gotoLabel] = address;
				}
				o.values[0].setValue(address);
			}
		}
	}

	d->_vm->_startPC = internalAddresses.value( d->_gen->_startAddress.toStdString() );
	d->_vm->_isRunnable =  !d->_gen->_startAddress.isEmpty() && internalAddresses.contains(d->_gen->_startAddress.toStdString());

	QStringList dataLines = processedData.split(QRegExp("(\r\n|\r|\n)"));
	int last_line = -2;
	if (d->_debugFlags & dBytecode)
	{
		qDebug() << "start: [" << d->_vm->_startPC << "], " << d->_gen->_startAddress;
		for (size_t i=0;i<d->_vm->_code.size();i++)
		{
			int l = d->_vm->_code[i].line;
			//int c = _vm->_code[i].col;
			if (last_line != l && l >=0){
				last_line = l;
				qDebug() << "";
				qDebug() << QString(">> ") +dataLines.value(last_line -1).trimmed() ;
			}
			qDebug() /*<< qPrintable(lc)*/ << i << ":" << d->_vm->_code[i].ConvertToString().c_str();
		}
	}

	bool res = !d->_messages.errorsCount;
	if (!res) {
		d->_vm->_isRunnable = false;
	}
	if (d->_debugFlags & dCompileMessages){
		foreach (const AST::CodeMessage& msg, d->_messages._messages)
			qWarning() << msg.toString();
	}
	return res;
}

bool CompilerFrontend::pascal2c(const QString &pascalText, QString &ctext)
{
	d->_gen->clear();
	registerSymTable();
	d->_messages.clear();

	d->_parser->_currentFile = 0;
	d->_scanner->_buf = preprocess(pascalText).toStdWString();
	d->_scanner->ReInit();
	d->_parser->Parse();

	auto mes = d->_messages;
	d->_gen->compile( d->_parser->_pascal );
	d->_messages = mes;

	StringVisitor visitor(StringVisitor::otC);
	visitor._codeTypes = d->_gen->_codeTypes;
	ctext = boost::apply_visitor(visitor, d->_parser->_pascal._pascal);

	return !d->_messages.errorsCount;
}

bool CompilerFrontend::run(bool firstRun)
{
	std::ostringstream virtDebug;
	std::ostringstream virtStd;
	std::ostringstream virtErr;

	bool resultStatus = prepareVM(firstRun, virtDebug, virtStd, virtErr);
	d->_vm->_stepLimit = d->_executeLimit;
	if (resultStatus) d->_vm->run();

	d->_vmDebug = QByteArray( virtDebug.str().c_str() );
	d->_vmStd = QByteArray( virtStd.str().c_str() );
	d->_vmErr = QByteArray( virtErr.str().c_str() );

	d->_vm->_debugout = nullptr;
	d->_vm->_errout = nullptr;
	d->_vm->_stdout = nullptr;

	if (d->_vmErr.size()) resultStatus = false;
	return resultStatus;
}

bool CompilerFrontend::isFinished()
{
	return d->_vm->_runState  == ScriptVM::rsFinished;
}

SymTable* CompilerFrontend::getSymTable()
{
	return d->_gen->_tab;
}

void CompilerFrontend::clearBindings()
{
	d->_librariesTexts.clear();
	d->_functions.clear();
	d->_functionsDescr.clear();
	d->_vars.clear();
	d->_classes.clear();
}

void CompilerFrontend::addFuncs(QStringList protos, QString classname)
{
	static QRegExp main("((\\w+)\\.)?(\\w+)\\s*\\(([^)]*)\\)(:(\\w+))?");
	static QRegExp arrs("(\\w+)\\s*\\[(\\d+)\\]");

	foreach (QString prototype, protos)
	{
		if (main.indexIn(prototype)!= -1)
		{
			TreeVariant f;
			QString exprClassname = main.cap(2).trimmed();
			if (!exprClassname.size())
				exprClassname = classname;
			f["className"] = exprClassname;
			f["name"] =main.cap(3).trimmed();
			f["typeName"]  =main.cap(6).trimmed();
			if (exprClassname.size())
			{
				TreeVariant fa;
				fa["name"] = "self";
				fa["ref"] = TreeVariant(true);
				fa["typeName"] = exprClassname;
				f["args"].append( fa );
			}
			foreach (QString arg, main.cap(4).trimmed().split(";", QString::SkipEmptyParts))
			{
				TreeVariant fa;
				arg = arg.toLower();
				if (arg.startsWith("var "))
				{
					fa["ref"] = TreeVariant(true);
					arg = arg.mid(4);
				}
				fa["name"] = arg.split(":").value(0).trimmed();
				QString typeName = arg.split(":").value(1).trimmed();
				fa["typeName"] = typeName;

				if (arrs.indexIn(typeName)!= -1)
				{
					fa["typeName"] = arrs.cap(1);
					fa["arraySize"] = (TreeVariant)arrs.cap(2).toInt();
				}

				f["args"].append( fa );
			}
			d->_functions .append( f );
			d->_functionsDescr[f["name"].toString()] = prototype;
		}
	}
}

void CompilerFrontend::addFuncs(const std::vector<std::string> &protos)
{
	QStringList qprotos;
	for (const auto & p : protos)
		qprotos << QString::fromStdString(p);
	addFuncs(qprotos);
}

void CompilerFrontend::addVars(const QList<QPair<QString, QString> >& vars)
{
	for (int i=0; i < vars.size();i++)
	   d->_vars[ vars[i].first ] = vars[i].second;
}

void CompilerFrontend::addVars(const TreeVariant &vars)
{
	foreach (QString key, vars.keys())
		d->_vars[key] = vars[key];
}

void CompilerFrontend::addClass(QString name, const TreeVariant &fields, const QStringList &methods)
{
	TreeVariant classDescr;
	classDescr["fields"] = fields;
	classDescr["parent"] = "";
	d->_classes[name]= classDescr;
	addFuncs(methods, name);
}

void CompilerFrontend::addClasses(const TreeVariant &classesMap)
{
	foreach (QString key, classesMap.keys())
		d->_classes[key] = classesMap[key];
}

void CompilerFrontend::setLibraries(const TreeVariant &libraries)
{
	d->_librariesTexts = libraries;
}

void CompilerFrontend::bindFuncs(QMap<QString, funCallbackWrapper> funcs)
{
	foreach (QString key, funcs.keys())
		d->_vm->bindFunction(key.toLower().toStdString(), funcs[key]);
}

void CompilerFrontend::bindFunc(QString id, funCallbackWrapper func)
{
	d->_vm->bindFunction(id.toLower().toStdString(),func);
}

QStringList CompilerFrontend::getInternalTypes()
{
	registerSymTable();
	return d->_gen->_tab->getRegisteredTypes();
}

TreeVariant CompilerFrontend::getExternalBindings() const
{
	return d->_gen->_tab->getExternalVarsList();
}

TreeVariant CompilerFrontend::getDeclaredTypes() const
{
	return d->_gen->_tab->getRegisteredTypesMap();
}

TreeVariant CompilerFrontend::getLibraries() const
{
	return d->_librariesTexts;
}

void CompilerFrontend::saveToFile(QString filename)
{
	if (!d->_parser || !d->_vm)
		return;
	std::ofstream out;
	out.open(filename.toUtf8().constData(), std::ios_base::binary);

	ByteOrderDataStreamWriter bo;
	bo << *(d->_vm);
	out.write((const char*)bo.GetBuffer().begin(), bo.GetBuffer().GetSize());

	out.close();
}

QByteArray CompilerFrontend::exportData()
{
	ByteOrderDataStreamWriter bo;
	bo << *(d->_vm);
	return QByteArray((const char*)bo.GetBuffer().begin(), bo.GetBuffer().GetSize());
}

ScriptVM *CompilerFrontend::vm()
{
	return d->_vm;
}

CompilerFrontend::Semantic CompilerFrontend::semantic() const
{
	return d->_semantic;
}

void CompilerFrontend::setSemantic(CompilerFrontend::Semantic s)
{
	d->_semantic = s;
}

void CompilerFrontend::setParserFlag(int flag, bool value)
{
	int flags = d->_gen->_parserOptions;
	if (value) flags |= flag;
	else       flags &= ~flag;
	d->_gen->_parserOptions = flags;
}

CompilerFrontend::DebugFlags CompilerFrontend::debugFlags() const
{
	return CompilerFrontend::DebugFlags(d->_debugFlags);
}

void CompilerFrontend::setDebugFlags(int flags)
{
	d->_debugFlags = flags;
}

void CompilerFrontend::setDebugFlag(int flags, bool value)
{
	if (value)
		d->_debugFlags |= flags;
	else
		d->_debugFlags &= ~flags;
}

void CompilerFrontend::setExecuteLimit(int limit)
{
	d->_executeLimit = limit;
}

QByteArray CompilerFrontend::getOutput(CompilerFrontend::OutChannel channel) const
{
	switch (channel)
	{
		case ocError: return d->_vmErr;
		case ocStd: return d->_vmStd;
		case ocDebug: return d->_vmDebug;
	}
	return QByteArray();
}

TreeVariant CompilerFrontend::functionsDescr() const
{
	return d->_functionsDescr;
}

TreeVariant CompilerFrontend::classesDescr() const
{
	TreeVariant ret;
	foreach (QString key, d->_classes.keys())
	{
		QString comment =  d->_classes[key]["comment"].toString();;
		if (comment.size()) comment = ": " + comment;
		if (key.startsWith("__")) continue;
		ret[key] = key + comment;
	}

	return ret;
}

TreeVariant CompilerFrontend::variables() const
{
	TreeVariant ret;
	foreach (QString key, d->_vars.keys())
		ret[key] = key +": " + d->_vars[key].toString();

	return ret;
}
const AST::CodeMessages &CompilerFrontend::messages() const
{
	return d->_messages;
}

void CompilerFrontend::makeTokenList(const QString &text, QList<AST::TToken> &list)
{
	QString processedData = preprocess(text.toUtf8());
	d->_scanner->_buf = processedData.toStdWString();
	d->_scanner->ReInit();
	d->_parser->MakeTokenList(list);
}

void CompilerFrontend::registerSymTable()
{
	d->_gen->_tab->clear();
	foreach (const QString & className, d->_classes.keys()) {

		const TreeVariant& fields = d->_classes[className]["fields"];
		const QString parentName = d->_classes[className]["parent"].toString();

		TypeDef tmpType(ScriptVariant::T_UNDEFINED, TypeDef::Class);
		foreach (QString fieldName, fields.keys())
		{
			const bool hasType = fields[fieldName].contains("type");
			const QString typeName = hasType ? fields[fieldName]["type"].toString() : fields[fieldName].toString();
			const int size =  hasType ? fields[fieldName]["size"].toInt() : 0;
			PTypeDef ftype = d->_gen->_tab->findType(typeName);

			if (size> 0)
			{
				TypeDef argType;
				argType._category = TypeDef::Array;
				argType._arrayHighBound = size - 1;
				argType._child << ftype;
				ftype = d->_gen->_tab->registerType(argType, true, true);
			}

			tmpType.addField(fieldName, ftype);
		}
		if (parentName.size()){
			tmpType._parent = d->_gen->_tab->findType(parentName);
		}

		PTypeDef classType = d->_gen->_tab->registerType(tmpType, true, true);
		d->_gen->_tab->setNameForType(classType, className);
		d->_gen->_tab->createNewClassObj(className, classType, parentName);
	}

	foreach (const TreeVariant &f, d->_functions.asList())
	{

		SymTable::FunctionRec fun;
		fun._name = f["name"].toString();
		fun._className = f["className"].toString();
		fun._typeName = f["typeName"].toString();
		foreach (const TreeVariant& fa, f["args"].asList()) {
			FuncObj::FunctionArg arg;
			arg._name = fa["name"].toString();
			arg._typeName = fa["typeName"].toString();
			arg._ref = fa["ref"].toBool();
			arg._arraySize = fa["arraySize"].toInt();
			fun._args << arg;
		}

		ClassObj *ownerClassObj = nullptr;
		if (fun._className.size())
			ownerClassObj = d->_gen->_tab->findClass(fun._className);

		d->_gen->_tab->createNewMethodObj(ownerClassObj, fun, true, false, true);
	}
	foreach (QString varName, d->_vars.keys())
	{
		const QString varType = d->_vars[varName].toString();
		PTypeDef type = d->_gen->_tab->findType(varType);
		if ( d->_semantic == smAssignment)
			varName.replace('#', '_');

		VarObj* newVarObj = d->_gen->_tab->createNewExternalVarObj(varName, type);
		newVarObj->changeMemoryAddress(d->_vm->addVariable(varName.toStdString(), newVarObj->getType().getByteSize() ));
	}
}

bool CompilerFrontend::prepareVM(bool firstRun,
								  std::ostringstream &virtDebug,
								  std::ostringstream &virtStd,
								  std::ostringstream &virtErr)
{
	d->_vm->_debugout = &virtDebug;
	d->_vm->_errout = &virtErr;
	d->_vm->_stdout = &virtStd;

	d->_vm->_debugFlags = d->_debugFlags;
	if (d->_debugFlags & dEmergencyMode) {
		d->_vm->_debugout = &std::cout;
		d->_vm->_stdout = &std::cout;
		d->_executeLimit = 1000;
	}
   // d->_vm->_debugFlags |=PascalLikeWrapper::dNone
  //              | PascalLikeWrapper::dOpcode
   //             | PascalLikeWrapper::dStack
	//            | PascalLikeWrapper::dCallStack
	 //           | PascalLikeWrapper::dOperations
			;
	bool result = true;
	if (firstRun)
		SciptRuntimeLibrary::bindAllStandard(d->_vm);
	result = d->_vm->checkExternalReferences();
	if (firstRun)
		d->_vm->initStatic();
	return result;
}


