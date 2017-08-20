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

#include "CodeGenerator.h"
#include "SymTable.h"
#include "Parser.h"
#include "ExpressionEvaluator.h"

class CompileVisitor : public boost::static_visitor<OpcodeSequence> {
public:
	CodeGenerator* _gen;
	CompileVisitor(CodeGenerator* gen):_gen(gen){ }
	template <class ASTnode>
	OpcodeSequence operator ()(const ASTnode& node) const {
		return _gen->compile(node);
	}
};

class CompileVisitorDecl : public boost::static_visitor<DeclarationAndInit> {
public:
	CodeGenerator* _gen;
	CompileVisitorDecl(CodeGenerator* gen):_gen(gen){ }
	template <class ASTnode>
	DeclarationAndInit operator ()(const ASTnode& node) const {
		return _gen->compileDecl(node);
	}
};

// **********************************************************************************************

#define CG_notImplemented(ast) \
OpcodeSequence CodeGenerator::compile(const AST::ast &val) \
{\
	OpcodeSequence ret;\
	_errors->Error(val._loc,QString("%1 is not implemented!").arg(#ast));\
	return ret;\
}

CodeGenerator::CodeGenerator(AST::CodeMessages* errors, ScriptVM *executor)
{
	_errors = errors;
	_tab = new SymTable(_errors);
	_executor = executor;
	_isTopBlock = false;
	_currentFunc = nullptr;
	_parserOptions = 0;
	_types = new TypeInferencer(_errors, _tab, this);
}

CodeGenerator::~CodeGenerator()
{
	delete _types;
	delete _tab;
}

void CodeGenerator::clear()
{
	_tab->clear();
	_executor->clear();
	_withObjects.clear();
}

// =============================== blocks =============================================

// most complex part of compiler. Processes primary designators like "array[123].field.method()"
CodeBlockInfo CodeGenerator::processBlock(const AST::primary &val)
{
	CodeBlockInfo result;
	result.type = _tab->getUndefinedType();

	const AST::ident               * ident = boost::get<AST::ident>(&val._expr);
	const AST::expr_list       * expr_list = boost::get<AST::expr_list>(&val._expr);
	const AST::constant         * constant = boost::get<AST::constant>(&val._expr);
	const AST::internalexpr * internalexpr = boost::get<AST::internalexpr>(&val._expr);

	if (constant)
	{
		if (val._accessors.size()) Error(val, "Acessors on constants not supported.");
		return this->processBlock(*constant);
	}
	if (expr_list)
	{
		if (val._accessors.size()) Error(val, "Acessors on expression lists not supported.");
		return this->processBlock(*expr_list);
	}
	if (internalexpr)
	{
		if (val._accessors.size()) Error(val, "Acessors on internal statements is not supported.");
		return this->processBlock(*internalexpr);
	}
	if (!ident) { Error(val, "Internal compiler error."); return result; }


	AST::primary prim = val;

	QString objectName = ident->_ident.toLower();
	if (_currentFunc && objectName == _currentFunc->getName()) objectName = "Result";

	MetaObj current(_tab);
	current.setClassObj(_types->_currentClass);
	current.funcObj = _currentFunc ; // setFuncObj is forbidden.
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());

	bool hasWith = false;
	if (!current.findAny(objectName, MetaObj::ffAllGlobal)){

		foreach (CodeBlockInfo withObj, _withObjects)
		{  // in "with" context
			current = withObj.obj;
			current.doAccess();
			if (current.findAny(objectName, MetaObj::ffAllGlobal))
			{
				hasWith = true;
				ret=withObj.code;

				ret.EmitAddref( current.fieldOffset);

				break;
			}
		}
	}


	// ================================
	if (current.metatype == MetaObj::mNone)
	{
		Error(prim, "Undeclared symbol: " + ident->_ident);
		return result;
	}


	if(current.isRefable() && !hasWith)
	{
		BytecodeVM::OpCodeType r = BytecodeVM::REF;
		if (current.varObj->isExternal()) r = BytecodeVM::REFEXT;
		if (current.varObj->isStatic())   r = BytecodeVM::REFEXT;
		bool autoderef = true;
		if (current.varObj->getType()._type->isPointer())
			autoderef = false;
		BytecodeVM & o =ret.Emit(r,
				 current.varObj->getMemoryAddress(),
				 current.varObj->getScopeLevel(),
				 current.varObj->getType()._type->getByteSize()   // this takes deref in account.
				 );
		o.values.push_back(ScriptVariant(autoderef));
		ret.EmitAddref(current.fieldOffset);

	}

	bool err = false;
	OpcodeSequence resultsSequence;
	foreach (const AST::primary_accessor &p, prim._accessors)
	{
		const AST::call_expr    * call_expr    = boost::get<AST::call_expr>(&p._primary_accessor);
		const AST::address_expr * address_expr = boost::get<AST::address_expr>(&p._primary_accessor);
		const AST::deref_expr   * deref_expr   = boost::get<AST::deref_expr>(&p._primary_accessor);
		const AST::idx_expr     * idx_expr     = boost::get<AST::idx_expr>(&p._primary_accessor);
		const AST::access_expr  * access_expr  = boost::get<AST::access_expr>(&p._primary_accessor);
		ret.setLocVal(p);

		if (call_expr)
		{
			if (!current.isCallable() )
			{
				Error(*ident, QString("Only functions could be called."));
				err = true; break;
			}
			_tab->_lastFuncObj = current.funcObj;
			AST::expr_list callArgs;
			callArgs = call_expr->_args;
			callArgs._loc = ident->_loc;
			QString errText;
			OpcodeSequence resultSequence;
			if (!emitCall(current.funcObj, callArgs, errText, ret,resultSequence))
			{
				if (errText.size())
				{
					Error(*ident, errText);
				}
				err = true; break;
			}
			resultsSequence.insert(resultsSequence.begin(),
								   resultSequence.begin(),
								   resultSequence.end());

			if(!current.doCall() )
			{
				Error(*call_expr, QString("Failed to make call."));
				err = true; break;
			}
		}
		if (address_expr)
		{
			if (!current.doAddress())
			{
				Error(*address_expr, QString("Failed to get address"));
				err = true; break;
			}
		}
		if (deref_expr)
		{
			if (!current.doDeref())
			{
				Error(*deref_expr, QString("Failed to derefernce"));
				err = true; break;
			}
			ret.Emit(BytecodeVM::DEREF, int(1));
		}
		if (idx_expr)
		{

			foreach (const AST::expr& expr, idx_expr->_indexes._exprs)
			{
				if (current.doIndex()) {
					ret << this->compile(expr);
					ret.Emit(BytecodeVM::IDX, current.type()._type->getByteSize(), current.low);
				}else if (current.doIndexStr()) {
					ret << this->compile(expr);
					ret.Emit(BytecodeVM::IDX_STR);
				}else{
					Error(expr, QString("Only arrays and strings could be indexed."));
					err = true; break;
				}
			}
		}
		if (access_expr)
		{
			if (!current.doAccess())
			{
				Error(*ident, QString("Only variables or methods could accessed."));
				err = true; break;
			}
			QString member = access_expr->_field._ident;
			if (!current.findAny(member, MetaObj::ffAllObject))
			{
				Error(*access_expr, "Invalid member: " + member);
				err = true; break;
			}
			ret.EmitAddref(current.fieldOffset);
		}
	}

	if (_tab->_doAC &&  current.wrapperClass)
	{
		_tab->_autoCompleteList = current.wrapperClass->getMemberList();
		_tab->_hasAC = true;
	}
	else if (_tab->_doAC &&  current.objectClass)
	{
		_tab->_autoCompleteList = current.objectClass->getMemberList();
		_tab->_hasAC = true;
	}

	if (err)
		return result;
	else
		_tab->_hasAC = false;

	if (current.isCallable() && !current.callDone)
	{
		Error(val, "Function call expression missing.");
		return result;
	}

	result.type = current.type();
	result.obj = current;
	result.code << resultsSequence;
	result.code << ret;
	result.code.optimize();
	return result;
}

CodeBlockInfo CodeGenerator::processBlock(const AST::constant &val)
{
	CodeBlockInfo result;
	result.code = this->compile(val);
	result.type = _types->typeInference(val);
	return result;
}

CodeBlockInfo CodeGenerator::processBlock(const AST::expr_list &val)
{
	CodeBlockInfo result;
	result.code = this->compile(val);
	result.type = _types->typeInference(val);
	return result;
}

CodeBlockInfo CodeGenerator::processBlock(const AST::internalexpr &val)
{
	CodeBlockInfo result;
	result.code = this->compile(val);
	result.type = _types->typeInference(val);
	return result;
}

// =============================== EXPRESSIONS =================================

OpcodeSequence CodeGenerator::compile(const AST::expr &val)
{
	return boost::apply_visitor(CompileVisitor(this), val._expr);
}
OpcodeSequence CodeGenerator::compile(const AST::expr_list &val)
{
	OpcodeSequence ret;
	foreach (const AST::expr& expr, val._exprs)
		ret << this->compile(expr);
	return ret;
}
OpcodeSequence CodeGenerator::compile(const AST::primary &val)
{
	return this->processBlock(val).code;
}

bool CodeGenerator::emitCall(FuncObj *function,
							 AST::expr_list &callArgs,
							 QString &prim,
							 OpcodeSequence &mainSequence,
							 OpcodeSequence &resultSequence)
{
	BytecodeVM::OpCodeType r =  BytecodeVM::CALL;
	if (function->isExternal()) r = BytecodeVM::CALLEXT;
	int argsSize = callArgs._exprs.size();
	int signatureSize = function->getArgumentsNumber();
	if (argsSize > signatureSize)
	{
		prim= QString("Passed %1 parameters, expected %2.").arg(argsSize).arg(signatureSize);
		return false;
	}
	resultSequence.setLocVal(callArgs);
	resultSequence.setScope(_tab->getCurrentScope());

	// allocating result on stack
	foreach (auto typeCnt, function->getType()._type->getSignature())
		resultSequence.EmitPush(typeCnt.first, typeCnt.second);

	QList<AST::expr> realPassed;
	QList<int> realPassedDeref;
	QHash<QString, int> name2index;

	QList<FuncObj::FunctionArg> visibleArguments;
	foreach (const FuncObj::FunctionArg &arg, function->getArguments())
	{
		AST::expr t;
		if (arg._initializer.get()) t = *arg._initializer;
		if (arg._name.toLower() == "self")
			continue;

		visibleArguments << arg;
		name2index[arg._name.toLower()] = realPassed.size();
		realPassed << t;
		realPassedDeref << 0;
	}
	for (int callArgsIndex = 0; callArgsIndex < argsSize; ++callArgsIndex)
	{
		//NOTE: !!! ugly code
		const AST::expr& arg = callArgs._exprs[callArgsIndex];
		RefType argType = _types->typeInference(arg);
		QString id = callArgs._idents.value(callArgsIndex)._ident.toLower();
		int realPassedIndex = callArgsIndex ;
		if (id.size()) realPassedIndex = name2index.value(id, -1);
		if (realPassedIndex < 0)
		{
			 prim= QString("Undefined argument:%1").arg(id);
			 return false;
		}
		if (realPassedIndex >= realPassed.size())
		{
			 prim= QString("Passed %1 argument, but maximum: %2").arg(realPassedIndex + 1).arg(realPassed.size());
			 return false;
		}

		realPassed[realPassedIndex] = arg;

		RefType sigType = visibleArguments[realPassedIndex]._type;
		bool sizeCheck = false;
		if (argType._isRef == sigType._isRef)
		{
			//OK
			sizeCheck = true;
		} else if (!argType._isRef && sigType._isRef)
		{
			//Error(arg, QString("Reference needed.")); return false;
			//error
		} else if (argType._isRef && !sigType._isRef)
		{
			//OK, deref
			sizeCheck = true;
			realPassedDeref[realPassedIndex]=sigType._type->getByteSize();
		}
		if (sizeCheck && sigType.getByteSize() != argType.getByteSize())
		{
			Error(arg, QString("Invalid parameter size: expected %1, get %2.").arg(sigType.getByteSize()).arg(argType.getByteSize()));
			return false;
		}
		if (sigType._type->isScalar() && !argType._isLiteral)
		{
			int sigPriority = TypeInferencer::_opValue_typePriority.indexOf(sigType._type->_opcodeType);
			int argPriority = TypeInferencer::_opValue_typePriority.indexOf(argType._type->_opcodeType);
			if (sigPriority < argPriority)
			{
				Error(arg, QString("Function argument truncated: expected %1, get %2.").arg(sigType.getType()->getAlias()).arg(argType.getType()->getAlias()));
				return false;
			}
		}
	}
	for (int i = 0; i < realPassed.size(); ++i)
	{
		if (realPassed[i]._expr.which() == 0)
		{
			 prim = QString("Undefined parameter: %1").arg(visibleArguments[i]._name);
			 return false;
		}
		mainSequence << this->compile(realPassed[i]);
		if (realPassedDeref[i])
			mainSequence.Emit(BytecodeVM::DEREF, realPassedDeref[i]);
	}
	mainSequence.setLoc(callArgs._loc._file,
						callArgs._loc._line,
						callArgs._loc._col);
	BytecodeVM &callOpCode =  mainSequence.Emit(r);
	callOpCode.values.resize(4);
	callOpCode.values[0].setValue( 0, ScriptVariant::T_AUTO);
	callOpCode.values[1].setValue( function->callSize() , ScriptVariant::T_AUTO);
	callOpCode.values[2].setValue( function->returnSize(), ScriptVariant::T_AUTO);
	callOpCode.values[3].setValue( function->getScopeLevel(), ScriptVariant::T_AUTO);
	callOpCode.gotoLabel = function->getFullName().toStdString();

	return true;
} // CodeGenerator::emitCall


AST::expr_list CodeGenerator::flatExprList(const AST::expr &expr)
{
	AST::expr_list ret;
	if (expr._expr.which() == AST::expr::t_primary)
	{
		 const AST::primary& primary =  boost::get<AST::primary>(expr._expr);
		 if (primary._expr.which() == AST::primary::t_expr_list)
		 {
			 const AST::expr_list& list2 = boost::get<AST::expr_list>(primary._expr);
			 foreach (const AST::expr& expr2, list2._exprs)
				 ret._exprs << this->flatExprList( expr2 )._exprs;

			 return ret;
		 }
	}
	ret._exprs << expr;
	return ret;
}
OpcodeSequence CodeGenerator::compile(const AST::unary &val)
{
	RefType type  = _types->typeInference(val._expr);
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	ret << this->compile(val._expr);
	BytecodeVM::UnOp sourceOp =  val._op;

	if (sourceOp == BytecodeVM::UINV && !type._type->isInt())
		type._type = _tab->findType("int64");

	ret.Emit(BytecodeVM::UNOP, (int)sourceOp, (int)type._type->_opcodeType);
	return ret;
}
OpcodeSequence CodeGenerator::compile(const AST::binary &val)
{
	OpcodeSequence left = this->compile(val._left);
	OpcodeSequence right = this->compile(val._right);
	static QSet<int> binOpers = QSet<int>() << BytecodeVM::ANDBIN << BytecodeVM::ORBIN << BytecodeVM::XORBIN;

	RefType typeLeft  = _types->typeInference(val._left);
	RefType typeRight  = _types->typeInference(val._right);
	_types->checkBinary(typeLeft, typeRight, val);
	RefType typeBothExtended = _types->binaryOperationType(typeLeft, typeRight);
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	ret << left;
	ret << right;
	BytecodeVM::BinOp sourceOp =  val._op;
	int flags = BytecodeVM::bNo;
	if (sourceOp == BytecodeVM::EQ || sourceOp == BytecodeVM::NE)
	{
		int flags = 0;
		if (sourceOp == BytecodeVM::NE) flags |= BytecodeVM::cNot;
		if (typeLeft._isRef) flags |= BytecodeVM::cLeftIsRef;
		if (typeRight._isRef) flags |= BytecodeVM::cRightIsRef;
		ret.Emit(BytecodeVM::CMPS, flags, (int)typeBothExtended._type->getByteSize());
	}
	else
	{
		if (!typeLeft._type->isScalar() || !typeRight._type->isScalar())
		{
			Error(val, "Only compare allowed for non-scalar.");
			return ret;
		}
		if (typeBothExtended._type->isInt()) {
			if (sourceOp == BytecodeVM::AND) sourceOp = BytecodeVM::ANDBIN;
			if (sourceOp == BytecodeVM::OR ) sourceOp = BytecodeVM::ORBIN;
			if (sourceOp == BytecodeVM::XOR) sourceOp = BytecodeVM::XORBIN;
		} else {
			if (sourceOp == BytecodeVM::AND) sourceOp = BytecodeVM::ANDLOG;
			if (sourceOp == BytecodeVM::OR ) sourceOp = BytecodeVM::ORLOG;
		}
		if (binOpers.contains(sourceOp) && !typeBothExtended._type->isInt())
			typeBothExtended._type = _tab->findType("int64");

		ret.Emit(BytecodeVM::BINOP, (int)sourceOp, (int)typeBothExtended._type->_opcodeType, flags);
	}
	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::constant &val)
{
	OpcodeSequence ret;
	const bool * b = boost::get<bool>(&val._constant);
	const int64_t * i = boost::get<int64_t>(&val._constant);
	const double * d = boost::get<double>(&val._constant);
	const QString * s = boost::get<QString>(&val._constant);
	const AST::set_construct * sc = boost::get<AST::set_construct>(&val._constant);
	ScriptVariant value;
	if (b) value.setValue(*b, ScriptVariant::T_AUTO);
	if (i) value.setValue(*i, ScriptVariant::T_AUTO);
	if (d) value.setValue(*d, ScriptVariant::T_AUTO);
	if (s) value.setValue(std::string(s->toUtf8().constData()), ScriptVariant::T_AUTO);

	if (sc) { //TODO: pascal set impl: [1, 2, 3]
		assert(0);
	}
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	ret.EmitPush( value );

	return ret;
}

OpcodeSequence  CodeGenerator::mkBinSeq(const AST::internalexpr &val, int op, int type)
{
	OpcodeSequence ret;
	ret << this->compile(val._expr_list);
	ret.Emit(BytecodeVM::MULTOP, op, type, (int)val._expr_list._exprs.size());
	return ret;
}

OpcodeSequence  CodeGenerator::mkLogicalSeq(const AST::internalexpr &val, const OpcodeSequence& opers)
{
	OpcodeSequence ret;
	for (int i=1;i<val._expr_list._exprs.size();i++)
	{
		ret << this->compile(val._expr_list._exprs[i-1]);
		ret << this->compile(val._expr_list._exprs[i]);
		ret << opers;
		if (i > 1)
			ret.Emit(BytecodeVM::BINOP, (int)BytecodeVM::ANDLOG, (int)ScriptVariant::T_bool, 0);
	}
	return ret;
}
OpcodeSequence  CodeGenerator::mkLogicalSeq2(const AST::internalexpr &val, int op, int type)
{
	OpcodeSequence opers;
	opers.Emit(BytecodeVM::BINOP, op, type, 0);
	return mkLogicalSeq(val, opers);
}

OpcodeSequence CodeGenerator::compile(const AST::internalexpr &val)
{
	OpcodeSequence ret;
	RefType typeRef = _types->typeInference(val._expr_list);
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	if (val._type == AST::internalexpr::tLow)
	{
		ret.EmitPush(typeRef._type->_arrayLowOffset);
		return ret;
	}
	if (val._type == AST::internalexpr::tHigh)
	{
		ret.EmitPush(typeRef._type->_arrayHighBound);
		return ret;
	}
	if (val._type == AST::internalexpr::tInc || val._type == AST::internalexpr::tDec)
	{
		ret << this->compile(val._expr_list);
		if (typeRef._type->_category == TypeDef::Pointer)
		{
			OpcodeSequence rightPart =  this->compile(val._expr_list);
			rightPart[0].values[3] = (ScriptVariant(true)); //TODO: wtf magic numbers...
			ret << rightPart;
			int pointedSize = typeRef._type->_child[0]->getByteSize();
			if (val._type == AST::internalexpr::tDec)
				pointedSize = -pointedSize;

			ret.EmitAddref(pointedSize);
			ret.Emit(BytecodeVM::MOVS, int(BytecodeVM::mLeftIsRef | BytecodeVM::mAddress), int(1));

		}else{
			int op = val._type == AST::internalexpr::tDec ? BytecodeVM::UDEC : BytecodeVM::UINC;
			ret.Emit(BytecodeVM::UNOP, op, (int)typeRef._type->_opcodeType);
			ret.Emit(BytecodeVM::POP, int(1));
		}

		return ret;
	}
	bool isBool = typeRef._type->isBoolean();
	// tMax, tMin,  tMux,
	if (val._type == AST::internalexpr::tAdd ) return mkBinSeq(val, BytecodeVM::PLUS, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tMul ) return mkBinSeq(val, BytecodeVM::MUL, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tAnd ) return mkBinSeq(val, isBool ? BytecodeVM::ANDLOG : BytecodeVM::ANDBIN, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tOr )  return mkBinSeq(val, isBool ? BytecodeVM::ORLOG : BytecodeVM::ORBIN  , typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tXor ) return mkBinSeq(val, isBool ? BytecodeVM::XOR : BytecodeVM::XORBIN, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tConcat ) return mkBinSeq(val, BytecodeVM::PLUS, ScriptVariant::T_string);
	if (val._type == AST::internalexpr::tEq )
	{
		OpcodeSequence opers;
		opers.Emit(BytecodeVM::CMPS, 0, 1, (int)typeRef._type->_opcodeType);
		return mkLogicalSeq(val, opers);
	}
	if (val._type == AST::internalexpr::tNe )
	{
		OpcodeSequence opers;
		opers.Emit(BytecodeVM::CMPS, (int)BytecodeVM::cNot, 1, (int)typeRef._type->_opcodeType);
		return mkLogicalSeq(val, opers);
	}
	if (val._type == AST::internalexpr::tGe ) return mkLogicalSeq2(val, BytecodeVM::GE, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tGt ) return mkLogicalSeq2(val, BytecodeVM::GT, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tLe ) return mkLogicalSeq2(val, BytecodeVM::LE, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tLt ) return mkLogicalSeq2(val, BytecodeVM::LT, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tMin )return mkBinSeq(val, BytecodeVM::MIN_, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tMax ) return mkBinSeq(val, BytecodeVM::MAX_, typeRef._type->_opcodeType);
	if (val._type == AST::internalexpr::tMux ) return mkBinSeq(val, BytecodeVM::MUX, typeRef._type->_opcodeType);

	Error(val,QString("Undefined internal expression type:%1").arg(val._type));
	return ret;
}


// =============================== STATEMENTS  =============================================
OpcodeSequence CodeGenerator::compile(const AST::statement &val)
{
	return boost::apply_visitor(CompileVisitor(this), val._statement);
}

OpcodeSequence CodeGenerator::compile(const AST::compoundst &val)
{
	OpcodeSequence ret;
	ret << this->compile(val._sequence);
	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::assignmentst &val)
{
	static const QMap<ScriptVariant::Types, int> bitSizes = []() -> QMap<ScriptVariant::Types, int> {
		QMap<ScriptVariant::Types, int> ret;
		ret[ScriptVariant::T_bool] = 1;
		ret[ScriptVariant::T_int8_t  ] = 8;
		ret[ScriptVariant::T_uint8_t ] = 8;
		ret[ScriptVariant::T_int16_t ] = 16;
		ret[ScriptVariant::T_uint16_t] = 16;
		ret[ScriptVariant::T_int32_t ] = 32;
		ret[ScriptVariant::T_uint32_t] = 32;
		ret[ScriptVariant::T_int64_t ] = 64;
		ret[ScriptVariant::T_uint64_t] = 64;

		ret[ScriptVariant::T_float32] = 24;
		ret[ScriptVariant::T_float64] = 48;
		ret[ScriptVariant::T_string] = 1000;
		return ret;
	}();

	OpcodeSequence left  = this->compile(val._left);
	OpcodeSequence right = this->compile(val._right);

	RefType typeLeft  = _types->typeInference(val._left);
	RefType typeRight = _types->typeInference(val._right);

	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	if (typeLeft._type->getByteSize() != typeRight._type->getByteSize())
	{
		_types->typeInference(val._right);
		Error(val, "type sizes are not equal in assignment.");
		return ret;
	}
	if (typeLeft._type->isScalar() && typeRight._type->isScalar())
	{
		int sizeBitLeft = bitSizes.value(typeLeft ._type->_opcodeType);
		int sizeBitRight= bitSizes.value(typeRight._type->_opcodeType);
		if (!typeRight._isLiteral)
		{
			if (sizeBitLeft < sizeBitRight)
				Warning(val, "Assignment types are inconsistent");
		}
	}
	if (typeLeft._type->isScalar() != typeRight._type->isScalar())
	{
		Error(val, "Assigning scalar to non-scalar and reverse is not allowed.");
		return ret;
	}
	if (typeLeft._type->isPointer() != typeRight._type->isPointer())
	{
		Error(val, "Assigning pointer to non-pointer and reverse is not allowed.");
		return ret;
	}

	if (typeLeft._isConst)
	{
		Error(val, "Assignment to constant is not allowed.");
		return ret;
	}

	ret << left;
	ret << right;

	int flags = BytecodeVM::mNo;
	if (typeLeft._isRef)  flags |= BytecodeVM::mLeftIsRef;
	if (typeRight._isRef && !typeRight._type->isPointer()) flags |= BytecodeVM::mRightIsRef;
	if (typeLeft._type->isPointer()) flags |= BytecodeVM::mAddress;

	if (!typeLeft._isRef)
	{
		Error(val, "Left side of assignment must be reference.");
		return ret;
	}
	ret.Emit(BytecodeVM::MOVS, (int)flags, typeLeft._type->getByteSize());
	return ret;
}
CG_notImplemented(gotost)

OpcodeSequence CodeGenerator::compile(const AST::switchst &val)
{
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());

	OpcodeSequence swVariable = this->compile(val._case);
	RefType varType = _types->typeInference(val._case);

	OpcodeSequence elsePart = this->compile(val._else);

	QList<OpcodeSequence> caseBlocks;
	int totalSize = 0;

	foreach (const AST::case_list_elem& case_list_elem, val._case_list)
	{
		OpcodeSequence cmpAndBody;
		cmpAndBody.setLocVal(case_list_elem);
		cmpAndBody.setScope(_tab->getCurrentScope());
		OpcodeSequence body =this->compile(case_list_elem._statement);

		bool isFirst = true;
		foreach (const AST::expr& expr, case_list_elem._expr_list._exprs)
		{
			OpcodeSequence cmp = this->compile(expr);
			cmpAndBody << swVariable;
			cmpAndBody << cmp;
			cmpAndBody.Emit(BytecodeVM::CMPS,
							(int)0/* TODO: variable in switch could be REF.*/,
							(int)varType._type->getByteSize());
			if (!isFirst)
				cmpAndBody.Emit(BytecodeVM::BINOP, (int)BytecodeVM::OR, (int)varType._type->_opcodeType, (int)0);

			isFirst = false;
		}
		cmpAndBody.Emit(BytecodeVM::FJMP, (int)(body.size() + 2));
		cmpAndBody << body;
		totalSize += cmpAndBody.size() + 1;//JMP to end;

		caseBlocks << cmpAndBody;
	}
	totalSize += elsePart.size();
	foreach (const OpcodeSequence& info, caseBlocks)
	{
		ret << info;
		ret.Emit(BytecodeVM::JMP, (int)(totalSize - ret.size()));
	}
	ret << elsePart;
	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::ifst &val)
{
	OpcodeSequence ret;
	RefType exprType = _types->typeInference(val._expr);
	if (!exprType._type->isScalar())
	{
		Error(val, "condition should be scalar.");
		return ret;
	}

	OpcodeSequence ifbranch = this->compile(val._if);
	OpcodeSequence elsebranch;
	bool hasElse = val._else._statement.which();
	if (hasElse)
		elsebranch = this->compile(val._else);

	ifbranch.setLocVal(val._if);
	ifbranch.setScope(_tab->getCurrentScope());
	elsebranch.setLocVal(val._else);
	elsebranch.setScope(_tab->getCurrentScope());

	ifbranch.Prepend(BytecodeVM::FJMP, ifbranch.size() + 1 + hasElse);
	if (hasElse)
		elsebranch.Prepend(BytecodeVM::JMP, elsebranch.size() + 1);

	ret << this->compile(val._expr);
	ret << ifbranch;
	if (hasElse)
		ret << elsebranch;
	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::forst &val)
{
	/*
	for i:= low to high do
		starement;
	===
	i:=low;
	while ( i <= high) do
		statement;
		i := i + 1;
	end;
*/
	OpcodeSequence ret;
	RefType fromType = _types->typeInference(val._fromExpr);
	RefType toType = _types->typeInference(val._toExpr);
	if (!fromType._type->isScalar())
	{
		Error(val, "from should be scalar.");
		return ret;
	}
	if (!toType._type->isScalar())
	{
		Error(val, "to should be scalar.");
		return ret;
	}
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());


	AST::primary ident;
	ident._expr = val._ident;
	OpcodeSequence getIdentVal = this->compile(ident);

	AST::expr identExpr;
	identExpr._expr = ident;

	AST::assignmentst ass;
	ass._left =identExpr;
	ass._right = val._fromExpr;

	AST::binary compare;
	compare._left = identExpr;
	compare._right = val._toExpr;
	compare._op = val._downto ? BytecodeVM::GE : BytecodeVM::LE;


	OpcodeSequence checkCondition = this->compile(compare);

	OpcodeSequence modifier;
	modifier << getIdentVal;
	modifier.Emit(BytecodeVM::UNOP, int(val._downto ? BytecodeVM::UDEC : BytecodeVM::UINC), int(ScriptVariant::T_int32_t) );

	ret << this->compile(ass);// for i := 0
	ret << checkCondition;

	OpcodeSequence body = this->compile(val._statement);
	body << modifier;
	body.ReplaceBreak(body.size() + 2);
	body.ReplaceContinue(body.size() - modifier.size());

	ret.Emit(BytecodeVM::FJMP, body.size() + 3);

	ret << body;

	ret.Emit(BytecodeVM::POP, int(1)); //TODO: expr could be more than sizeof==1 !
	ret.Emit(BytecodeVM::JMP, -int(body.size() + checkCondition.size() + 2));

	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::whilest &val)
{
	 RefType condType = _types->typeInference(val._expr);
	 if (!condType._type->isBoolean())
		 Error(val, "condition should be boolean.");

	 OpcodeSequence checkCondition = this->compile(val._expr);
	 OpcodeSequence body = this->compile(val._statement);

	 OpcodeSequence ret;
	 ret.setLocVal(val);
	 ret.setScope(_tab->getCurrentScope());
	 ret << checkCondition;

	 ret.Emit(BytecodeVM::FJMP, body.size() + 2);

	 ret << body;

	 ret.Emit(BytecodeVM::JMP, -int(body.size() + checkCondition.size() + 1));

	 return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::repeatst &val)
{
	RefType condType = _types->typeInference(val._expr);
	if (!condType._type->isBoolean())
		Error(val, "condition should be boolean.");

	OpcodeSequence checkCondition = this->compile(val._expr);
	OpcodeSequence body = this->compile(val._sequence);

	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());

	ret << body;
	ret << checkCondition;

	ret.Emit(BytecodeVM::FJMP, -int(body.size() + checkCondition.size() ));

	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::procst &val)
{
	CodeBlockInfo info = processBlock(val._primary);
	int size = 0;
	if (info.type.isValid())
	{
	   Warning(val, "Function result is ignored");
	   size = info.type._type->getByteSize();
	   info.code.Emit(BytecodeVM::POP, size);
	}
	return info.code;
}
OpcodeSequence CodeGenerator::compile(const AST::withst &val)
{
	CodeBlockInfo withObj = this->processBlock(val._expr);
	OpcodeSequence ret;
	_withObjects.push_front( withObj );
	ret << this->compile(val._statement);
	_withObjects.pop_front();
	return ret;
}

