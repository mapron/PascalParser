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

#include "ScriptVariant.h"

#include <ByteOrderStream.h>

#include <sstream>
#include <iomanip>

std::ostream &operator <<( std::ostream &debug, const ScriptVariant &opv)
{
	debug << opv.getString(true).c_str();
	return debug;
}

ScriptVariant ScriptVariant::_dumb;

std::string ScriptVariant::typenames[ScriptVariant::TYPES_COUNT] = {
	"bool",
	"float32",
	"float64",
	"int8",
	"uint8",
	"int16",
	"uint16",
	"int32",
	"uint32",
	"int64",
	"uint64",
	"*ptr",
	"string",
	"string_char",
	"array",
	"map"
};

ScriptVariant::ScriptVariant()
{
	_Type = T_UNDEFINED;
	_ValueChanged = false;
}

ScriptVariant::ScriptVariant(ScriptVariant::Types type)
	: _Type(type)
{
	_ValueChanged = false;
	if (_Type == T_string) {
		setValue(std::string());
	}else if (_Type != T_ptr){
		setValue(0);
	}
}


#define READ_STORAGE_CASE(type)  case T_##type: storage >>  _Data.f_##type;break
bool ScriptVariant::readFromByteStream(ByteOrderDataStreamReader &storage)
{
	storage >> _Type;
	switch (_Type){
		case T_bool: {uint8_t t;storage >> t; _Data.f_bool = t;}break;
		READ_STORAGE_CASE(float32) ;
		READ_STORAGE_CASE(float64) ;
		READ_STORAGE_CASE(int8_t) ;
		READ_STORAGE_CASE(uint8_t) ;
		READ_STORAGE_CASE(int16_t) ;
		READ_STORAGE_CASE(uint16_t) ;
		READ_STORAGE_CASE(int32_t) ;
		READ_STORAGE_CASE(uint32_t) ;
		READ_STORAGE_CASE(int64_t) ;
		READ_STORAGE_CASE(uint64_t) ;
		case T_string: {
			std::string t;
			if (!storage.ReadPascalString (t ) ){
				return false;
			}
			f_str.reset(new std::string(t));

		}
		break;
		case T_string_char: {
			char c;
			_Data.f_str_char = 0;
			storage >> c;
		}break;
		case T_array:  {
			uint32_t size;
			storage >> size;
			f_array.resize(size);
			for (size_t i =0; i< f_array.size(); i++) {
				f_array[i].readFromByteStream(storage);
			}
		}break;
		case T_map:  {
			uint32_t size;
			storage >> size;
			f_map.clear();
			f_map_keys.resize(size);
			for (size_t i =0; i< size; i++) {
				std::string t;
				if (!storage.ReadPascalString (t ) ){
					return false;
				}
				f_map_keys[i] = t;
				f_map[t].readFromByteStream(storage);
			}
		}break;

	}
	return true;
}
#define WRITE_STORAGE_CASE(type)  case T_##type:  storage  <<  _Data.f_##type;break
void ScriptVariant::writeToByteStream(ByteOrderDataStreamWriter &storage) const
{
	storage << _Type;
	switch (_Type){
		case T_bool: storage << _Data.f_bool;break;
		WRITE_STORAGE_CASE(float32) ;
		WRITE_STORAGE_CASE(float64) ;
		WRITE_STORAGE_CASE(int8_t) ;
		WRITE_STORAGE_CASE(uint8_t) ;
		WRITE_STORAGE_CASE(int16_t) ;
		WRITE_STORAGE_CASE(uint16_t) ;
		WRITE_STORAGE_CASE(int32_t) ;
		WRITE_STORAGE_CASE(uint32_t) ;
		WRITE_STORAGE_CASE(int64_t) ;
		WRITE_STORAGE_CASE(uint64_t) ;
		case T_string: {
			std::string t = f_str ? *f_str :  std::string();
			storage.WritePascalString ( t ) ;
		}break;
		case T_string_char: {
			storage << (_Data.f_str_char?*_Data.f_str_char : char(0) );
		}break;
		case T_array:  {
			storage << uint32_t(f_array.size());
			for (size_t i =0; i< f_array.size(); i++) {
				f_array[i].writeToByteStream(storage);
			}
		}break;
		case T_map:  {
			storage << uint32_t(f_map_keys.size());
			for (size_t i =0; i< f_map_keys.size(); i++) {
				storage.WritePascalString ( f_map_keys[i] ) ;
				((f_map.find(f_map_keys[i]))->second).writeToByteStream(storage);
			}
		}break;
	}
}
#define COMPARE_CASE(type)  case T_##type:  return  _Data.f_##type ==  Another._Data.f_##type
bool ScriptVariant::operator ==(const ScriptVariant &Another) const
{
	if (Another._Type != _Type) return false;
	switch (_Type){
		COMPARE_CASE(bool);
		COMPARE_CASE(float32) ;
		COMPARE_CASE(float64) ;
		COMPARE_CASE(int8_t) ;
		COMPARE_CASE(uint8_t) ;
		COMPARE_CASE(int16_t) ;
		COMPARE_CASE(uint16_t) ;
		COMPARE_CASE(int32_t) ;
		COMPARE_CASE(uint32_t) ;
		COMPARE_CASE(int64_t) ;
		COMPARE_CASE(uint64_t) ;
		case T_string: {
			return (*f_str) == (*Another.f_str);
		}
		case T_string_char: {
			return _Data.f_str_char && Another._Data.f_str_char && (*_Data.f_str_char) == (*Another._Data.f_str_char);
		}
		case T_array : return (f_array) == (Another.f_array);
		case T_map : return (f_map) == (Another.f_map);
		default: ; break;
	}
	return  false;
}

