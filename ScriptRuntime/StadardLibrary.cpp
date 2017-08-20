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

#include "StadardLibrary.h"

#include "ScriptVM.h"

#include <functional>
#include <chrono>

#undef M_PI
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

// sin(i:real):real;
void Sin(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::sin(args[0]->getValue<double>()) );
}
// cos(i:real):real;
void Cos(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::cos(args[0]->getValue<double>()) );
}
// sqr(i:real):real;
void Sqr(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	double i=args[0]->getValue<double>();
	result[0]->setValue( i*i);

}
// sqrt(i:real):real;
void Sqrt(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::sqrt(args[0]->getValue<double>()) );
}
// abs(i:real):real;
void Abs(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	double a = args[0]->getValue<double>();
	result[0]->setValue( a < 0 ? -a : a );
}
// tan(i:real):real;
void Tan(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
   result[0]->setValue( ::tan(args[0]->getValue<double>()) );
}
//EXPT(A,B) = A**B = POW
//XPY = A**B
// pow(i:real; j:real):real;
void Pow(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::pow(args[0]->getValue<double>(), args[1]->getValue<double>()) );
}

//ACS
// ACOS
void Acos(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::acos(args[0]->getValue<double>()) );
}
//ASN
//ASIN
void Asin(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::asin(args[0]->getValue<double>()) );
}
//ATN
//ATAN
void Atan(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::atan(args[0]->getValue<double>()) );
}
//ATAN2
void Atan2(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::atan2(args[0]->getValue<double>(), args[1]->getValue<double>() ) );
}
//DIV
void Div(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( args[0]->getValue<double>() / args[1]->getValue<double>() );
}
//EXP(A) = e**A
void Exp(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::exp(args[0]->getValue<double>()) );
}

//LN
void Ln(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::log(args[0]->getValue<double>()) );
}
//LIMIT(MN:=A,IN:=B,MX:=C); checks to see if B>=A and B<=C
//LOG
void Log(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue( ::log10(args[0]->getValue<double>()) );
}
//MOD
void Mod(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<uint64_t>() % args[1]->getValue<uint64_t>()  );
}
// ROL
void Rol(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<uint64_t>() << args[1]->getValue<uint64_t>()  );
}
// ROR
void Ror(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<uint64_t>() >> args[1]->getValue<uint64_t>()  );
}
// SEL(A,B,C)  A ==0 ? B :C
void Sel(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   !args[0]->getValue<double>() ? args[1]->getValue<double>()  : args[2]->getValue<double>() );
}
// SHL
void Shl(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<uint64_t>() << args[1]->getValue<uint64_t>()  );
}
// SHR
void Shr(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<uint64_t>() >> args[1]->getValue<uint64_t>()  );
}
// SUB
void Sub(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<double>() - args[1]->getValue<double>()  );
}
//TRUNC
void Trunc(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   (int) args[0]->getValue<double>()  );
}
//NEG
void Neg(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   - args[0]->getValue<double>()  );
}
//DEG(A) from radians to degrees
void Deg(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<double>() * 180 / M_PI  );
}
//RAD  to radians from degrees
void Rad(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<double>() * M_PI  / 180   );
}




//-----------------------------------------------------------------------------------------------------
//LEN
void Len(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	result[0]->setValue(   args[0]->getValue<std::string>().size()  );
}
//  MOVE
void Move(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	(*result[0]) = (*(args[0]->getReferenced()));
}
//  Limit(mn:real;in:real;mx:real):boolean  checks to see if in>=MN and in<=MX
void Limit(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	double mn = args[0]->getValue<double>();
	double in = args[1]->getValue<double>();
	double mx = args[2]->getValue<double>();
	 result[0]->setValue(   in >= mn && in <= mx  );
}
//-----------------------------------------------------------------------------------------------------
// "Now():Int64" - microseconds from UNIX epoch
void Now(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & )
{
	auto d = std::chrono::system_clock::now().time_since_epoch();
	result[0]->setValue( std::chrono::duration_cast<std::chrono::microseconds>(d).count() );
}
// "SecondsBetween(int64, int64):Double"
void SecondsBetween(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	 int64_t from   = args[0]->getValue<int64_t>();
	 int64_t to   = args[1]->getValue<int64_t>();
	 result[0]->setValue((to-from) / 1000000.0 );
}

// "ReadInt(lower:word;high:word;be:boolean):integer"
void ReadInt(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	uint16_t lower = args[0]->getValue<uint16_t>();
	uint16_t high = args[1]->getValue<uint16_t>();
	bool BE = args[2]->getValue<bool>();
	if (BE){
		lower = (lower % 256) << 8 | (lower / 256);
		high  = (high % 256) << 8  | (high / 256);
	}
	result[0]->setValue((high << 16) | lower);
}
// "ReadFloat(lower:word;high:word;be:boolean):float"
void ReadFloat(std::vector<ScriptVariant*> & result, std::vector<ScriptVariant*> & args)
{
	uint16_t lower = args[0]->getValue<uint16_t>();
	uint16_t high = args[1]->getValue<uint16_t>();
	bool BE = args[2]->getValue<bool>();
	if (BE){
		lower = (lower % 256) << 8 | (lower / 256);
		high  = (high % 256) << 8  | (high / 256);
	}
	 uint32_t t = (high << 16) | lower;
	  float t2=*((float*)&t);
	//  t2=1.5;

	result[0]->setValue(t2);
}

}

