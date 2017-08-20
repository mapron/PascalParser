#include "ScriptVM.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <boost/type_traits.hpp>


template <typename T>
inline const T &qMin1(const T &a, const T &b) { return (a < b) ? a : b; }
template <typename T>
inline const T &qMax1(const T &a, const T &b) { return (a < b) ? b : a; }
template <typename T>
inline T qAbs1(const T &t) { return t >= 0 ? t : -t; }

template <typename T>
inline bool qFuzzyIsNull1(T p) { return p == 0; }

template <>
inline bool qFuzzyIsNull1<double>(double d) {return qAbs1(d) <= 0.000000000001; }

template <>
inline bool qFuzzyIsNull1<float>(float f) { return qAbs1(f) <= 0.00001f; }


#define CONDITIONAL_OP(name, cond)\
template <typename resT, typename T>\
inline void name(resT& ,const T& , const T& ,\
				 typename boost::disable_if<boost::cond<T> >::type*  = 0) { ;  }\
template <typename resT, typename T>\
inline void name(resT& res, const T& op1, const T& op2,\
				 typename boost::enable_if<boost::cond<T> >::type*  = 0)

CONDITIONAL_OP(PLUS_F, is_arithmetic)   { res = op1 + op2; }
CONDITIONAL_OP(MINUS_F, is_arithmetic)  { res = op1 - op2; }
CONDITIONAL_OP(MUL_F, is_arithmetic)    { res = op1 * op2; }
CONDITIONAL_OP(DIV_F, is_arithmetic)    { res = uint64_t(op1 / op2); }
CONDITIONAL_OP(DIVR_F, is_arithmetic)   { res = op1 / op2; }
CONDITIONAL_OP(MOD_F, is_integral)      { res = op1 % op2; }
CONDITIONAL_OP(ANDLOG_F, is_arithmetic) { res = op1 && op2; }
CONDITIONAL_OP(ANDBIN_F, is_integral)   { res = op1 & op2; }
CONDITIONAL_OP(ORLOG_F, is_arithmetic)  { res = op1 || op2; }
CONDITIONAL_OP(ORBIN_F, is_integral)    { res = op1 | op2; }
CONDITIONAL_OP(XORLOG_F, is_integral)   { res = bool(op1) ^ bool(op2); }
CONDITIONAL_OP(XORBIN_F, is_integral)   { res = op1 ^  op2; }
CONDITIONAL_OP(SHL_F, is_integral)      { res = op1 << op2; }
CONDITIONAL_OP(SHR_F, is_integral)      { res = op1 >> op2; }

CONDITIONAL_OP(MIN__F, is_arithmetic)   { res = op1 < op2 ? op1 : op2; }
CONDITIONAL_OP(MAX__F, is_arithmetic)   { res = op1 > op2 ? op1 : op2; }

CONDITIONAL_OP(LT_F, is_arithmetic)     { res = op1 < op2; }
CONDITIONAL_OP(GT_F, is_arithmetic)     { res = op1 > op2; }
CONDITIONAL_OP(LE_F, is_arithmetic)     { res = op1 <= op2; }
CONDITIONAL_OP(GE_F, is_arithmetic)     { res = op1 >= op2; }

CONDITIONAL_OP(INVERSE_F, is_integral)  { res = ~op1; (void)op2; }

template <typename T>
inline void CMP_F(bool &res, const T& p1,const T& p2){ res = (p1 == p2); }

template <>
inline void CMP_F<double>(bool &res,const double& p1, const double& p2)
{ res = (qAbs1(p1 - p2) <= 0.000000000001 * qMin1(qAbs1(p1), qAbs1(p2))); }
template <>
inline void CMP_F<float>(bool &res,const float& p1, const float& p2)
{ res = (qAbs1(p1 - p2) <= 0.00001f * qMin1(qAbs1(p1), qAbs1(p2))); }