CG_notImplemented(labelst)
CG_notImplemented(tryst)
OpcodeSequence CodeGenerator::compile(const AST::writest &val)
{
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	for (int i=0; i < val._write_list._write_params.size();i++)
	{
		bool ln =val._isLn && i == val._write_list._write_params.size() - 1;
		ret << this->compile(val._write_list._write_params[i]);
		RefType wrType = _types->typeInference(val._write_list._write_params[i]._expr);
		ret.Emit(BytecodeVM::WRT, wrType._type->getByteSize(), (int)ln);
	}

	return ret;
}

CG_notImplemented(readst)
CG_notImplemented(strst)
CG_notImplemented(raisest)
CG_notImplemented(inheritedst)
CG_notImplemented(onst)

OpcodeSequence CodeGenerator::compile(const AST::internalfst &val)
{
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	switch (val._type)
	{
		case AST::internalfst::tExit:{
			ret.Emit(BytecodeVM::RET);
		}break;
		case AST::internalfst::tBreak:{
			ret.Emit(BytecodeVM::JMP, int(0)).symbolLabel = "__break";
		}break;
		case AST::internalfst::tContinue:{
			ret.Emit(BytecodeVM::JMP, int(0)).symbolLabel  = "__continue";
		}break;
	}

	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::write_param &val)
{
	OpcodeSequence ret;
	ret.setLocVal(val);
	ret.setScope(_tab->getCurrentScope());
	if (val._constants.size()) //TODO: [0] - size, [1] - after dot.
		_errors->Info(val._loc, "write attributes unsupported");
	ret << this->compile(val._expr);
	return ret;
}

