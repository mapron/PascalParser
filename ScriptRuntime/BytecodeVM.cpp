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
#include "BytecodeVM.h"

#include <ByteOrderStream.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <stdexcept>
#include <algorithm>

std::string BytecodeVM::ConvertToString(bool useType) const
{
	std::ostringstream ret;
	ret <<(op >=0 ?opcodes[op]:std::string("?")) ;

	if ((op == BINOP || op == MULTOP || op == UNOP) ){
		int t0 = values[0].getValue<int>();
		int t1 = values[1].getValue<int>();
		int t2 = op == UNOP ? 0 : values[2].getValue<int>();

		ret << " " << ( op == UNOP ? unopStr[t0] : binopStr[t0]) << ", type:" << t1;

		if (op == BINOP) {
			ret << ", flags:" << t2;
		}
		else if (op == MULTOP) {
			ret << ", count:" << t2;
		}

	}else if (op == CALL || op == CALLEXT) {
		int t0 = values[0].getValue<int>();
		int t1 = values[1].getValue<int>();
		int t2 = values[2].getValue<int>();
		 ret << " ["<< t0<< "] (args:" << t1 << ") -> ret:" << t2;
		 if (op == CALL) {
			 ret << ", SL:" <<  values[3].getValue<int>();
		 }
	}else if (op == REF) {
		int t0 = values[0].getValue<int>();
		int t1 = values[1].getValue<int>();
		int s  = values[2].getValue<int>();
		ret << " ["<< t0<< "] SL:" << t1 ;
		if (s > 1){
			ret << " size " << s;
		}
	}else if (op == REFEXT || op == REFST) {
		int t0 = values[0].getValue<int>();
		ret << " ["<< t0<< "]";
	}else if (op == PUSH) {
		int t1 = values[1].getValue<int>();
		ret << " (" << values[0].getString(useType) << ")";
		if (t1 > 1){
			ret << " *" << t1;
		}
	}
	else if (op == WRT) {
		int sz = values[0].getValue<int>();
		int endline = values[1].getValue<int>();
		ret << " size:" << sz;
		if (endline)
			ret << " \\n";
	}
	else if (op == MOVS) {
		int flags = values[0].getValue<int>();
		int size = values[1].getValue<int>();
		ret << ", ";
		if (flags & mLeftIsRef){
			ret << "&";
		}
		ret << "L";
		if (flags & mAddress){
			ret <<  " @= ";
		}else{
			ret <<  " := ";
		}

		if (flags & mRightIsRef){
			ret << "&";
		}
		ret << "R";
		if (size > 1)
			ret << ", size:" << size;
	}
	else if (op == JMP || op == FJMP || op == TJMP) {
		int offset = values[0].getValue<int>();
		if (offset > 0) ret << "+";
		ret << offset;
	}
	else if (op == ADDREF) {
		int offset = values[0].getValue<int>();
		ret << " [";
		if (offset > 0) ret << "+";
		ret << offset << "]";
	}
	else if (op == DEREF) {
		int size = values[0].getValue<int>();
		ret << " size: " << size;
	}
	else if (op == IDX) {
		int size = values[0].getValue<int>();
		int offset = values[1].getValue<int>();

		ret << " [" << size << "]";
		if (offset)
			ret << ", low:" << offset;
	}
	if (useType){
		if (gotoLabel.size())
			ret << " GOTO " <<  gotoLabel << ";";
		if (symbolLabel.size())
			ret << " @<- " << symbolLabel << "; ";
	}
	return ret.str();
}

ByteOrderDataStreamWriter &operator <<(ByteOrderDataStreamWriter &of, const BytecodeVM& opc)
{
	of << char(opc.op);
	of << uint8_t(opc.values.size());
	for (size_t i =0; i< opc.values.size();i++){
		 of << opc.values[i];
	}

	return of;
}


ByteOrderDataStreamReader &operator >>(ByteOrderDataStreamReader &ifs, BytecodeVM &opc)
{
	char c;
	ifs >> c;
	opc.op = BytecodeVM::OpCodeType(c);
	uint8_t size=0;
	ifs >> size;
	opc.values.resize(size);
	for (size_t i=0; i < opc.values.size();i++) {
		 ifs >> opc.values[i];
	}


	return ifs;
}
const std::string BytecodeVM::opcodes[BytecodeVM::OPCODE_COUNT] = {
	"NOP   ",
	"BINOP ",
	"UNOP  ",
	"MULTO ",
	"MOVS  ",
	"CMPS  ",

	"ADDRF ",
	"IDX   ",
	"REF   ",
	"REFEX ",
	"REFST ",
	"DEREF ",
	"POP   ",
	"PUSH  ",
	"CALL  ",
	"CALLX ",
	"RET   ",
	"JMP   ",
	"FJMP  ",
	"TJMP  ",
	"CVRT  ",

	"WRITE ",
	"EXIT  ",

	"IDX_ST"
};


const std::string BytecodeVM::binopStr[BytecodeVM::BinOp_COUNT] = {

	"+"
	,"-"
	,"%"
	,"*"
	,"/"
	,"//"

	,"&"
	,"<<"
	,">>"
	,"|"
	,"^"

	,"&&"
	,"&&"
	,"||"
	,"||"
	,"^^"
	,"<"
	,">"
	,"<="
	,">="
	,"=="
	,"!="

	,"in"
	,"min"
	,"max"
	,"mux"
};

const std::string BytecodeVM::unopStr[BytecodeVM::UnOp_COUNT] = {
	"+",
	"-",
	"!",
	"~",
	"++",
	"--"
};