inline void PLUS_F(std::string &res,const std::string& p1,const std::string& p2)
{ res = p1 + p2; } // concatenation.
inline void DIV_F(bool &res,const bool& ,const bool& )
{ res = false; }
inline void DIVR_F(bool &res,const bool& ,const bool& )
{ res = false; }
inline void MOD_F(bool &res,const bool& ,const bool& )
{ res = false; }
inline void SHL_F(bool &res,const bool& ,const bool& )
{ res = false; }
inline void SHR_F(bool &res,const bool& ,const bool& )
{ res = false; }

#define CASE_BIN(name, nameF) \
	 case BytecodeVM::name:{\
		   nameF(res , t1.getValue<T>(), t2.getValue<T>());\
		   Vres.setValue(res, ScriptVariant::T_AUTO); \
		};break

#define CASE_BIN_L(name, nameF) \
	 case BytecodeVM::name:{\
		   nameF(bres , t1.getValue<T>(), t2.getValue<T>());\
		   Vres.setValue(bres, ScriptVariant::T_bool); \
		};break

template <class T>
void makeBinaryOperation(BytecodeVM::BinOp op, ScriptVariant& Vres, const ScriptVariant& t1, const ScriptVariant& t2)
{
	T res = T();
	bool bres=false;

	switch(op)
	{
		CASE_BIN(PLUS, PLUS_F);
		CASE_BIN(MINUS, MINUS_F);
		CASE_BIN(MUL, MUL_F);
		CASE_BIN(DIV, DIV_F);
		CASE_BIN(DIVR, DIVR_F);
		CASE_BIN(MOD, MOD_F);

	  //  CASE_BIN_L(AND, ANDLOG_F);
		CASE_BIN_L(ANDLOG, ANDLOG_F);
	   // CASE_BIN_L(OR, ORLOG_F);
		CASE_BIN_L(ORLOG, ORLOG_F);
	   // CASE_BIN_L(XOR, XORLOG_F);


		case BytecodeVM::EQ:
			CMP_F(bres, t1.getValue<T>() , t2.getValue<T>());
			Vres.setValue(bres, ScriptVariant::T_bool);
			break;
		case BytecodeVM::NE:
			CMP_F(bres, t1.getValue<T>() , t2.getValue<T>());
			Vres.setValue(!bres, ScriptVariant::T_bool);
			break;
		CASE_BIN_L(LT, LT_F);
		CASE_BIN_L(GT, GT_F);
		CASE_BIN_L(LE, LE_F);
		CASE_BIN_L(GE, GE_F);

		CASE_BIN(ANDBIN, ANDBIN_F);
		CASE_BIN(ORBIN, ORBIN_F);
		CASE_BIN(XORBIN, XORBIN_F);
		CASE_BIN(SHL, SHL_F);
		CASE_BIN(SHR, SHR_F);

		default: Vres.setValue(res, ScriptVariant::T_AUTO); break;

	}

}

template <class T>
void makeUnaryOperation(BytecodeVM::UnOp op, ScriptVariant& t1)
{
	ScriptVariant tmp;
	switch(op)
	{
		case BytecodeVM::UPLUS:
			 tmp.setValue(+t1.getValue<T>(), ScriptVariant::T_AUTO);
			 t1 = tmp;
			 break;
		case BytecodeVM::UMINUS:
			 tmp.setValue(-t1.getValue<T>(), ScriptVariant::T_AUTO);
			 t1 = tmp;
			 break ;
		case BytecodeVM::UNOT:
			 tmp.setValue(!t1.getValue<T>(), ScriptVariant::T_AUTO);
			 t1 = tmp;
			 break ;

		case BytecodeVM::UINC:
			 t1.setValue(t1.getValue<T>() + 1);
			 break;
		case BytecodeVM::UDEC:
			 t1.setValue(t1.getValue<T>() - 1);
			 break;
		case BytecodeVM::UINV:{
			 T val;
			 INVERSE_F(val, t1.getValue<T>(), t1.getValue<T>());
			 t1.setValue(val);
			 } break;
		default: ; break;
	}

}

