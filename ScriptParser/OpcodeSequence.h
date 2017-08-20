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

#include <BytecodeVM.h>
class BlockScope;
class OpcodeSequence : public std::vector<BytecodeVM>
{
	int file, col, line;
	BlockScope* _blockScope;
	BytecodeVM cur_op() {
		BytecodeVM opc;
		opc.file = file;
		opc.col = col;
		opc.line = line;
		opc.scope = _blockScope;
		return opc;
	}

public:
	OpcodeSequence();
	void setLoc(int f, int l, int c);
	void setScope(BlockScope* blockScope);
	template <class T>
	void setLocVal(const T& val) {
		return setLoc(val._loc._file, val._loc._line, val._loc._col);
	}
	template <class T >
	BytecodeVM& Emit(BytecodeVM::OpCodeType op, T val){
		BytecodeVM opc =  cur_op();
		opc.op = op;
		opc.values.resize(1);
		opc.values[0].setValue(val, ScriptVariant::T_AUTO);
		this->push_back(opc);
		return (*this)[size()-1];
	}

	template <class T, class T2 >
	BytecodeVM& Emit(BytecodeVM::OpCodeType op, T val, T2 val2){
		BytecodeVM opc =  cur_op();
		opc.op = op;
		opc.values.resize(2);
		opc.values[0].setValue(val, ScriptVariant::T_AUTO);
		opc.values[1].setValue(val2, ScriptVariant::T_AUTO);
		this->push_back(opc);
		return (*this)[size()-1];
	}
	template <class T, class T2, class T3 >
	BytecodeVM& Emit(BytecodeVM::OpCodeType op, T val, T2 val2, T3 val3){
		BytecodeVM opc =  cur_op();
		opc.op = op;
		opc.values.resize(3);
		opc.values[0].setValue(val, ScriptVariant::T_AUTO);
		opc.values[1].setValue(val2, ScriptVariant::T_AUTO);
		opc.values[2].setValue(val3, ScriptVariant::T_AUTO);
		this->push_back(opc);
		return (*this)[size()-1];
	}

	BytecodeVM& Emit(BytecodeVM::OpCodeType op);
	BytecodeVM& Prepend(BytecodeVM::OpCodeType op, int size);

	BytecodeVM& EmitInit(BytecodeVM::OpCodeType op, ScriptVariant::Types t);
	void EmitAddref(int size);

	template <class T>
	BytecodeVM& EmitPush(T val, int size = 1){
		BytecodeVM opc =  cur_op();
		opc.op = BytecodeVM::PUSH;
		opc.values.resize(2);
		opc.values[0].setValue(val, ScriptVariant::T_AUTO);
		opc.values[1].setValue(size, ScriptVariant::T_AUTO);
		this->push_back(opc);
		return (*this)[this->size()-1];
	}


	BytecodeVM& EmitPush(ScriptVariant::Types val, int size = 1);
	BytecodeVM& EmitPush(const ScriptVariant& val);
	void ReplaceBreak(int jmpOffset);
	void ReplaceContinue(int jmpOffset);
	OpcodeSequence& operator << (const OpcodeSequence& another);
	OpcodeSequence& operator << (const BytecodeVM& opc);

	void optimize();
};