namespace SciptRuntimeLibrary
{
void bindAllStandard(ScriptVM *vm)
{
	if (!vm) return;
	using namespace std::placeholders;
	vm->bindFunction("sin", std::bind(&Sin, _1, _2));
	vm->bindFunction("cos", std::bind(&Cos, _1, _2));
	vm->bindFunction("sqr", std::bind(&Sqr, _1, _2));
	vm->bindFunction("sqrt",std::bind(&Sqrt, _1, _2));
	vm->bindFunction("abs", std::bind(&Abs, _1, _2));
	vm->bindFunction("pow", std::bind(&Pow, _1, _2));
	vm->bindFunction("expt", std::bind(&Pow, _1, _2));
	vm->bindFunction("xpy", std::bind(&Pow, _1, _2));
	vm->bindFunction("tan", std::bind(&Tan, _1, _2));

	vm->bindFunction("atan", std::bind(&Atan, _1, _2));
	vm->bindFunction("atn", std::bind(&Atan, _1, _2));
	vm->bindFunction("atan2", std::bind(&Atan2, _1, _2));
	vm->bindFunction("acos", std::bind(&Acos, _1, _2));
	vm->bindFunction("acs", std::bind(&Acos, _1, _2));
	vm->bindFunction("asin", std::bind(&Asin, _1, _2));
	vm->bindFunction("asn", std::bind(&Asin, _1, _2));


	vm->bindFunction("log", std::bind(&Log, _1, _2));
	vm->bindFunction("ln", std::bind(&Ln, _1, _2));
	vm->bindFunction("exp", std::bind(&Exp, _1, _2));

	vm->bindFunction("rol", std::bind(&Rol, _1, _2));
	vm->bindFunction("ror", std::bind(&Ror, _1, _2));
	vm->bindFunction("shl", std::bind(&Shl, _1, _2));
	vm->bindFunction("shr", std::bind(&Shr, _1, _2));

	vm->bindFunction("deg", std::bind(&Deg, _1, _2));
	vm->bindFunction("rad", std::bind(&Rad, _1, _2));

	vm->bindFunction("mod", std::bind(&Mod, _1, _2));
	vm->bindFunction("sub", std::bind(&Sub, _1, _2));
	vm->bindFunction("div", std::bind(&Div, _1, _2));
	vm->bindFunction("neg", std::bind(&Neg, _1, _2));

	vm->bindFunction("limit", std::bind(&Limit, _1, _2));
	vm->bindFunction("trunc", std::bind(&Trunc, _1, _2));
	vm->bindFunction("sel", std::bind(&Sel, _1, _2));
	vm->bindFunction("len", std::bind(&Len, _1, _2));
	vm->bindFunction("move", std::bind(&Move, _1, _2));

	// maths

	vm->bindFunction("now", std::bind(&Now, _1, _2));
	vm->bindFunction("secondsbetween", std::bind(&SecondsBetween, _1, _2));

	vm->bindFunction("readint", std::bind(&ReadInt, _1, _2));
	vm->bindFunction("readfloat", std::bind(&ReadFloat, _1, _2));
}

const std::vector<std::string> & allStandardProtoTypes()
{
	static const  std::vector<std::string> prototypes{
			  "Sin(a:real):real"
			, "Cos(a:real):real"
			, "Tan(a:real):real"
			, "Sqr(a:real):real"
			, "Sqrt(a:real):real"
			, "Abs(a:real):real"
			, "Pow(a:real;b:real):real"
			, "Expt(a:real;b:real):real"
			, "Xpy(a:real;b:real):real"

			, "Asin(a:real):real"
			, "Asn(a:real):real"
			, "Acos(a:real):real"
			, "Acs(a:real):real"
			, "Atan(a:real):real"
			, "Atn(a:real):real"
			, "Atan2(a:real):real"

			, "Ln(a:real):real"
			, "Log(a:real):real"
			, "Exp(a:real):real"

			, "Div(a:real;b:real):real"
			, "Mod(a:real;b:real):real"
			, "Sub(a:real;b:real):real"
			, "Neg(a:real):real"

			, "Rol(in:uint64;n:uint64):uint64"
			, "Ror(in:uint64;n:uint64):uint64"
			, "Shl(in:uint64;n:uint64):uint64"
			, "Shr(in:uint64;n:uint64):uint64"

			, "Deg(a:real):real"
			, "Rad(a:real):real"

			, "Limit(mn:real;in:real;mx:real):boolean"
			, "Trunc(a:real):real"
			, "Sel(a:real;b:real;c:real):real"
			, "Len(a:string):int"
			, "Move(a:real):real"

			, "Now():Int64"
			, "SecondsBetween(fromT:Int64;toT:int64):Double"

			, "ReadInt(lower:word;high:word;be:boolean):integer"
			, "ReadFloat(lower:word;high:word;be:boolean):float"
			   };
	return prototypes;
}

}
