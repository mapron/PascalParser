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

#include "SymTableTypes.h"
#include "SymTableObjects.h"

#include <BytecodeVM.h>

#include <QString>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QStringList>

namespace AST {
	struct expr;
	class CodeMessages ;
}

class SymTable;

/// Code model scope container. Contains named objects - variables, etc.
class BlockScope
{
public:
	explicit            BlockScope();
						~BlockScope();
	BlockScope*         createNested(BlockScope* parent = nullptr);
	bool                isRoot() const;
	BlockScope*         getParent() const;
	BlockScope*         getLastAddedNested();
	VarObj*             findVar(const QString &objName) const;
	FuncObj*            findFunc(const QString &objName) const;
	ClassObj*           findClass(const QString &objName) const;
	NamedObj*           findObj(const QString &objName, NamedObj::ObjType objType = NamedObj::tNone) const;
	void                registerVariable(VarObj *varPtr);
	void                registerObject(NamedObj *obj);
	QStringList         debug() const;
	void                clear();

	int                 getNextMemoryAddress() const;
	int                 getScopeLevel() const;

	QList<const VarObj*> getAllExternal() const;
	QList<const VarObj*> getAllLocal() const;
	QStringList         getContainedNames() const;
	QStringList         getFuctionsNames(NamedObj::AccessModifier am
												= NamedObj::amUndefined) const;
	QStringList         getVariablesNames(NamedObj::AccessModifier am
												= NamedObj::amUndefined) const;

private:
	explicit            BlockScope(BlockScope *parent);
	QStringList         debug(int level) const;

private:
	BlockScope          *_parent;
	QList<BlockScope>   _nested;
	int                 _nextMemoryAddress;

	QList<NamedObj*>    _objects;
	QHash<QString, int> _objectsNames;

};

/// Code model symbol table. Contains tree of scopes.
class SymTable
{
public:
	struct FunctionRec {
		RefType _type;
		QList<FuncObj::FunctionArg> _args;
		QString _className;
		QString _name;

		QString _typeName;
	};

public:
						SymTable(AST::CodeMessages *errors);
						~SymTable();
	void                clear(bool registerTypes = true);
	VarObj*             createNewRegularVarObj(const QString &name,
											   RefType type,
											   bool isConst = false);
	VarObj*             createNewStaticVarObj(QString name,
											  RefType type);
	VarObj*             createNewExternalVarObj(const QString &name,
												RefType type,
												bool isConst = false);
	FuncObj*            createNewMethodObj(ClassObj *ownerClassObj,
										   const FunctionRec &prototype,
										   bool isForward,
										   bool isStatic,
										   bool isExternal);
	ClassObj*           createNewClassObj(QString name,
										  RefType type,
										  QString nameParent);
	VarObj*             findSelfVar();
	VarObj*             findVar(const QString &name, BlockScope *workingScope = nullptr);
	VarObj*             findSelfClassField(const QString &name, int *fieldOffset);
	FuncObj*            findFunc(const QString &name, BlockScope *workingScope = nullptr);
	ClassObj*           findClass(const QString &name);
	NamedObj*           findObj(const QString &name, BlockScope *workingScope = nullptr);
	void                openScope (BlockScope *forceScope = nullptr);
	void                closeScope ();
	void                debug();
	const ClassObj*     getVarClass(const QString &varName);

	BlockScope*         getCurrentScope();
	BlockScope*         getTopScope();
	PTypeDef            getUndefinedType() const;
	PTypeDef            findType(QString type) const;
	PTypeDef            registerType(const TypeDef& tmpType, bool autoAppend = true, bool externalType = false);
	bool                setNameForType(PTypeDef type, QString name);

	QStringList         getRegisteredTypes() const;
	TreeVariant         getRegisteredTypesMap() const;

	TreeVariant         getExternalVarsList() const;


	QStringList         _autoCompleteList;
	bool                _hasAC;
	bool                _doAC;
	FuncObj*            _lastFuncObj;

private:
	bool                checkIdent(QString name);
	BlockScope          *_topScope;
	BlockScope          *_currentScope;
	QList<BlockScope *> _scopeStack;
	AST::CodeMessages   *_errors;

	QList<TypeDef>      _registeredTypes;
	QHash<QString, int> _registeredTypeIndex;

	TypeDef             _und;
};
