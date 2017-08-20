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

#include "SymTableTypes.h"

// =============================================================================
TypeDef::TypeDef(ScriptVariant::Types type, Types category)
	: _opcodeType(type)
	, _category(category)
	, _arrayHighBound(0)
	, _arrayLowOffset(0)
	, _isExternal(false)
	, _parent(nullptr)
{}

ScriptVariant::Types TypeDef::getOpcodeType() const
{
	return _opcodeType;
}

bool TypeDef::equalTo(const TypeDef &another) const
{
	if (_category == Class) return false;
	bool ret = (_opcodeType == another._opcodeType)
			&& (_category == another._category)
			&& (_arrayLowOffset == another._arrayLowOffset)
			&& (_arrayHighBound == another._arrayHighBound)
			&& (_child == another._child)
			&& (_childNamesArray == another._childNamesArray)
			;
	return ret;
}

const QString& TypeDef::getAlias() const
{
	return _userAlias;
}

TypeDef& TypeDef::setAlias(const QString &alias)
{
	_userAlias = alias;
	return *this;
}

bool TypeDef::isInt() const
{
	static QSet<ScriptVariant::Types> intTypes = QSet<ScriptVariant::Types>() << ScriptVariant::T_int32_t << ScriptVariant::T_int64_t << ScriptVariant::T_uint32_t << ScriptVariant::T_uint64_t
																  << ScriptVariant::T_int16_t << ScriptVariant::T_uint16_t << ScriptVariant::T_int8_t << ScriptVariant::T_uint8_t
																  << ScriptVariant::T_string_char
																	 ;
	return _category == Scalar && intTypes.contains(_opcodeType);
}

bool TypeDef::isFloat() const
{
	return (_category == Scalar )
		&& (_opcodeType == ScriptVariant::T_float32 || _opcodeType == ScriptVariant::T_float64);
}

bool TypeDef::isScalar() const
{
	return (_category == Scalar)
		&& (isInt() || isFloat() || (_opcodeType == ScriptVariant::T_string) || (_opcodeType == ScriptVariant::T_bool));
}

bool TypeDef::isBoolean() const
{
	return (_category == Scalar) && (_opcodeType == ScriptVariant::T_bool);
}

bool TypeDef::isUndefined() const
{
	return (_category == Scalar) && (_opcodeType == ScriptVariant::T_UNDEFINED);
}

bool TypeDef::isClass() const
{
	return _category == Class;
}

bool TypeDef::isPointer() const
{
	return _category == Pointer;
}

bool TypeDef::acceptType(PTypeDef type) const
{
	auto mySig = getSignature();
	auto typeSig = type->getSignature();
	return mySig == typeSig;
}

int TypeDef::getByteSize() const
{
	if( _category == Scalar ){
		if (isUndefined())
			return 0;
		return 1;
	}
	if( _category == Pointer ){
		return 1;
	}

	if ( _category == Array)
		return getArraySize() * _child[0]->getByteSize();

	int sum = 0;
	if (_parent)
		sum += _parent->getByteSize();

	for(int i = 0; i<_child.size(); i++){
		if (_child[i])
			sum += _child[i]->getByteSize();
	}

	return sum;
}

QString TypeDef::getTypeDescription() const
{
	static const QStringList cats = QStringList() << "SC" << "ARR" << "REC" << "PTR" << "CLS";
	// TODO: inheritance?
	QStringList childs;
	for(int i = 0; i < _child.size(); ++i) {
		QString namepart = "";
		if (_childNamesArray.size()) {
			namepart = _childNamesArray.value(i) +  ": ";
		}
		childs << namepart + _child[i]->getTypeDescription();
	}
	QString result;
	if (_category != Scalar) result += " " + cats.value(_category, "UNDEFINED");
	if (_category == Scalar) result += " " + ((_opcodeType < ScriptVariant::TYPES_COUNT)
											  ? QString::fromStdString(ScriptVariant::type2string(_opcodeType))
											  : "UNDEFINED");
	if (!_userAlias.isEmpty()) result += QString(" \"%1\"").arg(_userAlias);
	if (childs.size())         result += QString("(%1)").arg(childs.join(", "));
	if (_category == Array )   result += QString("[%1]").arg(getArraySize());
	return result;
}