bool ScriptVariant::operator !=(const ScriptVariant &Another) const
{
	return !(*this == Another);
}

ScriptVariant::Types ScriptVariant::getType() const
{
	return ScriptVariant::Types(_Type);
}

void ScriptVariant::setType(ScriptVariant::Types type)
{
	const ScriptVariant copy = *this;
	_Type = type;
	setOpValue(copy);
}
#define DataPointer_CASE(type)  case T_##type:  return  (char*)&(_Data.f_##type)
char *ScriptVariant::getDataPointerInternal() const
{
	switch (_Type){
		DataPointer_CASE(bool);
		DataPointer_CASE(float32) ;
		DataPointer_CASE(float64) ;
		DataPointer_CASE(int8_t) ;
		DataPointer_CASE(uint8_t) ;
		DataPointer_CASE(int16_t) ;
		DataPointer_CASE(uint16_t) ;
		DataPointer_CASE(int32_t) ;
		DataPointer_CASE(uint32_t) ;
		DataPointer_CASE(int64_t) ;
		DataPointer_CASE(uint64_t) ;
		default: ; break;
	}
	return  nullptr;
}

size_t ScriptVariant::getDataPointerSize() const
{
	static size_t sizes[] = {sizeof(bool),sizeof(float32),sizeof(float64),1,1,2,2,4,4,8,8,0,0,0 };
	return sizes[_Type];
}

size_t ScriptVariant::getStorageSize() const
{
	size_t res = getDataPointerSize();
	if (_Type == T_string) res += f_str->size();
	res += 1;
	return res;
}

ScriptVariant::Types ScriptVariant::string2type(const std::string &str)
{
	std::map<std::string, ScriptVariant::Types>::const_iterator i = name2type.find(str);
	return i == name2type.end() ? T_UNDEFINED : i->second;
}

