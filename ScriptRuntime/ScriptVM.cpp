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

#include "ScriptVM.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <string>

const int ScriptVM::_formatVersion = 1;

ScriptVM::ScriptVM()
{
	_runState = rsFinished;
	_errout =nullptr;
	_stdout =nullptr;
	_debugout =nullptr;
	_stackSize = 0;
	_startPC   = 0;
	_debugFlags = 0;
	_totalOPC = 0;
	_isRunnable = false;
	_doExit = false;
	_useBreakPoints = false;
	_useCurrentLine = false;
	_useSkipCalls = false;
	_stepLimit = -1;
	clear();
}

ScriptVM::~ScriptVM()
{
}

void ScriptVM::clear()
{
	_nameTable.clear();
	_funcTable.clear();
	_code.clear();
	initialState();
	_runState = rsFinished;
}

// --------------------------- Public API -------------------------------------

bool ScriptVM::bindFunction(std::string index, FuncNameRecord::funCallback func)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	for (size_t i=0;i< _funcTable.size();i++)
	{
		if (_funcTable[i]._resolved)
			continue;
		if (_funcTable[i]._name == index)
		{
			_funcTable[i]._resolved = true;
			_funcTable[i]._callback = func;
			return true;
		}
	}
	return false;
}

bool ScriptVM::bindFunction(std::string index, FuncNameRecordInterface *func)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	for (size_t i=0;i< _funcTable.size();i++)
	{
		if (_funcTable[i]._resolved)
			continue;
		if (_funcTable[i]._name == index)
		{
			_funcTable[i]._resolved = true;
			_funcTable[i]._callback2 = func;
			return true;
		}
	}
	return false;
}

bool ScriptVM::bindVariable(std::string index, ScriptVariant::AddressPtr p, bool forceRebind)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	for (size_t i=0;i< _nameTable.size();i++)
	{
		if (_nameTable[i]._resolved && !forceRebind)
			continue;
		if (_nameTable[i]._name == index)
		{
			_nameTable[i]._resolved = true;
			_nameTable[i]._ptr = p;
			return true;
		}
	}
	return false;
}

bool ScriptVM::bindVariable(std::string index, std::vector<ScriptVariant *> &container, int indexInContainer, int size, bool forceRebind)
{
	ScriptVariant::AddressPtr p;
	p.container = nullptr;
	p.container2 = &container;
	p.index = indexInContainer;
	if (size == -1)
		size = container.size();
	p.maxIndex = p.index + size - 1;
	return bindVariable(index, p, forceRebind);
}

bool ScriptVM::bindVariable(std::string index, std::vector<ScriptVariant> &container, int indexInContainer, int size, bool forceRebind)
{
	ScriptVariant::AddressPtr p;
	p.container = &container;
	p.container2 = nullptr;
	p.index = indexInContainer;
	if (size == -1)
		size =  container.size();
	p.maxIndex = p.index + size - 1;
	return bindVariable(index, p, forceRebind);
}

void ScriptVM::doAutoBindVars(std::vector<ScriptVariant> &container)
{
	int currentIndex = 0;
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags != NameRecord::bdNone)
		{
			ScriptVariant::AddressPtr p;
			p.container = &container;
			p.container2 = nullptr;
			p.index = currentIndex;
			p.maxIndex = p.index + nr._sizeBytes - 1;
			currentIndex += nr._sizeBytes;
			nr._ptr = p;
			nr._resolved = true;
		}
	}
	container.resize(currentIndex);
}

