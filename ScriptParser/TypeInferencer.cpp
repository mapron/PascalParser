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

#include "TypeInferencer.h"
#include "ExpressionEvaluator.h"

class TypeVisitor : public boost::static_visitor<RefType> {
public:
	TypeInferencer* _gen;
	TypeVisitor(TypeInferencer* gen):_gen(gen){ }
	template <class ASTnode>
	RefType operator ()(const ASTnode& node) const {
		return _gen->typeInference(node);
	}
};

QList<ScriptVariant::Types> TypeInferencer::_opValue_typePriority =
		QList<ScriptVariant::Types>()
		<< ScriptVariant::T_bool
		<< ScriptVariant::T_string_char
		<< ScriptVariant::T_int8_t
		<< ScriptVariant::T_uint8_t
		<< ScriptVariant::T_int16_t
		<< ScriptVariant::T_uint16_t
		<< ScriptVariant::T_int32_t
		<< ScriptVariant::T_uint32_t
		<< ScriptVariant::T_int64_t
		<< ScriptVariant::T_uint64_t
		<< ScriptVariant::T_float32
		<< ScriptVariant::T_float64
		<< ScriptVariant::T_string
		   ;



TypeInferencer::TypeInferencer(AST::CodeMessages *errors, SymTable *tab, CodeBlockProcessInterface *primaryProcessor)
	:_errors(errors), _tab(tab), _primaryProcessor(primaryProcessor), _currentClass(nullptr)
{
}