std::string ScriptVariant::type2string(ScriptVariant::Types type)
{
	return type < TYPES_COUNT ?  typenames[type] : "UNDEFINED";
}
const  std::map<std::string, ScriptVariant::Types> ScriptVariant::name2type = ScriptVariant::name2typeF();
std::map<std::string, ScriptVariant::Types> ScriptVariant::name2typeF()
{
	std::map<std::string, ScriptVariant::Types> ret;
	for (int i=0; i < TYPES_COUNT;i++) {
		ret[typenames[i]] = ScriptVariant::Types(i);
	}
	return ret;
}

ScriptVariant::~ScriptVariant()
{
}
#define COPY_CASE(type)  case T_##type:   _Data.f_##type =  another.getValue<type>(); break;

void ScriptVariant::setOpValue(const ScriptVariant &another)
{
	if (_Type == another._Type && _Type !=T_ptr){
	   *this = another;
		_ValueChanged = true;
		return;
	}
	_ValueChanged = true;

	switch (_Type){
		COPY_CASE(bool);
		COPY_CASE(float32) ;
		COPY_CASE(float64) ;
		COPY_CASE(int8_t) ;
		COPY_CASE(uint8_t) ;
		COPY_CASE(int16_t) ;
		COPY_CASE(uint16_t) ;
		COPY_CASE(int32_t) ;
		COPY_CASE(uint32_t) ;
		COPY_CASE(int64_t) ;
		COPY_CASE(uint64_t) ;
		case T_string: {
			(*f_str) = another.getString();
			break;
		}
		case T_string_char: {
			if (_Data.f_str_char){
				if (another.getType() == T_string) {
					std::string s = another.getString();
					(*_Data.f_str_char) = s.size() ? s[0] : 0;
				}else{
					(*_Data.f_str_char) = another.getValue<int8_t>();
				}
			}
			break;
		}
		case T_ptr:
			_Data.f_ptr.get()->setOpValue(another);
			break;
		default: ; break;
	}
}

void ScriptVariant::setOpValueAddress(const ScriptVariant &another)
{
	_ValueChanged = true;
	if (another._Type != T_ptr) {
		throw std::runtime_error("trying to set address of non-pointer!");
	}
	setPointer(another._Data.f_ptr);
}

std::string ScriptVariant::getString(bool useType, bool usePhysical) const
{
	return ConvertToString(useType, usePhysical);
}

void ScriptVariant::setString(const std::string& val)
{
	ConvertFromString(val);

}

void ScriptVariant::setStringReference(ScriptVariant &source, int n)
{
	_Type = T_string_char;
	_Data.f_str_char = 0;
	if (source._Type == T_string && n >=0 && n < source.f_str.get()->size()) {
		_Data.f_str_char = &((*source.f_str.get())[n]);
	}
}

void ScriptVariant::setPointer(std::vector<ScriptVariant> &c, int32_t i, int32_t size, bool autoDeref)
{
	ScriptVariant::AddressPtr ptr;
	ptr.container = &c;
	ptr.container2 = nullptr;
	ptr.index = i;
	if (size == -1)  size =  c.size();
	ptr.maxIndex = ptr.index + size - 1;
	setPointerDbg(ptr, autoDeref);
}

void ScriptVariant::setPointer(const ScriptVariant::AddressPtr &ptr, bool autoDeref)
{
	_Type = T_ptr;
	const ScriptVariant* referenced = ptr.get(0);
	if (autoDeref && referenced->_Type == T_ptr) {
		this->setPointer(referenced->_Data.f_ptr);
		return;
	}
	_Data.f_ptr = ptr;
}

void ScriptVariant::setPointerDbg(const ScriptVariant::AddressPtr &ptr, bool autoDeref)
{
	_Type = T_ptr;

	size_t i = ptr.index ;
	if (i > ptr.maxIndex) {
		std::ostringstream os;
		os << "Pointer has offset " << i << " with max offset " << ptr.maxIndex;
		throw std::runtime_error(os.str());
	}

	const ScriptVariant* referenced = ptr.getSafe(0);
	if (referenced == this){
		throw std::runtime_error("cyclic reference.");
	}
	if (autoDeref && referenced->_Type == T_ptr) {
		this->setPointer(referenced->_Data.f_ptr);
		return;
	}
	_Data.f_ptr = ptr;
}