bool ScriptVM::checkExternalReferences()
{
	bool result = true;
	for (size_t i=0;i< _funcTable.size();i++)
	{
		if (!_funcTable[i]._resolved)
		{
			if (_errout) (*_errout) << "Unresolved external symbol \"" << _funcTable[i]._name << "\"" <<std::endl;
			result = false;
		}
	}
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags == NameRecord::bdNone)
		{
			nr._resolved = true;
		}
		else
		{
			int bound_size = nr._ptr.maxIndex - nr._ptr.index + 1;
			if (nr._ptr.container == 0 && nr._ptr.container2 == 0) bound_size = 0;
			int expected_size = nr._sizeBytes;
			if (expected_size != bound_size)
			{
				  if (_errout) (*_errout) << "Symbol \"" << nr._name << "\" expected size " << expected_size
										  << ", but bound " << bound_size <<std::endl;
				  result = false;
			}

		}
		if (!nr._resolved)
		{
			if (_errout) (*_errout) << "Unresolved external symbol \"" << nr._name << "\"" <<std::endl;
			result = false;
		}
	}
	return result;
}

bool ScriptVM::initStatic()
{
	_staticVars.clear();
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags == NameRecord::bdNone)
		{
			int varSize = nr._staticValues.size();
			if (varSize)
			{
				nr._resolved = true;
				ScriptVariant::AddressPtr ptr;
				ptr.container = &_staticVars;
				ptr.container2 = nullptr;
				ptr.index=_staticVars.size();
				ptr.maxIndex = ptr.index + varSize - 1;
				for (int j=0;j<varSize;j++)
					_staticVars.push_back( nr._staticValues[j]);

				nr._ptr = ptr;
			}
		}
	}
	return true;
}

bool ScriptVM::hasFunction(std::string index)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	for (size_t i=0;i< _funcTable.size();i++)
	{
		FuncNameRecord & nr = _funcTable[i];
		if (nr._name == index)
			return true;
	}
	return false;
}

void ScriptVM::initialState()
{
	_pc = _startPC;
	_stackFrames.resize(1);
	_stackFrames[0] = CallStackFrame(0,0, _code.size(), 0, 0);
	_opCnt = 0;
	sClear();
	_runState = rsRunning;
}

void ScriptVM::run()
{
	if (_runState != rsRunning)
		initialState();

	size_t callLevelStart = _stackFrames.size();

	ExecutionStatus status = Success;
	try { //  DEREF can throw cyclic ref exception.

		while (status == Success)
		{

			status = executeOneCommand();
			_opCnt++;
			if (status == Error)
				break; // eof is Error.
			if (_stepLimit > -1 && _opCnt > _stepLimit)
				break;
			if (_useBreakPoints && _breakPointPC.find(_pc) != _breakPointPC.end())
				break;
			if (_useCurrentLine && _currentLinePC.find(_pc) == _currentLinePC.end())
			{
				size_t callLevelEnd = _stackFrames.size();
				if (_useSkipCalls && callLevelEnd > callLevelStart)
					continue;

				break;
			}

		}
	} catch(std::exception& e) {
		runtimeError(e.what());
		status = Error;
	}
	if (status == Error)
		_runState = rsFinished;
}


int ScriptVM::addVariable(std::string index, int size, NameRecord::BindDirection bd)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	NameRecord r;
	r._name = index;
	r._sizeBytes = size;
	r._flags = bd;
	this->_nameTable.push_back(r);
	return this->_nameTable.size()-1;
}

int ScriptVM::addStaticVariable(std::string index, const std::vector<ScriptVariant> &values)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	NameRecord r;
	r._name = index;
	r._staticValues = values;
	this->_nameTable.push_back(r);
	return this->_nameTable.size()-1;
}

int ScriptVM::addFunction(std::string index)
{
	std::transform(index.begin(), index.end(), index.begin(), ::tolower);
	FuncNameRecord r;
	r._name = index;
	this->_funcTable.push_back(r);
	return this->_funcTable.size()-1;
}

//------------------ BC runtime implementation ---------------