// =============================== DECLARATIONS=============================================

DeclarationAndInit CodeGenerator::compileDecl(const AST::decl_part_list &val)
{
	DeclarationAndInit ret;
	foreach (const AST::decl_part &d, val._parts)
	{
		DeclarationAndInit part = this->compileDecl(d);
		ret.decl << part.decl;
		ret.init << part.init;
	}
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::decl_part &val)
{
	return boost::apply_visitor(CompileVisitorDecl(this), val._decl_part);
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::label_decl_part &val)
{
	DeclarationAndInit ret;
	_errors->Error(val._loc,QString("not implemented!"));
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::const_def_part &val)
{
	DeclarationAndInit ret;
	foreach (const AST::const_def &const_def, val._const_defs)
	{
		RefType type = _types->typeInference(const_def._type);
		if (!type.isValid())
			type = _types->typeInference(const_def._initializer);// try to inference type from expr.

		OpcodeSequence ini = this->compile(const_def._initializer);
		ret.init << ini;
		VarObj* varObj = _tab->createNewRegularVarObj(const_def._ident._ident, type._type, true);

		if (!varObj)
		{
			Error(const_def, "Dublicate ident:" + const_def._ident._ident);
			continue;
		}
	}
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::type_def_part &val)
{
	DeclarationAndInit ret;
	foreach (const AST::type_def &type_def, val._type_defs)
	{
		QString typeName = type_def._ident._ident.toLower();
		_types->_typeNamesStack.push_back(typeName);
		_types->_classBuffer.clear();
		RefType t = _types->typeInference(type_def._type);
		ret.decl << _types->_classBuffer;
		_types->_classBuffer.clear();
		_types->_typeNamesStack.pop_back();
		if (!_tab->setNameForType(t._type, typeName ))
			Error(type_def, "Failed to register type " + typeName );
	}
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::var_decl_part &val)
{
	DeclarationAndInit ret;
	foreach (const AST::var_decl &decl,  val._var_decls)
	{
		 DeclarationAndInit part = this->compileDecl(decl);
		 ret.decl << part.decl;
		 ret.init << part.init;
	}
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::proc_def &val)
{
	DeclarationAndInit ret;
	OpcodeSequence resultOpcodes;
	resultOpcodes.setLocVal(val);
	resultOpcodes.setScope(_tab->getCurrentScope());
	SymTable::FunctionRec fun;
	fun._name = val._proc_decl._ident._ident;
	if (val._proc_decl._flags & AST::proc_decl::IsFunction)
		fun._type = _types->typeInference( val._proc_decl._type )._type;

	if (val._proc_decl._class._ident.size())
	{
		FuncObj::FunctionArg arg;
		arg._name = "self";
		arg._ref = true;
		if (!val._proc_decl._class._ident.isEmpty())
			arg._type = RefType(_tab->findType(val._proc_decl._class._ident));
		else throw std::runtime_error("CodeGenerator::compileDecl(const AST::proc_def&): "
									 "impossible state.");
		arg._type._isRef = true;
		fun._args << arg;
	}
	foreach (const AST::formal_param& formal_param, val._proc_decl._formal_params)
	{
		const AST::var_decl* var_decl = boost::get<AST::var_decl> (&formal_param._formal_param);
		if (!var_decl)
		{
			Error(formal_param, "Params only var type supported.");
			continue;
		}
		FuncObj::FunctionArg arg;
		arg._type = _types->typeInference(var_decl->_type)._type;
		arg._ref = formal_param._flags & AST::formal_param::IsVar;
		arg._initializer.reset(new AST::expr( var_decl->_initializer ));
		foreach (const AST::ident& ident, var_decl->_idents)
		{
			arg._name = ident._ident;
			fun._args << arg;
		}
	}

	QString ownerClassName = val._proc_decl._class._ident;
	_types->_currentClass = nullptr;
	if (!ownerClassName.isEmpty())
	{
		_types->_currentClass = _tab->findClass(ownerClassName);
		if (!_types->_currentClass)
			Error(val, "Class does not exist: " + ownerClassName);
	}
	// simple function, not a method:

	FuncObj *funcObj = _tab->createNewMethodObj(_types->_currentClass,
												fun,
												val._isForward,
												false,
												false);

	if (!funcObj)
	{
		Error(val, "Function exists:" + fun._name);
		return ret;
	}
	_currentFunc = funcObj;
	if (!val._isForward)
	{
		_tab->openScope(funcObj->getInternalScope());
		resultOpcodes.setScope(_tab->getCurrentScope());
		resultOpcodes.setLocVal(val._block._compoundst);

		if (val._proc_decl._flags & AST::proc_decl::IsFunction)
			_tab->createNewRegularVarObj("Result", fun._type);

		foreach (const  FuncObj::FunctionArg& arg, fun._args)
			_tab->createNewRegularVarObj(arg._name, arg._type);

		DeclarationAndInit di = this->compileDecl(val._block._decl_part_list);
		resultOpcodes << di.decl;
		resultOpcodes << di.init;

		resultOpcodes << this->compile(val._block._compoundst);

		_tab->closeScope();

		resultOpcodes.Emit(BytecodeVM::RET);

	}
	_types->_currentClass = nullptr;
	_currentFunc = nullptr;
	if (resultOpcodes.size())
		 resultOpcodes[0].symbolLabel = funcObj->getFullName().toStdString();

	ret.decl = resultOpcodes;
	return ret;
}
DeclarationAndInit CodeGenerator::compileDecl(const AST::unit_spec &val)
{
	DeclarationAndInit ret;
	return ret;
}
DeclarationAndInit CodeGenerator::compileDecl(const AST::uses_part &val)
{
	DeclarationAndInit ret;
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const AST::var_decl &val)
{
	DeclarationAndInit resultOpcodes;
	RefType type = _types->typeInference(val._type) ;
	bool isExternal = val._flags & AST::var_decl::IsExternal;
	if (isExternal && (_parserOptions& poForbidExternal))
	{
		Warning(val, "EXTERNAL keyword is useless for current script.");
		return resultOpcodes;
	}

	foreach (const AST::ident& ident, val._idents)
	{
		OpcodeSequence init;
		init.setLocVal(ident);
		init.setScope(_tab->getCurrentScope());
		if (val._initializer._expr.which() == 0)
		{
			if (!isExternal)
			{
				foreach (auto typeCnt, type._type->getSignature())
					init.EmitPush(typeCnt.first, typeCnt.second);
			}
		}
		else
		{
			AST::expr_list flatList = this->flatExprList(val._initializer);
			int i=-1;
			foreach (ScriptVariant::Types t, type._type->getExpandedSignature())
			{
				i++;
				if ( i >= flatList._exprs.size())
				{
					ScriptVariant val1;
					val1.setValue(0, t);
					init.EmitPush(val1);
				}
				else
				{
					const AST::expr& initExpr = flatList._exprs[i];
					RefType type = _types->typeInference(initExpr) ;
					init << this->compile(initExpr);
					if (!type._type->isScalar() || type._type->_opcodeType != t)
						init.Emit(BytecodeVM::CVRT, int(t));

				}
			}
		}

		if (_tab->findObj(ident._ident))
		{
			Error(ident, "Dublicate ident:" + ident._ident);
			continue;
		}

		VarObj *newVarObj;
		bool isConst = val._flags & AST::var_decl::IsConst;
		if (val._flags & AST::var_decl::IsStatic)
		{
			std::vector<ScriptVariant> resolvedValues;
			if (!ExpressionEvaluator().getOpValues(val._initializer, resolvedValues))
			{
				foreach (ScriptVariant::Types t, type.getType()->getExpandedSignature())
					resolvedValues.push_back(ScriptVariant(t));

			}
			if (type.getByteSize() != (int)resolvedValues.size())
			{
				Error(val, QString("Initializer should be %1 size").arg(type.getByteSize()) );
				return resultOpcodes;
			}
			if (_types->_currentClass && _currentFunc)
			{ // insede class method
				TypeDef* classType  = const_cast<TypeDef*>(_types->_currentClass->getType().getType()); // we modify class, adding hidden field.
				classType->addField(_currentFunc->getName() + "." + ident._ident, type.getType());

				continue;
			}

			newVarObj = _tab->createNewStaticVarObj(ident._ident, type);
			if (newVarObj)
			{
				int externalAddress = _executor->addStaticVariable(ident._ident.toStdString(), resolvedValues);
				newVarObj->changeMemoryAddress(externalAddress);
			}
		}
		else if (isExternal)
		{
			resultOpcodes.init << init;
			newVarObj = _tab->createNewExternalVarObj(ident._ident,
													  type,
													  isConst);

			newVarObj->changeMemoryAddress(_executor->addVariable(ident._ident.toStdString(), newVarObj->getType().getByteSize() ));
		}
		else
		{
			resultOpcodes.init << init;
			newVarObj = _tab->createNewRegularVarObj(ident._ident,
													 type,
													 isConst);
		}
	}
	return resultOpcodes;
}