void ScriptVariant::addPointer(int32_t i)
{
	if (_Type == T_ptr) {
		 ScriptVariant::AddressPtr ptr= this->_Data.f_ptr;
		 ptr.index += i;
		 this->setPointerDbg(ptr);
	}
}

const ScriptVariant *ScriptVariant::getReferenced(int offset, int limit) const
{
	if (limit == 0) return this;
	if (_Type== T_ptr) return _Data.f_ptr.getSafe(offset)->getReferenced(0, limit -1);
	return this;
}
ScriptVariant *ScriptVariant::getReferenced(int offset, int limit)
{
	if (limit == 0) return this;
	if (_Type== T_ptr){
		return _Data.f_ptr.getSafe(offset)->getReferenced(0, limit-1);
	}
	return this;
}


int ScriptVariant::getOffset() const
{
	if (_Type== T_ptr){
		return _Data.f_ptr.index;
	}
	return 0;
}

void ScriptVariant::listAppend(const ScriptVariant &val)
{
	_Type = T_array;
	f_array.push_back(val);
}

void ScriptVariant::listResize(size_t size)
{
	_Type = T_array;
	f_array.resize(size);
}

size_t ScriptVariant::listSize() const
{
	return f_array.size();
}

ScriptVariant &ScriptVariant::operator [](size_t index)
{
	if (_Type == T_array && index < f_array.size() ) {
		return f_array[index];
	}
	return _dumb;
}

const ScriptVariant &ScriptVariant::operator [](size_t index) const
{
	if (_Type == T_array && index < f_array.size() ) {
		return f_array[index];
	}
	return _dumb;
}

const std::vector<std::string> &ScriptVariant::mapKeys() const
{
	return f_map_keys;
}

void ScriptVariant::mapClear()
{
	_Type = T_map;
	f_map.clear();
	f_map_keys.clear();
}

ScriptVariant &ScriptVariant::operator [](const std::string &index)
{
	_Type = T_map;
	std::map<std::string, ScriptVariant>::const_iterator i = f_map.find(index);
	if (i == f_map.end()) {
		f_map_keys.push_back(index);
	}
	return f_map[index];
}

const ScriptVariant &ScriptVariant::operator [](const std::string &index) const
{
	std::map<std::string, ScriptVariant>::const_iterator i = f_map.find(index);
	if (i == f_map.end()) {
		return _dumb;
	}
	return i->second;
}

std::string printHex(char *Data, int Length)
{
	if (!Data)
		return std::string();
	std::ostringstream os;
	os << std::hex << std::setfill('0');
	for( int i = 0; i < Length; ++i )
		os << std::setw(2) << int((unsigned char)Data[i]) << ' ';
	os << std::dec;
	return os.str();
}