ScriptVM::ExecutionStatus ScriptVM::executeOneCommand()
{
	int opcode_n = -1;

	if(_pc < _code.size() && _code[_pc].op != BytecodeVM::EXIT && !_doExit)
	{
		const BytecodeVM &o = _code[_pc];
		if (_debugout && (_debugFlags & dOpcode))
			(*_debugout)<< "[" << std::setfill (' ') << std::setw(3) << _pc  << std::setw(3) << "]: "<<o.ConvertToString(false)<<"\n";

		bool incPC = true;
		int opcValue =0;
		int opcValue2 =0;
		if (o.values.size() > 0) opcValue  = o.values[0].getValue<int>();
		if (o.values.size() > 1) opcValue2 = o.values[1].getValue<int>();
		opcode_n = o.op;
		switch(opcode_n)
		{
		case BytecodeVM::BINOP:
			termOperation(BytecodeVM::BinOp(opcValue),  ScriptVariant::Types(opcValue2), BytecodeVM::BINOP_flags(o.values[2].getValue<int>()));
			break;
		case BytecodeVM::MOVS:
			movs(BytecodeVM::MOVS_flags(opcValue), opcValue2);
			break;
		case BytecodeVM::CMPS:
			cmps(BytecodeVM::CMPS_flags(opcValue), opcValue2);
			break;
		case BytecodeVM::UNOP:
			unaryOperation(BytecodeVM::UnOp(opcValue), ScriptVariant::Types(opcValue2));
			break;
		case BytecodeVM::MULTOP:
			multOperation(BytecodeVM::BinOp(opcValue), ScriptVariant::Types(opcValue2), o.values[2].getValue<int>());
			break;
		case BytecodeVM::REF:{
			ScriptVariant r;
			int address = 0;
			int scopeLevel = opcValue2;
			int size  = o.values[2].getValue<int>();
			bool autoDeref = o.values.size() > 3 ? o.values[3].getValue<bool>() : true;
			for(int i = _stackFrames.size() - 1; i>=0; i--)
			{
				if (_stackFrames[i].scopeLevel == scopeLevel || i == 0)
				{
					address =  _stackFrames[i].bottomAddress;
					break;
				}
			}
			address+= opcValue;
			if ( address >= sSize())
			{
				runtimeError(std::string("Trying to reference address beyond stack size."));
				return Error;
			}
			else
			{
				r.setPointer(_stack, address, size,  autoDeref);
			}
			sPush(r);
		} break;

		case BytecodeVM::REFEXT:{
			ScriptVariant r;
			r.setPointerDbg(_nameTable[opcValue]._ptr);
			sPush(r);
		} break;

		case BytecodeVM::DEREF:{
			ScriptVariant r = *(sTop().getReferenced(0, 1));
			sTop() = r;
		} break;

		case BytecodeVM::PUSH:
			sPush(o.values[0], opcValue2);
			break;

		case BytecodeVM::CALL:{

			int argSize =  opcValue2 ;
			int retSize = o.values[2].getValue<int>();
			int stackLevel = o.values[3].getValue<int>();
			int bottomAddress = sSize() - argSize - retSize;

			_stackFrames.push_back(CallStackFrame(retSize, argSize, _pc + 1,bottomAddress, stackLevel)  );

			_pc = opcValue;

			incPC = false;
			}break;
		case BytecodeVM::CALLEXT:{
			int index=opcValue;
			int argSize = opcValue2;
			int retSize = o.values[2].getValue<int>();
			std::vector<ScriptVariant*>  results(retSize);
			std::vector<ScriptVariant*>  args(argSize);

			for(int i = 0; i < retSize; i++) {
				results[i]= (&(sList(argSize + retSize, i)));
			}
			for(int i = 0; i < argSize; i++) {
				args[i]= (&(sList(argSize, i)));
			}

			if (_funcTable[index]._callback)
				_funcTable[index]._callback(results, args);
			else if (_funcTable[index]._callback2)
				_funcTable[index]._callback2->call(results, args);
			else
				runtimeError("unresolved call!");

			sPops(argSize);
		}break;
		case BytecodeVM::RET:{

			incPC = false;
			CallStackFrame &cur = _stackFrames[_stackFrames.size() - 1];

			_pc = cur.returnAddress;
			if (_stackFrames.size() > 1)
			{
				_stackSize = cur.bottomAddress + cur.resultSize;
				_stackFrames.pop_back();
			}

		} break;
		case BytecodeVM::JMP:
			_pc += opcValue;
			incPC = false;
			break;
		case BytecodeVM::FJMP:
			if (!sTop().getValue<bool>())
			{
				_pc += opcValue;
				incPC = false;
			}
			sPops();
			break;
		case BytecodeVM::TJMP:
			if (sTop().getValue<bool>())
			{
				_pc += opcValue;
				incPC = false;
			}
			sPops();
			break;
		case BytecodeVM::ADDREF:{
			sTop(0).addPointer( opcValue );
		   } break;
		case BytecodeVM::IDX:{
			int offset = (sTopValue(0) -  opcValue2 ) * opcValue;

			sTop(1).addPointer( offset );
			sPops();
		}break;

		case BytecodeVM::IDX_STR:{
			int offset = sTopValue(0);

			ScriptVariant *r = sTop(1).getReferenced();
			sTop(1).setStringReference(*r, offset);
			sPops();
		}break;

		case BytecodeVM::POP:
			sPops(opcValue);
			break;

		case BytecodeVM::CVRT:
			sTop(0).setType( ScriptVariant::Types(opcValue) );
			break;

		case BytecodeVM::WRT:{
			int size = opcValue;
			bool endline = o.values[1].getValue<bool>();
			if (_stdout)
			{
				if (size > 1) (*_stdout) << "( ";
				ScriptVariant &o = sTop();
				for (int i=0; i<size;i++){
					 (*_stdout) << o.getReferenced(i)->getString(false) << " ";
				}
				sPops();
				if (size > 1) (*_stdout) << ")";
				if (endline)
					(*_stdout)<<std::endl;
			}

		}break;

		default:{
			std::ostringstream os; os<<"unknown opcode " << o.op;
			runtimeError(os.str());
			return Error;
		}
		}
		if (incPC)
			_pc++;

		if (_debugout && (_debugFlags & dStack)){
			printStack();
		}
		if (_debugout && (_debugFlags & dStaticVars)){
			printStatic();
		}
		if (_debugout && (_debugFlags & dExternalVars)){
			printExternal();
		}
		if (_debugout && (_debugFlags & dCallStack)){
			printCallStack();
		}
	}

	if (!(_pc < _code.size() && _code[_pc].op != BytecodeVM::EXIT && !_doExit   ))
		return Error;

	return Success;
}


