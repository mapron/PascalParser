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

#include "SymTable.h"

#include "Parser.h"
#include "ast.h"
#include "SymTableObjects.h"

#include <TreeVariant.h>

#include <QDebug>

BlockScope::BlockScope()
	: _parent(nullptr)
	, _nextMemoryAddress(0)
{
}

BlockScope::~BlockScope()
{
	qDeleteAll(_objects);
}

BlockScope* BlockScope::createNested(BlockScope *parent)
{
	if (!parent) parent = this;
	_nested << BlockScope(parent);
	return &(_nested.last());
}

bool BlockScope::isRoot() const
{
	return _parent == nullptr;
}

BlockScope* BlockScope::getParent() const
{
	return _parent;
}

BlockScope* BlockScope::getLastAddedNested()
{
	if (!_nested.size()) return nullptr;
	return &_nested.last();
}

VarObj* BlockScope::findVar(const QString &objName) const
{
	return static_cast<VarObj*>(findObj(objName, NamedObj::tVarObj));
}

FuncObj* BlockScope::findFunc(const QString &objName) const
{
	return static_cast<FuncObj*>(findObj(objName, NamedObj::tFuncObj));
}

ClassObj* BlockScope::findClass(const QString &objName) const
{
	return static_cast<ClassObj*>(findObj(objName, NamedObj::tClassObj));
}

NamedObj *BlockScope::findObj(const QString &objName, NamedObj::ObjType objType) const
{
	int varIndex = _objectsNames.value(objName.toLower(), -1);
	NamedObj* obj = _objects.value(varIndex);
	if (objType != NamedObj::tNone) {
		if (obj && obj->getObjType() != objType)
			return nullptr;
	}
	return obj;
}

void BlockScope::registerVariable(VarObj *varPtr)
{
	_nextMemoryAddress += varPtr->getMemorySize();
	registerObject(varPtr);
}

void BlockScope::registerObject(NamedObj *obj)
{
	_objectsNames[obj->getName()] = _objects.size();
	_objects.append(obj);
}


QStringList BlockScope::debug() const
{
	return debug(0);
}

void BlockScope::clear()
{
	qDeleteAll(_objects);
	_objects.clear();
	_objectsNames.clear();
	_nextMemoryAddress = 0;
}

int BlockScope::getNextMemoryAddress() const
{
	return _nextMemoryAddress;
}

int BlockScope::getScopeLevel() const
{
	if (!_parent) return 0;
	else return _parent->getScopeLevel() + 1;
}

QList<const VarObj*> BlockScope::getAllExternal() const
{
	QList<const VarObj*>  ret;
	foreach (const NamedObj* obj, _objects) {
		if (obj->getObjType() == NamedObj::tVarObj){
		   const VarObj* var = static_cast<const VarObj*>(obj);
		   if (var->isExternal()) {
			   ret << var;
		   }
		}
	}
	return ret;
}

QList<const VarObj *> BlockScope::getAllLocal() const
{
	QList<const VarObj*>  ret;
	foreach (const NamedObj* obj, _objects) {
		if (obj->getObjType() == NamedObj::tVarObj){
		   const VarObj* var = static_cast<const VarObj*>(obj);
		   if (!var->isExternal() && !var->isStatic()) {
			   ret << var;
		   }
		}
	}
	return ret;
}

QStringList BlockScope::getContainedNames() const
{
	QStringList result;
	foreach (NamedObj *obj, _objects) result.push_back(obj->getNameOriginal());
	return result;
}

QStringList BlockScope::getFuctionsNames(NamedObj::AccessModifier am) const
{
	QStringList result;
	foreach (NamedObj *obj, _objects) {
		if (obj->getObjType() == NamedObj::tFuncObj) {
			if ((am == NamedObj::amUndefined) || (obj->getAccess() == am)) {
				result.push_back(obj->getNameOriginal());
			}
		}
	}
	return result;
}

QStringList BlockScope::getVariablesNames(NamedObj::AccessModifier am) const
{
	QStringList result;
	foreach (NamedObj *obj, _objects) {
		if (obj->getObjType() == NamedObj::tVarObj) {
			if ((am == NamedObj::amUndefined) || (obj->getAccess() == am)) {
				result.push_back(obj->getNameOriginal());
			}
		}
	}
	return result;
}

BlockScope::BlockScope(BlockScope *parent)
	: _parent(parent)
	, _nextMemoryAddress(0)
{
	assert(parent);
}