template <>
void makeUnaryOperation<bool>(BytecodeVM::UnOp op, ScriptVariant& t1)
{
	ScriptVariant ret;
	switch(op)
	{
		case BytecodeVM::UNOT:
		ret.setValue(! t1.getValue<bool>(), ScriptVariant::T_AUTO);
		t1 = ret;
		break;
		default: ; break;
	}

}

template <class T>
inline bool makeCmpOperation(ScriptVariant& t1, ScriptVariant& t2)
{
	bool res;
	CMP_F(res, t1.getValue<T>(), t2.getValue<T>());
	return res;
}

bool makeCmpOperationWrap(ScriptVariant& t1, ScriptVariant& t2)
{
	ScriptVariant::Types optype = ScriptVariant::Types(t1.getReferenced(0)->_Type);
	switch(optype){
		case ScriptVariant::T_bool:       return makeCmpOperation<bool       >(t1, t2); break;
		case ScriptVariant::T_float32:    return makeCmpOperation<float      >(t1, t2); break;
		case ScriptVariant::T_float64:    return makeCmpOperation<double     >(t1, t2); break;
		case ScriptVariant::T_int8_t:     return makeCmpOperation<int8_t     >(t1, t2); break;
		case ScriptVariant::T_uint8_t:    return makeCmpOperation<uint8_t    >(t1, t2); break;
		case ScriptVariant::T_int16_t:    return makeCmpOperation<int16_t    >(t1, t2); break;
		case ScriptVariant::T_uint16_t:   return makeCmpOperation<uint16_t   >(t1, t2); break;
		case ScriptVariant::T_int32_t:    return makeCmpOperation<int32_t    >(t1, t2); break;
		case ScriptVariant::T_uint32_t:   return makeCmpOperation<uint32_t   >(t1, t2); break;
		case ScriptVariant::T_int64_t:    return makeCmpOperation<int64_t    >(t1, t2); break;
		case ScriptVariant::T_uint64_t:   return makeCmpOperation<uint64_t   >(t1, t2); break;
		case ScriptVariant::T_string:     return makeCmpOperation<std::string>(t1, t2); break;
		default:
			return false;
	}

}

void ScriptVM::termOperation(BytecodeVM::BinOp op, ScriptVariant::Types optype, BytecodeVM::BINOP_flags /*flags*/)
{
	ScriptVariant& t1= sTop(1);
	ScriptVariant& t2= sTop(0);
	ScriptVariant res;
	switch(optype) {
	   case ScriptVariant::T_bool:        makeBinaryOperation<bool       >(op, res, t1, t2); break;
	   case ScriptVariant::T_float32:     makeBinaryOperation<float      >(op, res, t1, t2); break;
	   case ScriptVariant::T_float64:     makeBinaryOperation<double     >(op, res, t1, t2); break;
	   case ScriptVariant::T_int8_t:      makeBinaryOperation<int8_t     >(op, res, t1, t2); break;
	   case ScriptVariant::T_uint8_t:     makeBinaryOperation<uint8_t    >(op, res, t1, t2); break;
	   case ScriptVariant::T_int16_t:     makeBinaryOperation<int16_t    >(op, res, t1, t2); break;
	   case ScriptVariant::T_uint16_t:    makeBinaryOperation<uint16_t   >(op, res, t1, t2); break;
	   case ScriptVariant::T_int32_t:     makeBinaryOperation<int32_t    >(op, res, t1, t2); break;
	   case ScriptVariant::T_uint32_t:    makeBinaryOperation<uint32_t   >(op, res, t1, t2); break;
	   case ScriptVariant::T_int64_t:     makeBinaryOperation<int64_t    >(op, res, t1, t2); break;
	   case ScriptVariant::T_uint64_t:    makeBinaryOperation<uint64_t   >(op, res, t1, t2); break;
	   case ScriptVariant::T_string:      makeBinaryOperation<std::string>(op, res, t1, t2); break;
	   default: res._Type = ScriptVariant::T_UNDEFINED;
	}

	if (_debugFlags & dOperations)
		(*_debugout) << "BINOP t=" << optype << " " << t1.getString() << " " << BytecodeVM::binopStr[op] << " " << t2.getString() << " = " << res.getString() << "\n";
	sPops(2);
	sPush(res);

}