// ------------------- Debug functions -----------

void ScriptVM::printOpcodes()
{
	if (!_debugout)return;
	size_t size = _code.size();
	for(size_t i = 0; i<size; i++)
	{
		(*_debugout)<< std::setfill (' ') << std::setw(3) <<  i  << std::setw(1) << ": " << _code[i].ConvertToString() << std::endl;
	}
}

void ScriptVM::printStack()
{
	printOpValueVector("   --- stack  ---",  StackFrameBottom(), sSize(),  _stack);

}

void ScriptVM::printStatic()
{
	printOpValueVector("   --- static  ---",  0, _staticVars.size(),  _staticVars);
}

void ScriptVM::printExternal()
{
	if (!_debugout) return;
	(*_debugout)<< "   --- external  ---"<<std::endl;
	for (size_t i=0; i < _nameTable.size(); i++) {
		NameRecord& nr = _nameTable[i];
		if (!nr._resolved) continue;

		ScriptVariant::AddressPtr &ptr = nr._ptr;
		for (size_t a = ptr.index; a <= ptr.maxIndex; a++) {
			 (*_debugout)<<"   " << nr._name << "[" << a << "]=" << ptr.get(a - ptr.index)->getString(true) <<std::endl;
		}
	}
}

void ScriptVM::printOpValueVector(const std::string &title, int bottom, int size, std::vector<ScriptVariant> &array)
{
	if (!_debugout) return;
	(*_debugout)<<title<<std::endl;

	for(int i = 0; i < size; i++)
	{
		(*_debugout)<<"   ";

		if(i == sSize()-1)
			(*_debugout)<<">";
		else if(i == bottom)
			(*_debugout)<<"*";
		else if(i < bottom)
			(*_debugout)<<" ";
		else
			(*_debugout)<<"|";

		(*_debugout)<< i <<": " << array[i].getString(true);
		(*_debugout)<<std::endl;
	}
}