QStringList BlockScope::debug(int level) const
{
	QStringList result;
	foreach (NamedObj* obj, _objects) {
		if (obj->getObjType() == NamedObj::tVarObj){
			VarObj* var = static_cast<VarObj*>(obj);
			result << QString("  ").repeated(level) +
					  var->getName() +
					  QString(" v_adr:[%1] size:[%2] sl:%3")
							  .arg(var->getMemoryAddress())
							  .arg(var->getMemorySize())
							  .arg(var->getScopeLevel());
		}
		if (obj->getObjType() == NamedObj::tFuncObj){
			FuncObj* func = static_cast<FuncObj*>(obj);
			result << QString("  ").repeated(level) +
					  func->getName() +
					  QString(" f_adr:[%1] fwd:[%2] arg:[%3] ret:[%4] sl:%5")
							  .arg(func->getFullName())
							  .arg(func->isForward())
							  .arg(func->argumentsSizes())
							  .arg(func->returnSize())
							  .arg(func->getScopeLevel())
					  ;
			if (func->getInternalScope() && func->getInternalScope() != this) {
				QStringList resultNested = func->getInternalScope()->debug(level+1);
				if (resultNested.size()) result << resultNested << "";
			}
		}
		if (obj->getObjType() == NamedObj::tClassObj){
			ClassObj* clas= static_cast<ClassObj*>(obj);
			result << QString("  ").repeated(level) +
					  clas->getName() +
					  QString(" c_adr: sl:%1 size:%2 type:%3")
							  .arg(clas->getScopeLevel())
							  .arg(clas->getType().getType()->getByteSize())
							  .arg(clas->getType().getType()->getTypeDescription());
			QStringList resultNested = clas->getInternalScope()->debug(level+1);
			if (resultNested.size()) result << resultNested << "";
		}

	}
	for (int i=0; i<_nested.size(); i++)
		result << _nested[i].debug(level + 1);

	return result;
}


SymTable::SymTable(AST::CodeMessages *errors)
	: _currentScope(nullptr)
	, _topScope(nullptr)
	, _doAC(false)
	, _hasAC(false)
{
	_errors = errors;
	_topScope = new BlockScope();
	clear();
}

SymTable::~SymTable()
{
	clear(false);
	delete _topScope;
	_topScope = nullptr;
}

void SymTable::clear(bool registerTypes)
{
	_autoCompleteList.clear();
	_hasAC = false;
	_topScope->clear();
	_currentScope = _topScope;
	_scopeStack.clear();
	_lastFuncObj = nullptr;

	_registeredTypes.clear();
	_registeredTypeIndex.clear();
	if (!registerTypes) return;

	QMap<ScriptVariant::Types, QStringList> aliases;
	aliases[ScriptVariant::T_int8_t]      = QStringList() << "char" << "sint" << "int8";
	aliases[ScriptVariant::T_uint8_t]     = QStringList() << "byte" << "usint" << "uint8";
	aliases[ScriptVariant::T_int16_t]     = QStringList() << "smallint" << "int16";
	aliases[ScriptVariant::T_uint16_t]    = QStringList() << "word" << "uint16";
	aliases[ScriptVariant::T_int32_t]     = QStringList() << "int" << "dint" << "integer"<< "longint" << "int32";
	aliases[ScriptVariant::T_uint32_t]    = QStringList() << "longword" << "uint" << "udint" << "uint32";
	aliases[ScriptVariant::T_int64_t]     = QStringList() << "lint"<< "int64";
	aliases[ScriptVariant::T_uint64_t]    = QStringList() << "qword" << "ulint" << "uint64";
	aliases[ScriptVariant::T_float32]     = QStringList() << "float"  << "single" << "float32";
	aliases[ScriptVariant::T_float64]     = QStringList() << "double" << "real" << "lreal" << "float64";
	aliases[ScriptVariant::T_bool]        = QStringList() << "boolean" << "bit"<< "bool";
	aliases[ScriptVariant::T_string]      = QStringList() << "string";
	aliases[ScriptVariant::T_string_char] = QStringList() << "__string_char";

	foreach (ScriptVariant::Types type, aliases.keys()){
		foreach (QString alias, aliases[type]) {
			_registeredTypeIndex[alias] = _registeredTypes.size();
		}
		TypeDef t(type);
		t._userAlias = aliases[type].last();
		_registeredTypes.push_back(t);
	}
}

VarObj* SymTable::createNewRegularVarObj(const QString &name,
										 RefType type,
										 bool isConst)
{
	if (!checkIdent(name)) return nullptr;
	int memoryAddress = _currentScope->getNextMemoryAddress();
	int memorySize = type.getByteSize();
	VarObj* newVar = VarObj::createRegularVar(name,
											  type,
											  memoryAddress,
											  memorySize,
											  isConst);
	newVar->setScopeArguments(_currentScope);
	_currentScope->registerVariable(newVar);
	return newVar;
}

VarObj *SymTable::createNewStaticVarObj(QString name, RefType type)
{
	if (!checkIdent(name))
		return nullptr;

	VarObj* newVar = VarObj::createStaticVar(name,
											 type,
											 0);
	_currentScope->registerVariable(newVar);
	return newVar;
}

