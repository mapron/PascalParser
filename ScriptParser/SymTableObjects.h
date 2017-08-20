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

#pragma once

#include <QString>
#include <QList>
#include <stdexcept>

#include "ast.h"
#include "SymTableTypes.h"

class BlockScope;

/// Abstract symbol table object - variable, function or class.
class NamedObj
{

public:
	  enum AccessModifier { amUndefined, amPublic, amPrivate, amProtected };
	  enum ObjType {tNone, tVarObj, tFuncObj, tClassObj };
	  enum Flags {fNone = 0, fStatic = 1 << 0, fExternal = 1 << 1, fConst = 1 << 2, fForward = 1 << 3,  fUsed = 1 << 4};
	virtual ~NamedObj();

	QString             getName() const;
	QString             getNameOriginal() const;
	QString             getComment() const;
	RefType             getType() const;
	int                 getScopeLevel() const;
	AccessModifier      getAccess() const;
	ObjType             getObjType() const;

	bool                isConst() const;
	bool                isStatic() const;
	bool                isExternal() const;
	bool                isForward() const;
	bool                isUsed(); // used only for External variables.

	bool                setUsed();
	void                setForward(bool state);
	virtual void        setScopeArguments(BlockScope *scope);
	void                setAccess(AccessModifier access);
	void                setComment(const QString& comment);

protected:
	NamedObj        (const QString &name,
					 const RefType &type,
					 ObjType objType,
					 Flags flags = fNone);
	QString             _name;
	QString             _nameOriginal;
	QString             _comment;
	RefType             _type;
	Flags               _flags;
	AccessModifier      _access;
	BlockScope          *_scope;
	ObjType             _objType;
};

/// Code model variable.
class VarObj : public NamedObj
{
public:
	static VarObj*      createRegularVar(const QString &name,
										 const RefType &type,
										 int memoryAddress,
										 int memorySize,
										 bool isConst = false);
	static VarObj*      createStaticVar(const QString &name,
										const RefType &type,
										int externalMemoryAddress);
	static VarObj*      createExternalVar(const QString &name,
										  const RefType &type,
										  bool isConst = false);

	ScriptVariant::Types  getOpcodeType() const;
	int                 getOffset(const QString &name) const;
	int                 getMemoryAddress() const;
	int                 getMemorySize() const;

	PTypeDef            findField(const QString &fieldName) const;

	void                changeMemoryAddress(int newMemoryAddress);


private:
						VarObj(const QString &name,
							   const RefType &type,
							   int memoryAddress,
							   int memorySize,
							   Flags flags);

	int                 _memoryAddress;
	int                 _memorySize;
};

/// Code model function.
class FuncObj : public NamedObj
{
public:
	// Function argument, can be determined by two ways:
	struct FunctionArg
	{
		QString     _name;
		std::shared_ptr<AST::expr>
					_initializer;
		RefType     _type;
		//------- OR ----------
		QString     _typeName;
		int         _arraySize = -1;
		bool        _ref = false;
	};

public:
	static FuncObj*     createRegularFunc(const QString &name,
										  const QString &fullName,
										  const RefType &returnType,
										  const QList<FunctionArg> &arguments,
										  Flags flags);

	QString             getFullName() const;
	QString             getFullSignature() const;

	const BlockScope*   getInternalScope() const;
	BlockScope*         getInternalScope();
	const QList<FunctionArg>&
						getArguments() const;
	int                 getArgumentsNumber() const;

	QString             getReturnTypeName() const;

	int                 returnSize()  const;
	int                 callSize()  const;
	QString             argumentsSizes()  const;

	void                setScopeArguments(BlockScope *scope) override;

private:
						FuncObj(const QString &name,
								const QString &fullName,
								const RefType &returnType,
								const QList<FunctionArg> &arguments,
								Flags flags);


	QString             _fullName; // class name included.
	BlockScope          *_internalScope;  // function body scope
	QList<FunctionArg>  _arguments; // function signature.
};

/// Code model struct/class.
class ClassObj : public NamedObj
{
public:
						ClassObj(const QString &name,
								 PTypeDef classDefinition, // class fields type.
								 ClassObj* parent);

	BlockScope*         getInternalScope() const;
	void                setScopeArguments(BlockScope *scope) override;
	void                setClassDefinition(PTypeDef classDefinition);
	QStringList         getMemberList(const QString& hint = "") const;

private:
	ClassObj           *_parent;
	BlockScope          *_internalScope;
};