void ScriptVM::movs(BytecodeVM::MOVS_flags flags, int size)
{

	int baseOffset =0;
	const bool isLeftRef  = (flags & BytecodeVM::mLeftIsRef);
	const bool isRightRef = (flags & BytecodeVM::mRightIsRef);
	const bool isAddress  = (flags & BytecodeVM::mAddress);
	baseOffset +=isRightRef ? 1 : size;
	const int rightOffset = baseOffset;
	baseOffset +=isLeftRef  ? 1 : size;
	ScriptVariant &leftPointer  = sList(baseOffset, 0);
	ScriptVariant &rightPointer = sList(rightOffset, 0);
	std::string v;
	const bool debugOperations =(_debugFlags & dOperations) && _debugout ;
	for (int i=0; i<size;i++){
		ScriptVariant &leftVal  = isLeftRef ?  (* (leftPointer .getReferenced(i, 1))) : sList(baseOffset, i);
		ScriptVariant &rightVal = isRightRef ? (* (rightPointer.getReferenced(i, 1))) : sList(rightOffset, i);
		if (debugOperations){
			v = leftVal.getString(true);
		}

		if (isAddress) {
			leftVal.setOpValueAddress(rightVal);
		}else {
			leftVal.setOpValue(rightVal);
		}
		if (debugOperations){
			(*_debugout) << "MOVS " << v << " := " << rightVal.getString(true) << "\n";
		}

	}
	sPops(baseOffset);


}

void ScriptVM::cmps(BytecodeVM::CMPS_flags flags, int size)
{

	ScriptVariant res;
	int baseOffset =0;
	bool isLeftRef  = (flags & BytecodeVM::cLeftIsRef);
	bool isRightRef = (flags & BytecodeVM::cRightIsRef);
	baseOffset +=isRightRef ? 1 : size;
	int rightOffset = baseOffset;
	baseOffset +=isLeftRef  ? 1 : size;
	ScriptVariant &leftPointer  = sList(baseOffset, 0);
	ScriptVariant &rightPointer = sList(rightOffset, 0);

	bool t = true;
	for (int i=0; i<size;i++){
		ScriptVariant &leftVal  = isLeftRef ?  (* (leftPointer .getReferenced(i))) : sList(baseOffset, i);
		ScriptVariant &rightVal = isRightRef ? (* (rightPointer.getReferenced(i))) : sList(rightOffset, i);
		t = makeCmpOperationWrap(leftVal, rightVal);
		if (!t) break;
	}
	if (flags & BytecodeVM::cNot){
		t = !t;
	}
	res.setValue(t, ScriptVariant::T_bool);
	sPops(baseOffset);
	sPush(res);
}

