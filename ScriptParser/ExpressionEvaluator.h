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

#include <BytecodeVM.h>

#include <QVector>

using EvaluationList = QVector<ScriptVariant>;

/// Compile-time constant expression optimization.
/// Helps to optimize simple expressions 1 + 2 * 3
class ExpressionEvaluator
{
public:

	// Evaluation    -------------------

	// primitives
	bool evaluate(EvaluationList& result, const bool &val) const;
	bool evaluate(EvaluationList& result, const int64_t &val) const;
	bool evaluate(EvaluationList& result, const double &val) const;
	bool evaluate(EvaluationList& result, const QString &val) const;
	bool evaluate(EvaluationList& result, const AST::set_construct &val) const;
	bool evaluate(EvaluationList& result, const boost::blank &val) const;


	//expressions
	bool evaluate(EvaluationList& result, const AST::expr &val) const;
	bool evaluate(EvaluationList& result, const AST::primary &val) const;
	bool evaluate(EvaluationList& result, const AST::unary &val) const;
	bool evaluate(EvaluationList& result, const AST::binary &val) const;
	bool evaluate(EvaluationList& result, const AST::constant &val) const;
	bool evaluate(EvaluationList& result, const AST::internalexpr &val) const;

	bool getInt(const AST::expr &val, int64_t& value) const;
	bool getDouble(const AST::expr &val, double& value) const;
	bool getString(const AST::expr &val, QString& value) const;
	bool getOpValue(const AST::expr &val, ScriptVariant& value) const;
	bool getOpValues(const AST::expr &val, std::vector<ScriptVariant>& value) const;
};
