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
#include "SymTableTypes.h"

#include <boost/variant.hpp>

#include <QString>
#include <QStringList>

/**
 * \brief translator for AST to Pascal or C text.
 */
class StringVisitor : public boost::static_visitor<QString>
{

public:
	QMap<AST::CodeLocation, RefType> _codeTypes;
	enum OutType { otPascal, otC };
	StringVisitor(OutType type = otPascal, int flags = 0)  : _level(0),_type(type),_flags(flags) {}

	// primitives
	QString operator ()(const bool &val) const;
	QString operator ()(const int64_t &val) const;
	QString operator ()(const double &val) const;
	QString operator ()(const QString &val) const;
	QString operator ()(const AST::set_construct &val) const;
	QString operator ()(const AST::ident &val) const;
	QString operator ()(const AST::idents &val) const;

	//expressions
	QString operator ()(const AST::expr &val) const;
	QString operator ()(const AST::expr_list &val) const;
	QString operator ()(const AST::primary &val) const;
	QString operator ()(const AST::primary_accessor &val) const;
	QString operator ()(const AST::unary &val) const;
	QString operator ()(const AST::binary &val) const;

	QString operator ()(const AST::call_expr &val) const;
	QString operator ()(const AST::constant &val) const;
	QString operator ()(const AST::deref_expr &val) const;
	QString operator ()(const AST::address_expr &val) const;
	QString operator ()(const AST::idx_expr &val) const;
	QString operator ()(const AST::access_expr &val) const;
	QString operator ()(const AST::internalexpr &val) const;

	//statements
	QString operator ()(const AST::statement &val) const;
	QString operator ()(const AST::compoundst &val) const;
	QString operator ()(const AST::assignmentst &val) const;
	QString operator ()(const AST::gotost &val) const;
	QString operator ()(const AST::switchst &val) const;
	QString operator ()(const AST::ifst &val) const;
	QString operator ()(const AST::forst &val) const;
	QString operator ()(const AST::whilest &val) const;
	QString operator ()(const AST::repeatst &val) const;
	QString operator ()(const AST::procst &val) const;
	QString operator ()(const AST::withst &val) const;
	QString operator ()(const AST::labelst &val) const;
	QString operator ()(const AST::tryst &val) const;
	QString operator ()(const AST::writest &val) const;
	QString operator ()(const AST::readst &val) const;
	QString operator ()(const AST::strst &val) const;
	QString operator ()(const AST::raisest &val) const;
	QString operator ()(const AST::inheritedst &val) const;
	QString operator ()(const AST::onst &val) const;
	QString operator ()(const AST::internalfst &val) const;

	QString operator ()(const AST::write_list &val) const;
	QString operator ()(const AST::write_param &val) const;

	// types
	QString operator ()(const AST::type &val) const;
	QString operator ()(const AST::simple_type &val) const;
	QString operator ()(const AST::array_type &val) const;
	QString operator ()(const AST::set_type &val) const;
	QString operator ()(const AST::file_type &val) const;
	QString operator ()(const AST::pointer_type &val) const;
	QString operator ()(const AST::subrange_type &val) const;
	QString operator ()(const AST::enum_type &val) const;
	QString operator ()(const AST::function_type &val) const;
	QString operator ()(const AST::class_type &val) const;
	QString operator ()(const AST::class_section &val) const;

	//declarations
	QString operator ()(const AST::decl_part_list &val) const;
	QString operator ()(const AST::decl_part &val) const;
	QString operator ()(const AST::label_decl_part &val) const;
	QString operator ()(const AST::const_def_part &val) const;
	QString operator ()(const AST::type_def_part &val) const;
	QString operator ()(const AST::var_decl_part &val) const;
	QString operator ()(const AST::proc_def &val) const;
	QString operator ()(const AST::unit_spec &val) const;
	QString operator ()(const AST::uses_part &val) const;

	QString operator ()(const AST::var_decl &val) const;
	QString operator ()(const AST::proc_decl &val) const;
	QString operator ()(const AST::type_def &val) const;

	//program and blocks
	QString operator ()(const AST::pascalSource &val) const;
	QString operator ()(const AST::program &val) const;
	QString operator ()(const AST::stProgram &val) const;
	QString operator ()(const AST::unit &val) const;
	QString operator ()(const AST::sequence &val) const;
	QString operator ()(const AST::block &val) const;

	QString operator ()(const boost::blank &val) const;

private:
	mutable int _level;
	mutable bool _isDeclaration = false;
	mutable QString _currentFunction;
	mutable QString _makeResult;
	mutable OutType _type;
	mutable QString _typeSuffix;
	int _flags;

	inline QString idn() const { return QString("    ").repeated(_level);}
	inline QString kw(const QString & k) const { return _type == otC ? k.toLower() : k;}
};
