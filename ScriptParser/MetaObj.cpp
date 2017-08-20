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

#include "MetaObj.h"

MetaObj::MetaObj(SymTable *tab) : metatype(mNone), wrapperClassPrev(nullptr), wrapperClass(nullptr), objectClass(nullptr),  fieldOffset(-1),
	varObj(nullptr), funcObj(nullptr),  _tab(tab)
{
	low =0;
	isRef = true;
	callDone = false;
}

bool MetaObj::isCallable()
{
	return metatype ==  mFunc || metatype == mMethod;
}

bool MetaObj::isRefable()
{
	return metatype ==  mFuncRetainVar || metatype == mVar || metatype == mUnnamedVar;
}

RefType MetaObj::type()
{

	if ( metatype == MetaObj::mVar)
		return RefType(varObj->getType().getType()).setConst(varObj->getType().isConstant()).setRef(isRef);
	else if ( metatype == MetaObj::mUnnamedVar || metatype == MetaObj::mFuncRetainVar )
		return RefType(unnamedType).setRef(isRef);
	else if ( metatype == MetaObj::mFunc  ||  metatype == MetaObj::mMethod)
		return RefType(funcObj->getType()).setRef(isRef);

	return _tab->getUndefinedType();
}

void MetaObj::setClassObj(ClassObj *_classObj)
{
	wrapperClass = _classObj;
	if (wrapperClass)
		varObj = _tab->findSelfVar();

}

bool MetaObj::setVarObj(VarObj *_varObj)
{
	if (_varObj)
	{
		varObj = _varObj;
		metatype = MetaObj::mVar;
		determineClass();
	}else{
		metatype = MetaObj::mNone;
	}
	return metatype != MetaObj::mNone;
}

bool MetaObj::setFuncObj(FuncObj *_funcObj, bool isMethod)
{
	if (_funcObj)
	{
		funcObj = _funcObj;
		metatype = isMethod ? MetaObj::mMethod : MetaObj::mFunc;
		callDone = false;
		determineClass();
	}else{
		metatype = MetaObj::mNone;
	}
	return metatype != MetaObj::mNone;
}

bool MetaObj::setUnnamedObject(PTypeDef obj)
{
	if (!obj || obj->isUndefined())
		return false;

	unnamedType._type = obj;
	metatype = MetaObj::mUnnamedVar;
	determineClass();
	return true;
}

void MetaObj::determineClass()
{
	objectClass = nullptr;
	RefType t = type();
	if (t._type && !t._type->isUndefined())
	{
		objectClass = t._type->isClass()
				? _tab->findClass(t._type->getAlias())
				: nullptr;
	}
}

bool MetaObj::findField(const QString &name)
{
	if (!wrapperClass)
		return false;
	PTypeDef wrapperType = wrapperClass->getType().getType();
	if (!wrapperType)
		return false;

	PTypeDef accessObjType = wrapperType->findField(name);
	if (setUnnamedObject(accessObjType))
	{
		fieldOffset = wrapperType->getOffset(name);
		return true;
	}
	return false;
}

bool MetaObj::findMethod(const QString &name)
{
	if (!wrapperClass)
		return false;
	FuncObj * funcObj =  _tab->findFunc(name, wrapperClass->getInternalScope()); //TODO: access checks.
	return setFuncObj( funcObj, true);
}

bool MetaObj::findVariable(const QString &name)
{
	return setVarObj(_tab->findVar(name));
}

bool MetaObj::findFunction(const QString &name)
{
	FuncObj * funcObj = _tab->findFunc(name);
	return setFuncObj(funcObj, false);
}

bool MetaObj::findMethodRetain(const QString &name)
{
	if ( !funcObj)
		return false;
	QString fieldName = funcObj->getName() + "." + name;
	if (findField(fieldName))
		return true;

	if (wrapperClassPrev)
	{
		PTypeDef accessObjType = wrapperClassPrev->getType().getType()->findField(fieldName);
		if (setUnnamedObject(accessObjType))
		{
			fieldOffset = wrapperClassPrev->getType().getType()->getOffset(fieldName);//TODO: suboptimal calls. Consider refactor with findField.
			return true;
		}
	}
	return false;
}

bool MetaObj::doAccess()
{
	if (metatype ==  MetaObj::mNone || metatype == MetaObj::mFunc )
		return false;

	fieldOffset = 0;
	wrapperClassPrev = wrapperClass;
	wrapperClass = objectClass;
	objectClass = nullptr;
	return true;
}

bool MetaObj::doDeref()
{
	RefType t = type();
	if (t._type->_child.size() != 1)
		return false;
	low = 0;

	return setUnnamedObject(t._type->_child[0]);
}

bool MetaObj::doAddress()
{
	RefType t = type();
	if (t._isLiteral)
		return false;
	TypeDef pt;
	pt._category = TypeDef::Pointer;
	pt._child.append(t._type);
	PTypeDef newType = _tab->registerType(pt);
	isRef = false;
	return setUnnamedObject(newType);
}

bool MetaObj::doIndex()
{
	RefType t = type();
	if (!doDeref())
		return false;
	if (t._type->_category != TypeDef::Array){
		return false;
	}
	low = t._type->_arrayLowOffset;
	return true;
}

bool MetaObj::doIndexStr()
{
	RefType t = type();
	if (t._type->_category != TypeDef::Scalar || t._type->getOpcodeType() != ScriptVariant::T_string )
		return false;
	return setUnnamedObject(_tab->findType("__string_char"));
}

bool MetaObj::doCall()
{
	callDone = true;
	isRef = false;
	setUnnamedObject(funcObj->getType()._type);
	return true;
}

bool MetaObj::findAny(const QString &name, int findFlags)
{
	if ((findFlags& ffMethod)   && findMethod(name))
		return true;
	if ((findFlags& ffField)    && findField(name))
		return true;
	if ((findFlags& ffVariable) && findVariable(name))
		return true;
	if ((findFlags& ffFunction) && findFunction(name))
		return true;
	if ((findFlags& ffMethodRetain) && findMethodRetain(name))
		return true;
	return false;
}