VarObj* SymTable::createNewExternalVarObj(const QString &name,
										  RefType type,
										  bool isConst)
{
	if (!checkIdent(name))
		return nullptr;
	VarObj* newVar = VarObj::createExternalVar(name,
											   type,
											   isConst);
	newVar->setScopeArguments(_currentScope);
	_topScope->registerVariable(newVar);

	return newVar;
}

FuncObj* SymTable::createNewMethodObj(ClassObj *ownerClassObj,
									  const SymTable::FunctionRec &prototype,
									  bool isForward,
									  bool isStatic,
									  bool isExternal)
{
	//TODO: check protected/private modifiers.

	assert(!(isStatic && isExternal));
	BlockScope *currentScope  = _currentScope;
	QString classNamePrefix;
	if (ownerClassObj) {
		currentScope = ownerClassObj->getInternalScope();
		classNamePrefix = ownerClassObj->getName().toLower() + "@";
	}

	QString nameLowerCase = prototype._name.toLower();

	FuncObj* funcObj = currentScope->findFunc(nameLowerCase);
	if (funcObj)  {
		if (!funcObj->isForward()  || isForward)
			return nullptr;
	}else{
		if (!checkIdent(nameLowerCase))
			return nullptr;
	}

	if (!funcObj) {


		RefType funcReturnType = prototype._type.isValid()
							   ? prototype._type
							   : findType(prototype._typeName);
		QList<FuncObj::FunctionArg> funcArgs;
		foreach (FuncObj::FunctionArg arg, prototype._args) {
			if (arg._type.isValid()) {
				if (arg._ref) arg._type._isRef = true;
			}
			else {
				arg._type =this->findType(arg._typeName);
				arg._type._isRef = arg._ref;


				if (arg._type._type->isScalar()){
					TypeDef argType;
					argType._category = TypeDef::Scalar;
					argType._opcodeType = arg._type._type->_opcodeType;
					if (arg._arraySize > 0) {
						arg._type._isRef = true;
						argType._category = TypeDef::Array;
						argType._arrayHighBound = arg._arraySize - 1;

						argType._child << this->findType(arg._typeName);

					}
					arg._type._type = this->registerType(argType, true);
				}

			}
			funcArgs << arg;
		}
		FuncObj::Flags flags(FuncObj::fNone);
		if (isStatic)   flags=FuncObj::Flags(flags |  FuncObj::fStatic);
		if (isExternal) flags=FuncObj::Flags(flags |  FuncObj::fExternal);
		if (isForward)  flags=FuncObj::Flags(flags |  FuncObj::fForward);

		funcObj = FuncObj::createRegularFunc(prototype._name,
											 classNamePrefix +  nameLowerCase,
											 funcReturnType,
											 funcArgs,
											 flags);
		currentScope->registerObject(funcObj);
	}
	funcObj->setForward(isForward);
	funcObj->setScopeArguments(currentScope);
	return funcObj;
}

ClassObj *SymTable::createNewClassObj(QString name,
									  RefType type,
									  QString nameParent)
{
	if (!checkIdent(name))
		return nullptr;
	name = name.toLower();
	nameParent = nameParent.toLower();
	ClassObj* existentParent = nullptr;
	if (nameParent.size()) {
		existentParent = findClass(nameParent);
		if (!existentParent) {
			 _errors->Error(AST::CodeLocation(), QString("Undefined parent name: %1").arg(nameParent));
			return nullptr;
		}
	}

	ClassObj *newClass = new ClassObj(name, type.getType(), existentParent);
	newClass->setScopeArguments(_currentScope);
	_currentScope->registerObject(newClass);
	return newClass;
}

VarObj* SymTable::findSelfVar()
{
	return _currentScope->findVar("self");
}

VarObj* SymTable::findVar(const QString &name, BlockScope *workingScope)
{
	NamedObj *obj = findObj(name, workingScope);
	if (obj && obj->getObjType() == NamedObj::tVarObj) {
		 VarObj *resultVar = static_cast<VarObj*>(obj);
		 return resultVar;
	}
	return nullptr;
}

// Returns class object and field offset
VarObj* SymTable::findSelfClassField(const QString &name, int *fieldOffset)
{
	assert(fieldOffset);
	if (!name.size())
		return nullptr;

	VarObj *selfVar = _currentScope->findVar("self");
	if (selfVar) {
		*fieldOffset = selfVar->getOffset(name.toLower());
		if (*fieldOffset < 0)
			return nullptr;
		else
			return selfVar;
	}
	return nullptr;
}

FuncObj* SymTable::findFunc(const QString &name, BlockScope *workingScope)
{
	NamedObj *obj = findObj(name, workingScope);
	if (obj && obj->getObjType() == NamedObj::tFuncObj) {
		 FuncObj *resultFunc = static_cast<FuncObj*>(obj);
		 return resultFunc;
	}
	return nullptr;
}

