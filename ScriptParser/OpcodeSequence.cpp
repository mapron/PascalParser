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

#include "OpcodeSequence.h"

OpcodeSequence::OpcodeSequence()
{
	setLoc(-1,-1,-1);
	_blockScope = nullptr;
}

void OpcodeSequence::setLoc(int f, int l, int c)
{
	file = f;
	col = c;
	line = l;
}

void OpcodeSequence::setScope(BlockScope *blockScope)
{
	_blockScope =blockScope;
}

BytecodeVM &OpcodeSequence::Emit( BytecodeVM::OpCodeType op){
	BytecodeVM opc =  cur_op();
	opc.op = op;
	this->push_back(opc);
	return (*this)[size()-1];
}

BytecodeVM &OpcodeSequence::Prepend(BytecodeVM::OpCodeType op, int size)
{
	BytecodeVM opc =  cur_op();
	opc.op = op;
	opc.values.resize(1);
	opc.values[0].setValue(size, ScriptVariant::T_AUTO);
	this->insert(begin(), opc);
	return (*this)[0];
}

BytecodeVM &OpcodeSequence::EmitInit(BytecodeVM::OpCodeType op, ScriptVariant::Types t){
	BytecodeVM opc =  cur_op();
	opc.op = op;
	opc.values.resize(2);
	opc.values[0].setValue(1, ScriptVariant::T_int32_t);
	opc.values[1].setValue(0, t);
	this->push_back(opc);
	return (*this)[size()-1];
}

void OpcodeSequence::EmitAddref(int size)
{
	if (size < 0) return;
	if (this->size() == 0) this->Emit(BytecodeVM::ADDREF, size);
	BytecodeVM &last = (*this)[this->size()-1];
	if (last.op == BytecodeVM::ADDREF) {
		last.values[0].setValue(size + last.values[0].getValue<int>());
	}else{
		 this->Emit(BytecodeVM::ADDREF, size);
	}
}

BytecodeVM &OpcodeSequence::EmitPush(ScriptVariant::Types val, int size){
	BytecodeVM opc =  cur_op();
	opc.op = BytecodeVM::PUSH;
	opc.values.resize(2);
	opc.values[0].setValue(0, val);
	opc.values[1].setValue(size, ScriptVariant::T_AUTO);
	this->push_back(opc);
	return (*this)[this->size()-1];
}

BytecodeVM &OpcodeSequence::EmitPush(const ScriptVariant &val)
{
	BytecodeVM opc =  cur_op();
	opc.op = BytecodeVM::PUSH;
	opc.values.resize(2);
	opc.values[0] = val;
	opc.values[1].setValue(int (1), ScriptVariant::T_AUTO);
	this->push_back(opc);
	return (*this)[size()-1];
}

void OpcodeSequence::ReplaceBreak(int jmpOffset)
{
	for (size_t i=0; i < this->size();i++)
	{
		BytecodeVM &opc=(*this)[i];
		if (opc.symbolLabel == "__break")
		{
			opc.symbolLabel.clear();
			opc.values.resize(1);
			opc.values[0].setValue(jmpOffset-i, ScriptVariant::T_int32_t);
		}
	}
}

void OpcodeSequence::ReplaceContinue(int jmpOffset)
{
	for (size_t i=0; i < this->size();i++)
	{
		BytecodeVM &opc=(*this)[i];
		if (opc.symbolLabel == "__continue")
		{
			opc.symbolLabel.clear();
			opc.values.resize(1);
			opc.values[0].setValue(jmpOffset-i, ScriptVariant::T_int32_t);
		}
	}
}

OpcodeSequence &OpcodeSequence::operator <<(const OpcodeSequence &another)
{
	this->insert(this->end(), another.begin(), another.end());
	return *this;
}

OpcodeSequence &OpcodeSequence::operator <<(const BytecodeVM &opc)
{
	this->push_back(opc);
	return *this;
}

void OpcodeSequence::optimize()
{
	// TODO:?
}