void ScriptVM::printCallStack()
{
	if (!_debugout) return;
	(*_debugout)<<"   --- calls  ---"<<std::endl;
	for (size_t i=0;i<_stackFrames.size();i++){
		CallStackFrame& f = _stackFrames[i];
		(*_debugout)<<" [" << f.bottomAddress << "] :  ret=" << f.returnAddress
				 << " scopeLevel=" << f.scopeLevel
				 <<std::endl;
	}

}

// ------------------- IO -----------------

ByteOrderDataStreamWriter &operator <<(ByteOrderDataStreamWriter &of, const ScriptVM &opc)
{
	of << ScriptVM::_formatVersion

	   << opc._startPC;

	uint32_t size = opc._code.size();
	of << size;
	for(uint32_t i = 0; i<size; i++)
	{
		of<<opc._code[i];
	}
	size = opc._nameTable.size();
	of << size;
	for(uint32_t i = 0; i<size; i++)
	{
		of<<opc._nameTable[i];
	}

	size = opc._funcTable.size();
	of << size;
	for(uint32_t i = 0; i<size; i++)
	{
		of<<opc._funcTable[i];
	}
	return of;
}
ByteOrderDataStreamReader &operator >>(ByteOrderDataStreamReader &ifs, ScriptVM &opc)
{
	int version;
	ifs >> version;
	if (version != ScriptVM::_formatVersion)
		throw std::runtime_error("format version differs.");

	ifs  >> opc._startPC;
	uint32_t size=0;
	ifs >> size;
	opc._code.resize(size);
	for(uint32_t i = 0; i<size; i++)
	{
		ifs>>opc._code[i];
	}
	 ifs >> size;
	opc._nameTable.resize(size);
	for(uint32_t i = 0; i<size; i++)
	{
		ifs>>opc._nameTable[i];
	}
	ifs >> size;
	opc._funcTable.resize(size);
	for(uint32_t i = 0; i<size; i++)
	{
		ifs>>opc._funcTable[i];
	}
	return ifs;
}


ByteOrderDataStreamWriter &operator <<(ByteOrderDataStreamWriter &of, const ScriptVM::NameRecord &opc)
{
	uint32_t flags = opc._flags;
	uint32_t staticsize = opc._staticValues.size();

	of << flags << staticsize;
	for (uint32_t i=0;i<staticsize;i++)
		of << opc._staticValues[i];

	of.WritePascalString( opc._name );
	of << opc._sizeBytes;
	return of;
}


ByteOrderDataStreamReader &operator >>(ByteOrderDataStreamReader &ifs, ScriptVM::NameRecord &opc)
{
	uint32_t flags;
	uint32_t staticsize;
	ifs >> flags >> staticsize;
	opc._flags =  ScriptVM::NameRecord::BindDirection(flags);
	opc._staticValues.resize(staticsize);
	for (uint32_t i=0;i<staticsize;i++)
		ifs >> opc._staticValues[i];

	ifs.ReadPascalString(opc._name);
	ifs >> opc._sizeBytes;
	return ifs;
}

ByteOrderDataStreamWriter &operator <<(ByteOrderDataStreamWriter &of, const ScriptVM::FuncNameRecord &opc)
{
	of.WritePascalString( opc._name );
	return of;
}


