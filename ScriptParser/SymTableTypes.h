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
#include <BytecodeVM.h>

#include <QStringList>
#include <QVector>
#include <QHash>
#include <QSet>

class TypeDef;
using PTypeDef = const TypeDef*;

// Symbol table type of code model.
class TypeDef
{
public:
	enum Types {
		Scalar,
		Array,
		Pointer,
		Class
	};

public:
						TypeDef(ScriptVariant::Types type = ScriptVariant::T_UNDEFINED,
								Types category = Scalar);
	ScriptVariant::Types      getOpcodeType() const;
	bool                equalTo(const TypeDef& another) const;
	const QString&      getAlias() const;
	TypeDef&            setAlias(const QString &alias);

	bool                isInt() const;
	bool                isFloat() const;
	bool                isScalar() const;
	bool                isBoolean() const;
	bool                isUndefined() const;
	bool                isClass() const;
	bool                isPointer() const;
	bool                acceptType(PTypeDef type) const;

	int                 getByteSize() const;
	QString             getTypeDescription() const;
	int                 getOffset(const QString &fieldName)  const;
	QList<QPair<ScriptVariant::Types, int> >
						getSignature() const;
	QList< ScriptVariant::Types >
						getExpandedSignature() const;
	int                 getArraySize() const;

	bool                addField(const QString &fieldName, PTypeDef fieldType);
	PTypeDef            findField(const QString &str) const;

	ScriptVariant::Types      _opcodeType;    //!< internal VM type
	Types               _category;      //!< standard type name
	QString             _userAlias;     //!< identifier
	int64_t             _arrayHighBound;
	int64_t             _arrayLowOffset;
	bool                _isExternal;

	QHash<QString, int> _childNames;
	QList<QString>      _childNamesArray;
	QList<PTypeDef>     _child;
	PTypeDef            _parent;
};

/// Symbol table type with CV-qualifiers.
class RefType {
public:
						RefType(PTypeDef type = nullptr);
	ScriptVariant::Types      getOpcodeType() const;
	int                 getOffset(const QString &name) const;
	PTypeDef            getType() const;
	int                 getByteSize() const;

	bool                isConstant() const;
	bool                isReference() const;
	bool                isValid() const;

	RefType&            setConst(bool isConst);
	RefType&            setRef(bool isref);

	PTypeDef            _type;
	bool                _isConst;
	bool                _isRef;
	bool                _isLiteral;
};
