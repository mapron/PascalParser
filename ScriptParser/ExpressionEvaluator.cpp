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

#include "ExpressionEvaluator.h"

class EvalVisitor : public boost::static_visitor<bool> {
public:
	const ExpressionEvaluator* _gen;
	EvaluationList &_result;
	EvalVisitor(const ExpressionEvaluator* gen, EvaluationList &result):_gen(gen), _result(result){ }
	template <class ASTnode>
	bool operator ()(const ASTnode& node) const {
		return _gen->evaluate(_result, node);
	}
};

bool ExpressionEvaluator::evaluate(EvaluationList &result, const bool &val) const
{
	ScriptVariant result_v;
	result_v.setValue(val, ScriptVariant::T_AUTO);
	result << result_v;
	return true;
}
bool ExpressionEvaluator::evaluate(EvaluationList &result,const int64_t &val) const
{
	ScriptVariant result_v;
	result_v.setValue(val, ScriptVariant::T_AUTO);
	result << result_v;
	return true;
}
bool ExpressionEvaluator::evaluate(EvaluationList &result,const double &val) const
{
	ScriptVariant result_v;
	result_v.setValue(val, ScriptVariant::T_AUTO);
	result << result_v;
	return true;
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const QString &val) const
{
	ScriptVariant result_v;
	result_v.setValue(val.toStdString(), ScriptVariant::T_AUTO);
	result << result_v;
	return true;
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::set_construct &val) const
{
	return false;
}

bool ExpressionEvaluator::evaluate(EvaluationList &result, const boost::blank &val) const
{
	return false;
}


bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::expr &val) const
{
	return boost::apply_visitor(EvalVisitor(this, result), val._expr);
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::primary &val) const
{
	const AST::constant* constant = boost::get<AST::constant>(&val._expr);
	const AST::ident* ident = boost::get<AST::ident>(&val._expr);
	const AST::expr_list* expr_list = boost::get<AST::expr_list>(&val._expr);
	if (ident) { //TODO: check for const table?

	}
	if (constant) return this->evaluate(result, *constant);
	if (expr_list){
		foreach (const AST::expr& expr, expr_list->_exprs){
			if (!this->evaluate(result, expr))
				return false;
		}
		return true;
	}
	return false;
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::unary &val) const
{
	bool ret = this->evaluate(result, val._expr);
	if (ret && val._op == BytecodeVM::UMINUS) {
		result.last().setValue( - result.last().getValue<double>() );

	}
	return ret;
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::binary &val) const
{
	EvaluationList left, right;
	if (!this->evaluate(result, val._left))  return false;
	if (!this->evaluate(right,  val._right)) return false;
	ScriptVariant result_v = left.last();
	double left_v = left.last().getValue<double>();
	double right_v = right.last().getValue<double>();
	switch (val._op) {
		case BytecodeVM::PLUS:  result_v.setValue( left_v + right_v ); break;
		case BytecodeVM::MINUS: result_v.setValue( left_v - right_v ); break;
		case BytecodeVM::DIV:   result_v.setValue( left_v / right_v ); break;
		case BytecodeVM::MUL:   result_v.setValue( left_v * right_v ); break;
	default:
		return false;
	}
	result << result_v;

	return true;
}


bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::constant &val) const
{
	return boost::apply_visitor(EvalVisitor(this, result), val._constant);
}

bool ExpressionEvaluator::evaluate(EvaluationList &result,const AST::internalexpr &val) const
{
	return false;
}


bool ExpressionEvaluator::getInt(const AST::expr &val, int64_t &value) const
{
	EvaluationList result;
	if (!this->evaluate(result, val)) return false;
	value = result.last().getValue<int64_t>();
	return true;
}

bool ExpressionEvaluator::getDouble(const AST::expr &val, double &value) const
{
	EvaluationList result;
	if (!this->evaluate(result, val)) return false;
	value = result.last().getValue<double>();
	return true;
}

bool ExpressionEvaluator::getString(const AST::expr &val, QString &value) const
{
	EvaluationList result;
	if (!this->evaluate(result, val)) return false;
	value = QString::fromStdString( result.last().getValue<std::string>() );
	return true;
}

bool ExpressionEvaluator::getOpValue(const AST::expr &val, ScriptVariant &value) const
{
	EvaluationList result;
	if (!this->evaluate(result, val)) return false;
	value = result.last();
	return true;
}

bool ExpressionEvaluator::getOpValues(const AST::expr &val, std::vector<ScriptVariant> &value) const
{
	EvaluationList result;
	if (!this->evaluate(result, val)) return false;
	value = result.toStdVector();
	return true;
}