#define TR_notImplemented(ast) \
RefType TypeInferencer::typeInference(const AST::ast &val) \
{\
	_errors->Error(val._loc,QString("%1 is not implemented!").arg(#ast));\
	return _tab->getUndefinedType();\
}


RefType TypeInferencer::typeInference(const bool &val)
{
	return _tab->findType("bool");
}

RefType TypeInferencer::typeInference(const int64_t &val)
{
	return _tab->findType("int64");
}

RefType TypeInferencer::typeInference(const double &val)
{
	return _tab->findType("float64");
}

RefType TypeInferencer::typeInference(const QString &val)
{
	return _tab->findType("string");
}

RefType TypeInferencer::typeInference(const AST::set_construct &val)
{
	return _tab->getUndefinedType();
}

RefType TypeInferencer::typeInference(const AST::expr &val)
{
	return boost::apply_visitor(TypeVisitor(this), val._expr);
}

RefType TypeInferencer::typeInference(const AST::expr_list &val)
{
	if (val._exprs.size() == 0) return _tab->getUndefinedType();
	else return this->typeInference(val._exprs.last());
}

RefType TypeInferencer::typeInference(const AST::primary &val)
{
	return _primaryProcessor->processBlock(val).type;
}

RefType TypeInferencer::typeInference(const AST::unary &val)
{
	RefType left = this->typeInference(val._expr);
	return left;
}
RefType TypeInferencer::binaryOperationType(RefType left, RefType right) const
{
	RefType ret = left;
	if (left._type->isScalar() && right._type->isScalar()) {
		int leftPri = _opValue_typePriority.indexOf(left._type->_opcodeType);
		int rightPri = _opValue_typePriority.indexOf(right._type->_opcodeType);
		if (rightPri > leftPri) {
			 ret = right;
		}
	}
	return ret;
}

void TypeInferencer::checkBinary(RefType left, RefType right, const AST::binary &val)
{
	QString errReport;
	ScriptVariant::Types  leftType= left._type->_opcodeType;
	ScriptVariant::Types  rightType= right._type->_opcodeType;

	bool leftIsBool = leftType == ScriptVariant::T_bool;
	bool rigthIsBool = rightType == ScriptVariant::T_bool;
	if (left._type->isUndefined()) {
		  errReport= "Left operand is undefined.";
	}
	else if (right._type->isUndefined()) {
		  errReport= "Right operand is undefined.";
	}
	if ((leftIsBool || rigthIsBool) && (!leftIsBool || !rigthIsBool)) {
		//TODO:
	}
	if (   (val._op == BytecodeVM::DIV)
		|| (val._op == BytecodeVM::MOD)) {
		if (!ScriptVariant::isTypeInt(leftType) || !ScriptVariant::isTypeInt(rightType)) {
			errReport= "Integral binary operation: Invalid type of arguments.";
		}
	}

	if (!errReport.isEmpty()) {
		Error(val, errReport);
	}
}


RefType TypeInferencer::typeInference(const AST::binary &val)
{
	RefType left = this->typeInference(val._left);
	RefType right = this->typeInference(val._right);
	checkBinary (left, right, val);
	RefType ret = binaryOperationType(left, right);
	if (val._op >= BytecodeVM::AND && val._op <= BytecodeVM::NE) {
		ret._type = _tab->findType("bool");
	}
	ret._isRef = false;
	return ret;
}

RefType TypeInferencer::typeInference(const AST::internalexpr &val)
{
	RefType left = _tab->getUndefinedType();
	static QSet<int> intTypes = QSet<int>()
			<<AST::internalexpr::tLow
			<<AST::internalexpr::tHigh;
	static QSet<int> doubleTypes = QSet<int>()
			<<AST::internalexpr::tAdd
			<<AST::internalexpr::tMax
			<<AST::internalexpr::tMin
			<<AST::internalexpr::tMul
			<<AST::internalexpr::tMux
				;
	static QSet<int> boolTypes = QSet<int>()
			<<AST::internalexpr::tAnd
			<<AST::internalexpr::tEq
			<<AST::internalexpr::tNe
			<<AST::internalexpr::tGe
			<<AST::internalexpr::tGt
			<<AST::internalexpr::tLe
			<<AST::internalexpr::tLt
			<<AST::internalexpr::tOr
			<<AST::internalexpr::tXor
			;

	if (intTypes.contains(val._type)) {
		left = _tab->findType("integer");
	}else if (doubleTypes.contains(val._type)) {
		left = _tab->findType("double");
	}else if (boolTypes.contains(val._type)) {
		left = _tab->findType("bool");
	}else if (val._type == AST::internalexpr::tConcat) {
		left = _tab->findType("string");
	}

	return left;
}

RefType TypeInferencer::typeInference(const AST::constant &val)
{
	RefType ret = boost::apply_visitor(TypeVisitor(this), val._constant);
	ret._isConst = true;
	ret._isLiteral = true;
	return ret;
}

RefType TypeInferencer::typeInference(const boost::blank &val)
{
	return _tab->getUndefinedType();
}

void TypeInferencer::addVars(TypeDef &typeObject,
							 const AST::var_decls &var_decls)
{
	foreach (const AST::var_decl & var_decl,  var_decls) {
		RefType t = this->typeInference(var_decl._type);
		foreach (const AST::ident &ident, var_decl._idents) {
			typeObject.addField(ident._ident, t._type);
		}
	}
}

RefType TypeInferencer::typeInference(const AST::type &val)
{
	return boost::apply_visitor(TypeVisitor(this), val._type);
}

RefType TypeInferencer::typeInference(const AST::simple_type &val)
{
	RefType type;
	type._type = _tab->findType(val._ident._ident);
	if (type._type->isUndefined()) {
		Error(val, "Undefined type");
	}
	return type;
}

RefType TypeInferencer::typeInference(const AST::array_type &val)
{
	TypeDef arrType;
	if (val._array_ranges.size() !=1) {
		Error(val, "Arrays with dimension other than 1 not supported");
		return _tab->getUndefinedType();
	}

	if (!ExpressionEvaluator().getInt(val._array_ranges[0]._boundHigh, arrType._arrayHighBound )) {
		Error(val, "Arrays should have integral dimension");
		return _tab->getUndefinedType();
	}

	if (val._array_ranges[0]._hasLow) {
		if (!ExpressionEvaluator().getInt(val._array_ranges[0]._boundLow, arrType._arrayLowOffset )) {
			Error(val, "Arrays should have integral dimensions");
			return _tab->getUndefinedType();
		}
	}

	arrType._category = TypeDef::Array;
	RefType child = this->typeInference(val._type);
	arrType._child.push_back(child._type);

	return _tab->registerType(arrType);
}
TR_notImplemented(set_type)
TR_notImplemented(file_type)
RefType TypeInferencer::typeInference(const AST::pointer_type &val)
{
	TypeDef recType;
	recType._category = TypeDef::Pointer;
	recType._child << this->typeInference(val._type)._type;

	return _tab->registerType(recType);
}
TR_notImplemented(subrange_type)
TR_notImplemented(enum_type)
TR_notImplemented(function_type)
RefType TypeInferencer::typeInference(const AST::class_type &val)
{
	TypeDef classType;
	classType._category = TypeDef::Class;
	if (_typeNamesStack.isEmpty()){
		Error(val, "Unnamed classes are not allowed.");
		return _tab->getUndefinedType();
	}
	PTypeDef parentTypeDef = nullptr;
	if (val._parent._ident.size()) {
		parentTypeDef = _tab->findType(val._parent._ident);
		if (!parentTypeDef) {
			Error(val, "Undefined base class:" + val._parent._ident);
			return _tab->getUndefinedType();
		}
	}
	classType._parent = parentTypeDef;


	QString className = _typeNamesStack.last();
	ClassObj *classObj = _tab->createNewClassObj(className, PTypeDef(), val._parent._ident);
	if (!classObj) {
		Error(val, "Class name is already used: " + className);
		return _tab->getUndefinedType();
	}

	foreach (const AST::class_section& sec, val._sections) {
		addVars(classType, sec._var_decls/*, RefType::Public*/);
	}

	PTypeDef t =_tab->registerType(classType);
	_tab->setNameForType(t, className);
	classObj->setClassDefinition(t);
	_currentClass = classObj;
	foreach (const AST::class_section& sec, val._sections) {
		foreach (AST::proc_def proc_def, sec._proc_fwd_decls) {
		   proc_def._proc_decl._class._ident = className;
		   _classBuffer << _primaryProcessor->compileDecl(proc_def).decl;
		}
	}
	_currentClass = nullptr;

	return t;
}
TR_notImplemented(class_section)