ClassObj* SymTable::findClass(const QString &name)
{
	NamedObj *obj = findObj(name);
	if (obj && obj->getObjType() == NamedObj::tClassObj) {
		 ClassObj *resultClass = static_cast<ClassObj*>(obj);
		 return resultClass;
	}
	return nullptr;
}

NamedObj* SymTable::findObj(const QString &name, BlockScope *workingScope)
{
	if (!name.size())
		return nullptr;

	BlockScope *currentScope = workingScope ?  workingScope : _currentScope ;
	while (currentScope) {
		NamedObj *resultFunc = currentScope->findObj(name);
		if (resultFunc) {
			return resultFunc;
		}
		if (workingScope) {
			// TODO: this was a protection from calls like a.sin(). Removed for inheritance working...
			// break;
		}
		currentScope = currentScope->getParent();
	}
	return nullptr;
}

void SymTable::openScope (BlockScope *forceScope)
{
	_scopeStack << _currentScope;
	if (forceScope)
		_currentScope = forceScope;
	else
		_currentScope = _currentScope->createNested();
}

void SymTable::closeScope ()
{
	_currentScope = _scopeStack.takeLast();
}

void SymTable::debug()
{
	QStringList d;
	d << (QString)"Symtable:" + QString::number(_topScope->getNextMemoryAddress());
	d << _topScope->debug();
	qDebug() << d.join("\n");
}

const ClassObj *SymTable::getVarClass(const QString &varName)
{
	VarObj *var = this->findVar(varName,  _topScope->getLastAddedNested() );
	if (var && var->getType().getType()->isClass())
		return _topScope->findClass(var->getType().getType()->_userAlias);

	return nullptr;
}

BlockScope *SymTable::getCurrentScope()
{
	return _currentScope;
}

BlockScope *SymTable::getTopScope()
{
	return _topScope;
}

PTypeDef SymTable::getUndefinedType() const
{
	return &_und;
}

PTypeDef SymTable::findType(QString typeName) const
{
	if (!typeName.size()) return &_und;
	typeName = typeName.toLower();
	int index =  _registeredTypeIndex.value(typeName, -1);
	if (index >= 0) return &_registeredTypes[index];
	return &_und;
}

PTypeDef SymTable::registerType(const TypeDef &tmpType, bool autoAppend, bool externalType)
{
	for(int i = 0; i < _registeredTypes.size(); i++)
	{
		if( _registeredTypes[i].equalTo(tmpType) )
			return &_registeredTypes[i];
	}
	if (autoAppend) {
		int i = _registeredTypes.size();
		_registeredTypes.push_back(tmpType);
		_registeredTypes[i]._isExternal = externalType;
		return &_registeredTypes[i];
	}
	return  &_und;
}

bool SymTable::setNameForType(PTypeDef type, QString name)
{
	name = name.toLower();
	for(int i = 0; i < _registeredTypes.size(); i++)
	{
		if( &_registeredTypes[i]== type)
		{
			_registeredTypes[i]._userAlias = name;
			_registeredTypeIndex[name] = i;
			return true;
		}
	}
	return true;
}

QStringList SymTable::getRegisteredTypes() const
{
	return _registeredTypeIndex.keys();
}

TreeVariant SymTable::getRegisteredTypesMap() const
{
	TreeVariant ret;
	for(int i = 0; i < _registeredTypes.size(); i++)
	{
		const TypeDef &t =_registeredTypes[i];
		if (t.isClass() && !t._isExternal)
		{
			TreeVariant classDescr;
			for (int j=0; j< t._child.size(); j++)
			{
				TreeVariant classDescrField;
				classDescrField["type"] = t._child[j]->_userAlias;
				if (t._child[j]->getArraySize() > 0)
				{
					classDescrField["type"] = t._child[j]->_child[0]->_userAlias;
					classDescrField["size"] = TreeVariant( t._child[j]->getArraySize() );
				}
				const QString fieldName = t._childNamesArray[j];
				classDescr["fields"][fieldName]=classDescrField;
			}
			ret[t._userAlias] = classDescr;
		}
	}
	return ret;
}

TreeVariant SymTable::getExternalVarsList() const
{
	TreeVariant ret;
	foreach (const VarObj* var,  _topScope->getAllExternal()) {
		TreeVariant varRecord;
		varRecord["comment"] = var->getComment();
		ret[var->getName()] = varRecord;
	}
	return ret;
}

bool SymTable::checkIdent(QString name)
{
	if (findObj(name))
	{
		_errors->Error(AST::CodeLocation(), QString("Duplicate ident: %1").arg(name));
		return false;
	}
	return true;
}