#define CONVERT_TO_STRING(type) case T_##type: os << _Data.f_##type; break;
std::string ScriptVariant::ConvertToString (bool useType, bool usePhysical, int maxRefCount) const
{
	std::ostringstream os;
	os.precision( 15);
	if (maxRefCount < 0) {
		os << "(CYCLIC REFERENCE)";
		return os.str();
	}
	if (useType && _Type < T_UNDEFINED) os << typenames[_Type] + ": ";
	switch (_Type)
	{
		case T_bool:
			os << (_Data.f_bool ? "true" : "false");
			break;
		case T_int8_t:
			os << int(_Data.f_int8_t);
			break;
		case T_uint8_t:
			os << int(_Data.f_uint8_t);
			break;
		CONVERT_TO_STRING(int16_t)
		CONVERT_TO_STRING(uint16_t)
		CONVERT_TO_STRING(int32_t)
		CONVERT_TO_STRING(uint32_t)
		CONVERT_TO_STRING(int64_t)
		CONVERT_TO_STRING(uint64_t)
		CONVERT_TO_STRING(float32)
		CONVERT_TO_STRING(float64)
		case T_string:
			os << (f_str ? *f_str : std::string());
			break;
		case T_ptr:{
			if (useType) os << "[" <<_Data.f_ptr.index << "]";
			if (usePhysical) os << "{" << std::hex << _Data.f_ptr.get() << "}";
			os << _Data.f_ptr.get()->ConvertToString(useType, usePhysical, maxRefCount -1);
		}
			break;
		case T_string_char:
			if (_Data.f_str_char)
				os << *_Data.f_str_char;
			break;
		case T_array: {
			 os << "[ ";
			 for (size_t i=0; i< f_array.size();i++) {
				 if (i>0) os << ", ";
				 os << f_array[i].getString();
			 }
			 os << " ]";
		}break;
		case T_map: {
			 os << "{ ";
			 for (size_t i=0; i< f_map_keys.size();i++) {
				 if (i>0) os << ", ";
				 os << f_map_keys[i] << ": ";
				 os << (f_map.find(f_map_keys[i])->second).getString();
			 }
			 os << " }";
		}break;
		case T_UNDEFINED:

			os << "UNDEFINED";
			break;
		default:
			os << "Type:" << _Type;
	}
	return os.str();
}


#define CONVERT_FROM_STR(type) case T_##type: is >> _Data.f_##type; break;
bool ScriptVariant::ConvertFromString (const std::string &Input)
{
	std::istringstream is(Input);
	switch (_Type)
	{
		case T_bool:
			_Data.f_bool = (Input != "0" && Input != "false");
			break;
		case T_int8_t:
			{int temp;
			is >> temp;
			_Data.f_int8_t = temp;}
			break;
		case T_uint8_t:
			{int temp;
			is >> temp;
			_Data.f_uint8_t = temp;}
			break;
		CONVERT_FROM_STR(int16_t)
		CONVERT_FROM_STR(uint16_t)
		CONVERT_FROM_STR(int32_t)
		CONVERT_FROM_STR(uint32_t)
		CONVERT_FROM_STR(int64_t)
		CONVERT_FROM_STR(uint64_t)
		CONVERT_FROM_STR(float32)
		CONVERT_FROM_STR(float64)
		case T_string:
			f_str.reset(new std::string(Input));
			break;
		case T_ptr:
			_Data.f_ptr.get()->ConvertFromString(Input);
			break;
		case T_string_char:
			if (_Data.f_str_char)
				is >> *_Data.f_str_char;
			break;
		default:
			return false;
	}
	return true;
}


ByteOrderDataStreamWriter &operator <<(ByteOrderDataStreamWriter &of, const ScriptVariant &opv)
{
	opv.writeToByteStream(of);
	return of ;
}

ByteOrderDataStreamReader &operator >>(ByteOrderDataStreamReader &ifs, ScriptVariant &opv)
{
	opv.readFromByteStream(ifs);
	return ifs;
}


ScriptVariant *ScriptVariant::AddressPtr::getSafe(size_t offset)
{
   if (container2) {
	   if (container2->size() < index+offset) throw std::runtime_error("Too large AddressPtr index!");
	   return (*container2)[index+offset];
   }
   if (container) {
	   if (container->size() < index+offset) throw std::runtime_error("Too large AddressPtr index!");
	   return &((*container)[index+offset]);
   }
   throw std::runtime_error("No valid container found!");
}

const ScriptVariant *ScriptVariant::AddressPtr::getSafe(size_t offset) const
{
   if (container2) {
	   if (container2->size() < index+offset)
		   throw std::runtime_error("Too large AddressPtr index!");
	   return (*container2)[index+offset];
   }
   if (container) {
	   if (container->size() < index+offset)
		   throw std::runtime_error("Too large AddressPtr index!");
	   return &((*container)[index+offset]);
   }
   throw std::runtime_error("No valid container found!");
}
