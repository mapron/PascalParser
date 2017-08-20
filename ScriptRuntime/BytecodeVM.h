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

#include "ScriptVariant.h"

#include <string>
#include <vector>

/**
 * \brief VirtualMachine opcodes, used by SciptVM.
 */
struct BytecodeVM
{

	enum MOVS_flags { mNo=0, mLeftIsRef = 0x01, mRightIsRef = 0x02, mAddress = 0x04 };
	enum CMPS_flags { cNo=0, cLeftIsRef = 0x01, cRightIsRef = 0x02, cNot = 0x04 };
	enum BINOP_flags {bNo=0 };
	enum ADDREF_flags {aNo=0, aDestructive= 0x01 };
	enum OpCodeType{
		NOP,
		BINOP,  // +/*/  :=  [operation, type, flags] stack(-2 +1)
		UNOP,   // *-1      [operation, type] stack(-1 +1)
		MULTOP, // min/max/and [operation, type, count]
		MOVS,   // [flags, size] stack(-size*2... -2  +0);
		CMPS,   // [flags, size, type] stack(-size*2... -2  +1);

		ADDREF, // [offset] take reference from TOP, shift address to +offset and put to TOP
		IDX,    // [size, lowoffset] stack(-2 +1) take index from TOP, take reference from TOP+1, shift reference to (size-lowoffset)*offset and put to TOP.
		REF  ,  // [n, stackframe, size], put to TOP reference to address[n]. stackframe - number of stack level; size is max addressable size of reference.
		REFEXT, // [n], put to TOP external refence with address [n].
		REFST,  // [size], put reference to stack TOP value, of size [size].
		DEREF,  // [size] take reference from TOP and replace it with its value.
		POP,    // [size] remove from TOP [size] values.
		PUSH ,  // [value, N] push to TOP [n] values.
		CALL,   // [address, argsSize, returnSize, stackLevel]
		CALLEXT,// [address, argsSize, returnSize]
		RET,    //  return to callee. PC taken from TOP.
		JMP ,   // [+-address], PC += address.
		FJMP,   // [+-address] stack(-1), PC += address if TOP == 0
		TJMP ,  // [+-address] stack(-1), PC += address if TOP != 0
		CVRT ,  // [type], convert TOP to type(ScriptVariant::Types).

		WRT,    // [size, endLine] print TOP value to standard output.
		EXIT ,  // Terminate execution.

		IDX_STR,    // [] stack(-2 +1) index string, put char to TOP.
		OPCODE_COUNT
	};
	static const std::string opcodes[OPCODE_COUNT];

	enum UnOp {
		 UPLUS
		,UMINUS
		,UNOT
		,UINV
		,UINC
		,UDEC

		,UnOp_COUNT
	};

	enum BinOp {
		 PLUS
		,MINUS
		,MOD
		,MUL
		,DIV
		,DIVR

		,ANDBIN
		,SHL
		,SHR
		,ORBIN
		,XORBIN

		,AND
		,ANDLOG
		,OR
		,ORLOG
		,XOR
		,LT
		,GT
		,LE
		,GE
		,EQ
		,NE

		,IN_
		,MIN_
		,MAX_
		,MUX

		,BinOp_COUNT
	};

	 static const std::string binopStr[BinOp_COUNT];
	 static const std::string unopStr[UnOp_COUNT];

	OpCodeType op;
	std::vector<ScriptVariant> values;
	int col, line, file;
	void *scope;
	std::string symbolLabel;
	std::string gotoLabel;
	BytecodeVM() : op(NOP),col(-1), line(-1), file(-1),scope(0) {}
	std::string ConvertToString(bool useType = true) const;

};

ByteOrderDataStreamWriter&  operator <<(ByteOrderDataStreamWriter&  of,const BytecodeVM& opc);
ByteOrderDataStreamReader&  operator >>(ByteOrderDataStreamReader&  ifs,BytecodeVM& opc);
