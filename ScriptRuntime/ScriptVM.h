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

#include "BytecodeVM.h"

#include <ByteOrderStream.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iosfwd>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <functional>

/**
 * \brief Virtual bytecode machi for script exection
 *
 * Serialization through >>  and <<.
 * Bind external function using bindFunction, variables - bindVariable
 * Execute script calling run().
 */
class ScriptVM
{
	enum ExecutionStatus {
		Error,
		Success
	};

public:

	/// External variable binding.
	class NameRecord
	{
	public:
		enum BindDirection { bdNone = 0x00, bdInput = 0x01, bdOutput = 0x02, bdIO = bdInput | bdOutput };
		bool _resolved;
		std::string _name;
		uint32_t _sizeBytes;
		std::vector<ScriptVariant> _staticValues;
		BindDirection _flags;
		ScriptVariant::AddressPtr _ptr;
		NameRecord() :_resolved(false), _sizeBytes(0),_flags(bdNone){_ptr.container=0;_ptr.container2=0;}
		NameRecord(const char *c) :_resolved(false), _name(c), _sizeBytes(0),_flags(bdNone){_ptr.container=0;_ptr.container2=0;}
	};
	/// External function binding interface.
	class FuncNameRecordInterface
	{
	public:
		virtual ~FuncNameRecordInterface() = default;
		virtual void call(std::vector<ScriptVariant*>  &result, std::vector<ScriptVariant*>  &args) = 0;
	};
	/// External function binding
	struct FuncNameRecord
	{
		bool _resolved = false;
		std::string _name;
		using funCallback = std::function< void(std::vector<ScriptVariant*>  &, std::vector<ScriptVariant*>  & )>;
		funCallback _callback;
		FuncNameRecordInterface* _callback2 = nullptr;
	};


	static const int _formatVersion;

	enum DebugFlags { dNone = 0, dOpcode = 1 << 1, dStack = 1 << 2, dExternalVars = 1 << 3, dStaticVars = 1 << 4, dCallStack = 1 << 5, dOperations = 1 << 6,  dEmergencyMode = 1 << 7 };
	enum RunState { rsFinished, rsRunning };

	int _debugFlags;
	int _stepLimit;
	uint32_t _startPC;
	bool _isRunnable;
	bool _doExit;
	std::vector<BytecodeVM> _code;
	std::vector<NameRecord> _nameTable;
	std::vector<FuncNameRecord> _funcTable;
	std::ostream* _errout;
	std::ostream* _stdout;
	std::ostream* _debugout;
	RunState  _runState;
	bool _useBreakPoints;
	std::set<int> _breakPointPC;
	bool _useCurrentLine;
	std::set<int> _currentLinePC;
	bool _useSkipCalls;

	ScriptVM();
	~ScriptVM();

	void clear();

	void initialState();              //!< Reset VM state to initial.
	void run();

	int addVariable(std::string index, int size, NameRecord::BindDirection bd = NameRecord::bdIO);
	int addStaticVariable(std::string index, const std::vector<ScriptVariant> &values);
	int addFunction(std::string index);

	bool bindFunction( std::string index, FuncNameRecord::funCallback func);
	bool bindFunction( std::string index, FuncNameRecordInterface* func);
	bool bindVariable(std::string index, ScriptVariant::AddressPtr p, bool forceRebind = false);
	bool bindVariable(std::string index, std::vector<ScriptVariant*>& container, int indexInContainer = 0,int size = -1, bool forceRebind = false);
	bool bindVariable(std::string index, std::vector<ScriptVariant>& container, int indexInContainer = 0,int size = -1, bool forceRebind = false);

	void doAutoBindVars(std::vector<ScriptVariant>& container);
	bool checkExternalReferences();
	bool initStatic();
	bool hasFunction(std::string index);


	void printOpcodes();
	bool importFromHexString(const std::string &str);
	std::string exportToHex() const;

