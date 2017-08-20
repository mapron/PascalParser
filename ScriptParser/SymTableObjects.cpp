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

#include "SymTableObjects.h"

#include "SymTable.h"

NamedObj::NamedObj(const QString &name,
				   const RefType &type,
				   ObjType objType,
				   NamedObj::Flags flags)
	: _name(name.toLower())
	, _nameOriginal(name)
	, _type(type)
	, _flags(flags)
	, _access(amPublic)
	, _scope(nullptr)
	, _objType(objType)
{}

NamedObj::~NamedObj()
{}

QString NamedObj::getName() const
{
	return _name;
}

QString NamedObj::getNameOriginal() const
{
	return _nameOriginal;
}

QString NamedObj::getComment() const
{
	return _comment;
}

RefType NamedObj::getType() const
{
	return _type;
}

int NamedObj::getScopeLevel() const
{
	return _scope ?  _scope->getScopeLevel() + (_objType == tFuncObj) : 0;
}

NamedObj::AccessModifier NamedObj::getAccess() const
{
	return _access;
}

NamedObj::ObjType NamedObj::getObjType() const
{
	return _objType;
}

bool NamedObj::isConst() const
{
	return _flags & fConst;
}
bool NamedObj::isStatic() const
{
	return _flags & fStatic;
}

bool NamedObj::isExternal() const
{
	return _flags & fExternal;
}

bool NamedObj::isForward() const
{
	return _flags & fForward;
}

bool NamedObj::isUsed()
{
	if (isExternal()) return _flags & fUsed;
	return false;
}

bool NamedObj::setUsed()
{
	if (isExternal()) {
		if (!isUsed()){
			_flags = Flags(_flags |fUsed);
			return true;
		}
	}
	return false;
}

void NamedObj::setScopeArguments(BlockScope *scope)
{
	_scope = scope;
}

void NamedObj::setAccess(AccessModifier access)
{
	_access = access;
}

void NamedObj::setComment(const QString &comment)
{
	_comment = comment;
}

void NamedObj::setForward(bool state)
{
	if (state)
		_flags = Flags(_flags | fForward);
	else
		_flags = Flags(_flags & (~fForward));
}


VarObj::VarObj(const QString &name,
			   const RefType &type,
			   int memoryAddress,
			   int memorySize,
			   Flags flags)
	: NamedObj(name, type, tVarObj, flags)
	, _memoryAddress(memoryAddress)
	, _memorySize(memorySize)

{}

// static
VarObj *VarObj::createRegularVar(const QString &name,
								 const RefType &type,
								 int memoryAddress,
								 int memorySize,
								 bool isConst)
{
	Flags flags(fNone);
	if (isConst) flags = Flags(flags  | fConst);
	return new VarObj(name, type, memoryAddress, memorySize,
					  flags);
}

// static
VarObj *VarObj::createStaticVar(const QString &name,
								const RefType &type,
								int externalMemoryAddress)
{
	return new VarObj(name, type, externalMemoryAddress, 0,
					  fStatic);
}

// static
VarObj *VarObj::createExternalVar(const QString &name,
								  const RefType &type,
								  bool isConst)
{
	Flags flags(fExternal);
	if (isConst) flags = Flags(flags  | fConst);
	return new VarObj(name, type, -1, 0,
					  flags);
}

ScriptVariant::Types VarObj::getOpcodeType() const
{
	return _type.getOpcodeType();
}

int VarObj::getOffset(const QString &name) const
{
	return _type.getOffset(name);
}

int VarObj::getMemoryAddress() const
{
	return _memoryAddress;
}

int VarObj::getMemorySize() const
{
	return _memorySize;
}


PTypeDef VarObj::findField(const QString &fieldName) const
{
	return _type._type->findField(fieldName.toLower());
}

void VarObj::changeMemoryAddress(int newMemoryAddress)
{
	_memoryAddress = newMemoryAddress;
}


FuncObj::FuncObj(const QString &name, const QString &fullName,
				 const RefType &returnType,
				 const QList<FuncObj::FunctionArg> &arguments,
				 Flags flags)
	: NamedObj(name, returnType, tFuncObj, flags)
	, _fullName(fullName)
	, _arguments(arguments)
	, _internalScope(nullptr)


{}

// static
FuncObj *FuncObj::createRegularFunc(const QString &name,
									const QString &fullName,
									const RefType &returnType,
									const QList<FuncObj::FunctionArg> &arguments,
									Flags flags)
{
	return new FuncObj(name, fullName, returnType, arguments, flags);
}




QString FuncObj::getFullName() const
{
	return _fullName;
}

QString FuncObj::getFullSignature() const
{
	QStringList parts;
	foreach (const FunctionArg &arg, _arguments) {
		parts << arg._type._type->getTypeDescription();
	}
	return _name + "(" + parts.join("; ") +");";
}

const BlockScope *FuncObj::getInternalScope() const
{
	if (!_internalScope)
		const_cast<FuncObj*>(this)->_internalScope = _scope->createNested();
	return _internalScope;
}

BlockScope *FuncObj::getInternalScope()
{
	if (!_internalScope)
		_internalScope = _scope->createNested();
	return _internalScope;
}

const QList<FuncObj::FunctionArg> &FuncObj::getArguments() const
{
	return _arguments;
}

int FuncObj::getArgumentsNumber() const
{
	return _arguments.size();
}

QString FuncObj::getReturnTypeName() const
{
	if (_type._type) {
		return _type._type->_userAlias;
	}
	else return "";
}

int FuncObj::returnSize() const
{
	return _type.getByteSize();
}

int FuncObj::callSize() const
{
	int callSize = 0;
	foreach (const FunctionArg &arg, _arguments){
		callSize += arg._type.getByteSize();
	}
	return callSize;
}

QString FuncObj::argumentsSizes() const
{
	QStringList args;
	foreach (const FunctionArg &arg, _arguments){
	   args << QString("%1: %2").arg( arg._typeName).arg( arg._type.getByteSize() );
	}
	return args.join(", ");
}

void FuncObj::setScopeArguments(BlockScope *scope)
{
	_scope = scope;


}

ClassObj::ClassObj(const QString &name,
				   PTypeDef classDefinition,
				   ClassObj *parent)
	: NamedObj(name, classDefinition, tClassObj)
	, _parent(parent)
	, _internalScope(nullptr)
{
}

BlockScope *ClassObj::getInternalScope() const
{
	return _internalScope;
}

void ClassObj::setScopeArguments(BlockScope *scope)
{
	_scope = scope;
	if (!_internalScope){
		BlockScope *internalParent = nullptr;
		if (_parent)
			internalParent = _parent->getInternalScope();
		_internalScope = _scope->createNested(internalParent);
	}
}

void ClassObj::setClassDefinition(PTypeDef classDefinition)
{
	_type = classDefinition;
}

QStringList ClassObj::getMemberList(const QString &hint) const
{
	QStringList ret;

	ret << _internalScope->getContainedNames();
	if (ret.isEmpty() && _internalScope->getParent() != _scope)
		 ret << _internalScope->getParent()->getFuctionsNames();
	ret << _type._type->_childNamesArray;

	return ret;
}