ByteOrderDataStreamReader &operator >>(ByteOrderDataStreamReader &ifs, ScriptVM::FuncNameRecord &opc)
{
	ifs.ReadPascalString(opc._name);
	return ifs;
}

bool ScriptVM::importFromHexString(const std::string &str)
{
	ByteOrderBuffer buf;
	ByteOrderDataStreamWriter bo(&buf);
	for (size_t i=0;i<str.size()-1;i+=2)
	{
		char c=str[i];
		if (c>='0'&&c<='9') c-='0';
		if (c>='a'&&c<='f') c-=('a'-10);
		c <<= 4;
		char c2=str[i+1];
		if (c2>='0'&&c2<='9') c2-='0';
		if (c2>='a'&&c2<='f') c2-=('a'-10);
		c+=c2;
		bo << c;
	}
	buf.ResetRead();
	ByteOrderDataStreamReader boread(&buf);
	try {
		boread >> *this;
	}catch(...){
		return false;
	}
	return true;
}

std::string ScriptVM::exportToHex() const
{
	if (_code.size() == 0) return std::string();
	static char h[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	ByteOrderBuffer buf;
	ByteOrderDataStreamWriter bo(&buf);
	bo << *this;
	std::string ret;
	buf.ResetRead();
	const uint8_t * c = buf.begin();
	int size = buf.GetSize();

	for (int i=0;i < size;i++)
	{
		uint8_t low = c[i] %16;
		uint8_t high = c[i] /16;
		ret += h[high];
		ret += h[low];
	}
	return ret;
}

std::string ScriptVM::getProfilingData()
{
	std::ostringstream os;
	os <<  " opc:" << _totalOPC << "\n";

   for (std::map<int, ProfileResult>::const_iterator i = _Profiling.begin(); i!= _Profiling.end(); i++ )
   {
		int opcode = i->first;
		if (opcode < 0)
			continue;
		const ProfileResult &res =i->second;
		os << BytecodeVM::opcodes[opcode] << ": " << res.count << ", " << (res.ns/1000) << " us \n";
   }

   return os.str();
}

void ScriptVM::setExternalData(const ScriptVariant &data)
{
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags == NameRecord::bdNone)
			continue;

		const ScriptVariant &values =data[nr._name];
		int bound_size = nr._ptr.maxIndex - nr._ptr.index + 1;
		for (size_t j=0; j < values.listSize() && j < bound_size; j++ )
			nr._ptr.get(j)->setOpValue(values[j]);
	}
}

void ScriptVM::getExternalData(ScriptVariant &data)
{
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags == NameRecord::bdNone)
			continue;

		ScriptVariant &values =data[nr._name];
		int bound_size = nr._ptr.maxIndex - nr._ptr.index + 1;
		values.listResize(bound_size);
		for (size_t j=0; j < values.listSize() && j < bound_size; j++ )
			values[j] = *nr._ptr.get(j);
	}
}

void ScriptVM::getStackData(ScriptVariant &data)
{
	size_t b = StackFrameBottom();
	data.listResize(_stackSize - b);

	for (size_t i=b;i<_stackSize;i++)
	{
		ScriptVariant &o =_stack[i] ;
		data[i-b] = *o.getReferenced();
	}
}

void ScriptVM::getStaticData(ScriptVariant &data)
{
	for (size_t i=0;i< _nameTable.size();i++)
	{
		NameRecord & nr = _nameTable[i];
		if (nr._flags != NameRecord::bdNone) continue;
		ScriptVariant &values =data[nr._name];
		int bound_size = nr._staticValues.size();
		values.listResize(bound_size);
		for (size_t j=0; j < values.listSize() && j < bound_size; j++ )
			values[j] = nr._staticValues[j];
	}
}

void ScriptVM::runtimeError(const std::string &text)
{
	if (_errout)
	{
		(*_errout) << "[pc=" << _pc << "] Runtime error:" << text << std::endl;
		printStack();
	}
}