	operator bool () const { return _code.size(); }
	int getOpCnt() const {return _opCnt;}
	int getPC() const {return _pc;}
	int getMaxStackSize() const {return _stack.size();}

	std::string getProfilingData();

	void setExternalData(const ScriptVariant& data);
	void getExternalData(ScriptVariant& data);
	void getStackData(ScriptVariant& data);
	void getStaticData(ScriptVariant& data);


protected:
	void runtimeError(const std::string &text);
	ExecutionStatus executeOneCommand();
	void printStack();
	void printStatic();
	void printExternal();
	void printOpValueVector(const std::string &title, int bottom, int size, std::vector<ScriptVariant>& array);
	void printCallStack();

	inline void sPops(int size = 1){
		_stackSize -= size;
	}
	inline void sClear(){
		_stackSize = 0;
	}
	inline int sSize(){
		return _stackSize;
	}
	inline int sTopValue(int offset = 0){
		return _stack[sSize() - 1 - offset].getValue<int>();
	}
	inline int sValue(int addr = 0){
		return _stack[addr].getValue<int>();
	}
	inline ScriptVariant& sTop(int offset = 0){
		return _stack[sSize() -1 - offset];
	}
	inline ScriptVariant& sList(int offset, int index){
		return sTop(offset - index -1);
	}

	inline void sPush(const ScriptVariant& v, size_t size=1){
		if (_stack.size() < _stackSize + size){
			_stack.resize(_stackSize + size);
		}
		for (size_t i=0;i<size;i++)
			_stack[_stackSize+i]=v;
		_stackSize += size;
	}

	void termOperation(BytecodeVM::BinOp op, ScriptVariant::Types optype, BytecodeVM::BINOP_flags flags);
	void movs(BytecodeVM::MOVS_flags flags, int size);
	void cmps(BytecodeVM::CMPS_flags flags, int size);
	void unaryOperation(BytecodeVM::UnOp op, ScriptVariant::Types optype);

	void multOperation(BytecodeVM::BinOp op, ScriptVariant::Types optype, int count);

	struct CallStackFrame {
		int resultSize;
		int paramsSize;
		int returnAddress;
		int bottomAddress;
		int scopeLevel;
		CallStackFrame() {}
		CallStackFrame(int r, int p, int retA, int botA, int sl)
			: resultSize(r)
			, paramsSize(p)
			, returnAddress(retA)
			, bottomAddress(botA)
			, scopeLevel(sl)
		{}
	};

	inline int StackFrameBottom(int stackFrameIndex = 0) {
		return _stackFrames[_stackFrames.size() - 1 - stackFrameIndex].bottomAddress;
	}


	uint32_t _pc;
	uint32_t _opCnt;
	uint32_t _stackSize;
	std::vector<ScriptVariant> _stack;
	std::vector<ScriptVariant> _staticVars;
	std::vector<CallStackFrame> _stackFrames;
	int64_t _totalOPC;

	struct ProfileResult {
		int64_t ns;
		int count;
		ProfileResult() : ns(0), count(0) {}
	};
	std::map<int, ProfileResult> _Profiling;

};

ByteOrderDataStreamWriter& operator <<(ByteOrderDataStreamWriter& of,const ScriptVM& opc);
ByteOrderDataStreamReader& operator >>(ByteOrderDataStreamReader& ifs,ScriptVM& opc);

ByteOrderDataStreamWriter& operator <<(ByteOrderDataStreamWriter& of,const ScriptVM::NameRecord& opc);
ByteOrderDataStreamReader& operator >>(ByteOrderDataStreamReader& ifs,ScriptVM::NameRecord& opc);

ByteOrderDataStreamWriter& operator <<(ByteOrderDataStreamWriter& of,const ScriptVM::FuncNameRecord& opc);
ByteOrderDataStreamReader& operator >>(ByteOrderDataStreamReader& ifs,ScriptVM::FuncNameRecord& opc);

