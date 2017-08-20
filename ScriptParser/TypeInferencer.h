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

#include "ast.h"
#include "SymTable.h"
#include "OpcodeSequence.h"
#include "MetaObj.h"

#include <BytecodeVM.h>

struct CodeBlockInfo {
	OpcodeSequence code;
	RefType type;
	MetaObj obj;
};

struct DeclarationAndInit {
	OpcodeSequence decl;
	OpcodeSequence init;
};

class CodeBlockProcessInterface {
public:
	virtual ~CodeBlockProcessInterface() = default;
	virtual CodeBlockInfo processBlock(const AST::primary &val) = 0;
	virtual DeclarationAndInit compileDecl(const AST::proc_def &val) = 0;
};

class TypeInferencer
{
	template <typename T>
	void Error(const T& val, const QString& message) {
		_errors->Error(val._loc, message);
	}
	AST::CodeMessages* _errors;
	SymTable* _tab;
	CodeBlockProcessInterface* _primaryProcessor;
public:
	QList<QString> _typeNamesStack;
	ClassObj *_currentClass;
	OpcodeSequence _classBuffer;
	TypeInferencer(AST::CodeMessages* errors, SymTable* tab, CodeBlockProcessInterface* primaryProcessor);

	// TypeResolution  -------------------

	// primitives
	RefType typeInference(const bool &val);
	RefType typeInference(const int64_t &val);
	RefType typeInference(const double &val);
	RefType typeInference(const QString &val);
	RefType typeInference(const AST::set_construct &val);


	//expressions
	RefType typeInference(const AST::expr &val);
	RefType typeInference(const AST::expr_list &val);
	RefType typeInference(const AST::primary &val);

	RefType binaryOperationType( RefType left,  RefType right) const;
	void    checkBinary(RefType left,  RefType right, const AST::binary &val);

	RefType typeInference(const AST::unary &val);
	RefType typeInference(const AST::binary &val);
	RefType typeInference(const AST::internalexpr &val);

	RefType typeInference(const AST::constant &val);

	// types
	RefType typeInference(const AST::type &val);
	RefType typeInference(const AST::simple_type &val);
	RefType typeInference(const AST::array_type &val);

	RefType typeInference(const AST::set_type &val);
	RefType typeInference(const AST::file_type &val);
	RefType typeInference(const AST::pointer_type &val);
	RefType typeInference(const AST::subrange_type &val);
	RefType typeInference(const AST::enum_type &val);
	RefType typeInference(const AST::function_type &val);
	RefType typeInference(const AST::class_type &val);
	RefType typeInference(const AST::class_section &val);

	RefType typeInference(const boost::blank &val);

	static QList<ScriptVariant::Types> _opValue_typePriority;
private:
	void    addVars(TypeDef &typeObject,
					const AST::var_decls &var_decls);
};
