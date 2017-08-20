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

#include "SymTable.h"

/// Code model object, used for translating constructions: "array[123].field.method()";
struct MetaObj
{
	enum MetaType { mNone, mVar, mFunc, mMethod,  mFuncRetainVar, mUnnamedVar  };
	MetaType metatype;
	ClassObj *wrapperClassPrev;
	ClassObj *wrapperClass;
	ClassObj *objectClass;
	int fieldOffset ;
	int low ;
	bool isRef;
	bool callDone;
	VarObj  *varObj ;
	FuncObj *funcObj ;
	RefType unnamedType;
	SymTable * _tab;
	MetaObj(SymTable * tab = nullptr);

	bool isCallable();
	bool isRefable();
	RefType type();

	void setClassObj( ClassObj *_classObj);
	bool setVarObj(VarObj  *_varObj);
	bool setFuncObj(FuncObj  *_funcObj, bool isMethod);
	bool setUnnamedObject(PTypeDef obj);
	void determineClass();

	bool findField(const QString& name);
	bool findMethod(const QString& name);
	bool findVariable(const QString& name);
	bool findFunction(const QString& name);
	bool findMethodRetain(const QString& name);
	bool doAccess();
	bool doDeref();
	bool doAddress();
	bool doIndex();
	bool doIndexStr();
	bool doCall();
	enum FindFlags { ffVariable = 1 << 1, ffFunction = 1 << 2, ffField = 1 << 3, ffMethod = 1 << 4, ffMethodRetain = 1 << 5,
					 ffAllGlobal = ffVariable | ffFunction | ffField | ffMethod |ffMethodRetain,
					 ffAllObject = ffField | ffMethod|ffMethodRetain
				   };
	bool findAny(const QString &name, int findFlags);
};