void ScriptVM::unaryOperation(BytecodeVM::UnOp op, ScriptVariant::Types optype)
{
	ScriptVariant& t1= sTop(0);
	ScriptVariant dbg = t1;
	switch(optype){
		case ScriptVariant::T_bool:       makeUnaryOperation<bool       >(op, t1); break;
		case ScriptVariant::T_float32:    makeUnaryOperation<float      >(op, t1); break;
		case ScriptVariant::T_float64:    makeUnaryOperation<double     >(op, t1); break;
		case ScriptVariant::T_int8_t:     makeUnaryOperation<int8_t     >(op, t1); break;
		case ScriptVariant::T_uint8_t:    makeUnaryOperation<uint8_t    >(op, t1); break;
		case ScriptVariant::T_int16_t:    makeUnaryOperation<int16_t    >(op, t1); break;
		case ScriptVariant::T_uint16_t:   makeUnaryOperation<uint16_t   >(op, t1); break;
		case ScriptVariant::T_int32_t:    makeUnaryOperation<int32_t    >(op, t1); break;
		case ScriptVariant::T_uint32_t:   makeUnaryOperation<uint32_t   >(op, t1); break;
		case ScriptVariant::T_int64_t:    makeUnaryOperation<int64_t    >(op, t1); break;
		case ScriptVariant::T_uint64_t:   makeUnaryOperation<uint64_t   >(op, t1); break;
	   // case ScriptVariant::T_string:     makeUnaryOperation<std::string>(op, t1); break;
		default: t1._Type = ScriptVariant::T_UNDEFINED;
	}
	if (_debugFlags & dOperations)
		(*_debugout) << "UNNOP t=" << optype<< " " << BytecodeVM::unopStr[op] << " " << dbg.getValue<double>()  <<  " = " << t1.getValue<double>() << "\n";

}
#define IF_FUN_2(name, namef) \
	else if (op == BytecodeVM::name)\
		for (size_t i=1; i< args.size();i++){\
		   namef(r , r, args[i]->getValue<T>());\
		}
#define IF_FUN(name) IF_FUN_2(name, name##_F)


template <class T>
void multOper(BytecodeVM::BinOp op, std::vector<ScriptVariant*>& args, ScriptVariant& res)
{
	T r = args[0]->getValue<T>();
	if (op == BytecodeVM::PLUS)
		for (size_t i=1; i< args.size();i++){
		   PLUS_F(r, r, args[i]->getValue<T>());
		}
	IF_FUN(MUL)
	IF_FUN(ANDLOG)
	IF_FUN(ANDBIN)
	IF_FUN(ORLOG)
	IF_FUN(ORBIN)
	IF_FUN_2(XOR, XORLOG_F)
	IF_FUN(XORBIN)
	IF_FUN(MIN_)
	IF_FUN(MAX_)
	else if (op == BytecodeVM::MUX){
		size_t index =   args[0]->getValue<uint32_t>();
		if (index < args.size() - 1){
			r = args[index + 1]->getValue<T>();
		}
	}
	res.setValue(r, ScriptVariant::T_AUTO);
}


void ScriptVM::multOperation(BytecodeVM::BinOp op, ScriptVariant::Types optype, int count)
{
	if (!count) return;
	std::vector<ScriptVariant*> args;
	for (int i=0;i<count;i++){
		args.push_back(&(sList(count, i)));
	}
	ScriptVariant res;
	switch(optype){
		case ScriptVariant::T_bool:       multOper<bool       >(op, args, res); break;
		case ScriptVariant::T_float32:    multOper<float      >(op, args, res); break;
		case ScriptVariant::T_float64:    multOper<double     >(op, args, res); break;
		case ScriptVariant::T_int8_t:     multOper<int8_t     >(op, args, res); break;
		case ScriptVariant::T_uint8_t:    multOper<uint8_t    >(op, args, res); break;
		case ScriptVariant::T_int16_t:    multOper<int16_t    >(op, args, res); break;
		case ScriptVariant::T_uint16_t:   multOper<uint16_t   >(op, args, res); break;
		case ScriptVariant::T_int32_t:    multOper<int32_t    >(op, args, res); break;
		case ScriptVariant::T_uint32_t:   multOper<uint32_t   >(op, args, res); break;
		case ScriptVariant::T_int64_t:    multOper<int64_t    >(op, args, res); break;
		case ScriptVariant::T_uint64_t:   multOper<uint64_t   >(op, args, res); break;
		case ScriptVariant::T_string:     multOper<std::string>(op, args, res); break;
		default: res._Type = ScriptVariant::T_UNDEFINED;
	}
	//printStack();
	sPops(count);
	sPush(res);

	if (_debugFlags & dOperations)
		(*_debugout) << "MULTOP t=" << optype << " " <<  BytecodeVM::binopStr[op] << " [" << count << "] = " << res.getValue<double>() << "\n";

}