// ===============================  PROGRAM    =============================================

OpcodeSequence CodeGenerator::compile(const AST::pascalSource &val)
{
	return boost::apply_visitor(CompileVisitor(this), val._pascal);
}

OpcodeSequence CodeGenerator::compile(const AST::program &val)
{
	_startAddress = "";
	_isTopBlock = true;
	return this->compile(val._block);
}

OpcodeSequence CodeGenerator::compile(const AST::stProgram &val)
{
	_startAddress = "";
	OpcodeSequence ret;

	DeclarationAndInit decls = this->compileDecl(val._decl_part_list);
	ret << decls.decl;


	if (val._hasProgram)
	{
		DeclarationAndInit declsBody =this->compileDecl(val._program);
		OpcodeSequence body;
		body << decls.init;
		body << declsBody.decl;
		body << declsBody.init;
		if (body.size() )
		{
			body[0].symbolLabel = "__start";
			_startAddress = "__start";
		}
		ret << body;
	}
	return ret;
}

CG_notImplemented(unit)

OpcodeSequence CodeGenerator::compile(const AST::sequence &val)
{
	OpcodeSequence ret;
	foreach (const AST::statement& s, val._statements)
		ret << this->compile(s) ;
	return ret;
}

OpcodeSequence CodeGenerator::compile(const AST::block &val)
{
	bool is_top = _isTopBlock;
	_isTopBlock = false;
	OpcodeSequence ret;
	_tab->openScope();
	DeclarationAndInit decls =  this->compileDecl(val._decl_part_list);
	ret << decls.decl;
	OpcodeSequence body;
	body << decls.init;
	body << this->compile(val._compoundst);
	if (body.size() && is_top)
	{
		body[0].symbolLabel = "__start";
		_startAddress = "__start";
	}
	ret << body;

	_tab->closeScope();
	return ret;
}

OpcodeSequence CodeGenerator::compile(const boost::blank &val)
{
	OpcodeSequence ret;
	_errors->Error(0,0,QString("Internal error: undefined node."));
	return ret;
}

DeclarationAndInit CodeGenerator::compileDecl(const boost::blank &val)
{
	DeclarationAndInit ret;
	_errors->Error(0,0,QString("Internal error: undefined node."));
	return ret;
}