bool TypeDef::addField(const QString &fieldName, PTypeDef fieldType)
{
	if (_childNames.contains(fieldName.toLower())) return false;
	_childNames[fieldName.toLower()] = _child.size();
	_child << (fieldType);
	_childNamesArray << fieldName.toLower();
	return true;
}

PTypeDef TypeDef::findField(const QString &fieldName) const
{
	static TypeDef undef;
	int childIndex = _childNames.value(fieldName.toLower(), -1);
	if (childIndex >= 0) return _child[childIndex];
	else if (_parent) return _parent->findField(fieldName);
	else return &undef;
}

int TypeDef::getOffset(const QString &fieldName) const
{
	int parentOffset = -1;
	int parentSize = 0;
	if (_parent) {
		parentOffset = _parent->getOffset(fieldName);
		if (parentOffset) {
			parentSize = _parent->getByteSize();
		}
	}
	if (parentOffset != -1) return parentOffset;
	parentOffset = parentSize;

	int childIndex = _childNames.value(fieldName.toLower(), -1);
	if (childIndex >= 0) {
		int offset = 0;
		for (int i = 0; i < childIndex; ++i) {
			if (_child[i]) offset += _child[i]->getByteSize();
		}
		return offset + parentOffset;
	}

	return -1;
}

QList<QPair<ScriptVariant::Types, int> > TypeDef::getSignature() const
{
	QList<QPair<ScriptVariant::Types, int> > result;
	if (isUndefined()) return result;
	if( _category == Scalar)
	{
		result.push_back(QPair<ScriptVariant::Types, int>(_opcodeType,1));
		return result;
	}
	if( _category == Pointer)
	{
		result.push_back(QPair<ScriptVariant::Types, int>(ScriptVariant::T_int32_t,1));
		return result;
	}
	if (_parent) {
		result << _parent->getSignature();
	}

	QList<QPair<ScriptVariant::Types, int> > childsSign;
	for (int i = 0; i < _child.size(); ++i) {
		if (_child[i]) childsSign << _child[i]->getSignature();
	}
	if (_category == Array) {
		int size = getArraySize();
		if (childsSign.size() == 1) {
			result << QPair<ScriptVariant::Types, int> (childsSign.first().first, childsSign.first().second * size);
		}
		else {
			for (int i = 0; i < size; ++i)
				result += childsSign;
		}
	}
	else
		result << childsSign;
	return result;
}

QList<ScriptVariant::Types> TypeDef::getExpandedSignature() const
{
	QList<ScriptVariant::Types> ret;
	foreach (auto pair, this->getSignature()) {
		for (int i=0; i< pair.second;i++)
			ret << pair.first;
	}
	return ret;
}

int TypeDef::getArraySize() const
{
	if (_category != Array)
		return 0;
	return _arrayHighBound - _arrayLowOffset +1;
}


// =============================================================================

RefType::RefType(PTypeDef type)
	: _type(type)
	, _isConst(false)
	, _isRef(false)
	, _isLiteral(false)
{}

ScriptVariant::Types RefType::getOpcodeType() const
{
	return _type->getOpcodeType();
}

int RefType::getOffset(const QString &name) const
{
	return _type->getOffset(name);
}

PTypeDef RefType::getType() const
{
	return _type;
}

bool RefType::isConstant() const
{
	return _isConst;
}

bool RefType::isReference() const
{
	return _isRef;
}

int RefType::getByteSize() const
{
	if (!_type)
		return 0;
	if (_isRef)
		return 1;
	return _type->getByteSize();
}

bool RefType::isValid() const
{
	return _type && !_type->isUndefined();
}

RefType &RefType::setConst(bool isConst)
{
	_isConst = isConst;
	return *this;
}

RefType &RefType::setRef(bool isref)
{
	_isRef = isref;
	return *this;
}
