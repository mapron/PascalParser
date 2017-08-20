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
#include "TypeInferencer.h"

#include <BytecodeVM.h>
#include <ScriptVM.h>

#include <QString>
#include <QStringList>

#include <boost/variant.hpp>

/**
 * \brief Compiles AST to VM bytecode. Represents compiler semantics.
 */
class CodeGenerator : public CodeBlockProcessInterface
{
	template <typename T>
	void Error(const T& val, const QString& message) {
		_errors->Error(val._loc, message);
	}
	template <typename T>
	void Warning(const T& val, const QString& message) {
		_errors->Warning(val._loc, message);
	}
	QList<CodeBlockInfo> _withObjects;
	ScriptVM* _executor;

	bool _isTopBlock;

	FuncObj *_currentFunc;
	TypeInferencer* _types;

public:
	SymTable   *_tab;
	AST::CodeMessages* _errors;
	QString _startAddress;
	enum ParserOptions {poNone = 0, poForbidExternal = 1 << 0 };
	int _parserOptions;

	CodeGenerator(AST::CodeMessages* errors, ScriptVM* executor) ;
	~CodeGenerator() ;

	void clear();

	// Code generation

	CodeBlockInfo processBlock(const AST::primary &val);
	CodeBlockInfo processBlock(const AST::constant &val);
	CodeBlockInfo processBlock(const AST::expr_list &val);
	CodeBlockInfo processBlock(const AST::internalexpr &val);

	// primitives

	//expressions
	OpcodeSequence compile(const AST::expr &val);
	OpcodeSequence compile(const AST::expr_list &val);
	OpcodeSequence compile(const AST::primary &val);
	OpcodeSequence compile(const AST::unary &val);
	OpcodeSequence compile(const AST::binary &val);
	OpcodeSequence compile(const AST::constant &val);
	OpcodeSequence compile(const AST::internalexpr &val);

	//statements
	OpcodeSequence compile(const AST::statement &val);
	OpcodeSequence compile(const AST::compoundst &val);
	OpcodeSequence compile(const AST::assignmentst &val);
	OpcodeSequence compile(const AST::gotost &val);
	OpcodeSequence compile(const AST::switchst &val);
	OpcodeSequence compile(const AST::ifst &val);
	OpcodeSequence compile(const AST::forst &val);
	OpcodeSequence compile(const AST::whilest &val);
	OpcodeSequence compile(const AST::repeatst &val);
	OpcodeSequence compile(const AST::procst &val);
	OpcodeSequence compile(const AST::withst &val);
	OpcodeSequence compile(const AST::labelst &val);
	OpcodeSequence compile(const AST::tryst &val);
	OpcodeSequence compile(const AST::writest &val);
	OpcodeSequence compile(const AST::readst &val);
	OpcodeSequence compile(const AST::strst &val);
	OpcodeSequence compile(const AST::raisest &val);
	OpcodeSequence compile(const AST::inheritedst &val);
	OpcodeSequence compile(const AST::onst &val);
	OpcodeSequence compile(const AST::internalfst &val);

	OpcodeSequence compile(const AST::write_param &val);

	// types

	//declarations
	DeclarationAndInit compileDecl(const AST::decl_part_list &val);
	DeclarationAndInit compileDecl(const AST::decl_part &val);
	DeclarationAndInit compileDecl(const AST::label_decl_part &val);
	DeclarationAndInit compileDecl(const AST::const_def_part &val);
	DeclarationAndInit compileDecl(const AST::type_def_part &val);
	DeclarationAndInit compileDecl(const AST::var_decl_part &val);
	DeclarationAndInit compileDecl(const AST::proc_def &val);
	DeclarationAndInit compileDecl(const AST::unit_spec &val);
	DeclarationAndInit compileDecl(const AST::uses_part &val);
	DeclarationAndInit compileDecl(const AST::var_decl &val);

	//program and blocks
	OpcodeSequence compile(const AST::pascalSource &val);
	OpcodeSequence compile(const AST::program &val);
	OpcodeSequence compile(const AST::stProgram &val);
	OpcodeSequence compile(const AST::unit &val);
	OpcodeSequence compile(const AST::sequence &val);
	OpcodeSequence compile(const AST::block &val);

	OpcodeSequence compile(const boost::blank &val);
	DeclarationAndInit compileDecl(const boost::blank &val);

private:
	OpcodeSequence mkBinSeq(const AST::internalexpr &val, int op, int type);
	OpcodeSequence mkLogicalSeq(const AST::internalexpr &val, const OpcodeSequence &opers);
	OpcodeSequence mkLogicalSeq2(const AST::internalexpr &val, int op, int type);
	bool emitCall(FuncObj *function,
				  AST::expr_list &callArgs,
				  QString &prim,
				  OpcodeSequence &mainSequence, OpcodeSequence &resultSequence);

	AST::expr_list flatExprList(const AST::expr &expr);


};
