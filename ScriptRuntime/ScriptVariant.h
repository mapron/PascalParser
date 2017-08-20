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

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <typeinfo>
#include <stdexcept>
#include <memory>

class ByteOrderDataStreamReader;
class ByteOrderDataStreamWriter;
class ScriptVariant;
template <class T>
struct ScriptVariantGetter{
static inline T get(const ScriptVariant* opv, int maxRefCount);
};


template <class T>
struct ScriptVariantSetter {
static inline void set(ScriptVariant* opv, const T& value);
};
class ScriptVariant;
/**
 * \brief boost::variant-alike structure. Holds integer/real/string.
 *
 * Can hold pointers to another variant.
 * Have ability to detect if value was changed
 */
class ScriptVariant
{
	template <class T> friend  struct ScriptVariantGetter;
	template <class T> friend  struct ScriptVariantSetter;
	static ScriptVariant _dumb;
public:
	// for macro usage.
	using float32 = float ;
	using float64 = double ;

	enum Types {
		T_AUTO = 255,

		T_bool=0,
		T_float32,
		T_float64,
		T_int8_t,
		T_uint8_t,
		T_int16_t,
		T_uint16_t,
		T_int32_t ,
		T_uint32_t,
		T_int64_t,
		T_uint64_t,

		T_ptr,      //11
		T_string,   //12
		T_string_char,
		T_array,
		T_map,
		TYPES_COUNT,
		T_UNDEFINED = TYPES_COUNT,

		T_float = T_float32,
		T_double = T_float64

	};
	static bool inline isTypeFloat(Types type)
	{
		return (type >= T_float32) && (type <=  T_float64);
	}
	static bool inline isTypeInt(Types type)
	{
		return (type >= T_int8_t) && (type <=  T_uint64_t);
	}


	struct AddressPtr {
		std::vector<ScriptVariant> *container;
		std::vector<ScriptVariant*> *container2;
		size_t index;
		size_t maxIndex;
		inline ScriptVariant* get(size_t offset = 0)             { return container2? (*container2)[index+offset] : &((*container)[index+offset]);}
		inline const ScriptVariant* get(size_t offset = 0) const { return container2? (*container2)[index+offset] : &((*container)[index+offset]);}
		ScriptVariant* getSafe(size_t offset = 0); // throw
		const ScriptVariant* getSafe(size_t offset = 0) const; // throw
	};

	unsigned char _Type;
	bool _ValueChanged;

	template<class T>
	T getValue() const;

	template<class T>
	T getValueCounted(int maxRefCount = MAX_REFERENCE_DEPTH ) const;

	template<class T>
	void getValue(T &val) const;

	template<class T>
	void setValue(const T& val, unsigned char newType = T_UNDEFINED);

	void setOpValue(const ScriptVariant& another);
	void setOpValueAddress(const ScriptVariant& another);

	std::string getString(bool useType = false,bool usePhysical = false) const;
	void setString(const std::string &val);
	void setStringReference(ScriptVariant& source, int n);

	void setPointer(std::vector<ScriptVariant>& c, int32_t i, int32_t size, bool autoDeref = true);
	void setPointer(const ScriptVariant::AddressPtr& ptr, bool autoDeref = true);
	void setPointerDbg(const ScriptVariant::AddressPtr& ptr, bool autoDeref = true);
	void addPointer(int32_t i);

	const ScriptVariant *getReferenced(int offset = 0, int limit = -1) const;
	ScriptVariant *getReferenced(int offset = 0, int limit = -1);
	int getOffset() const;

	void listAppend(const ScriptVariant& val);
	void listResize(size_t size);
	size_t listSize() const;
	inline void listClear() { listResize(0); }

	ScriptVariant& operator [] (size_t index);
	const ScriptVariant& operator [] (size_t index) const;

	const std::vector<std::string>& mapKeys() const;
	void mapClear();

	ScriptVariant& operator [] (const std::string& index);
	const ScriptVariant& operator [] (const std::string& index) const;

	ScriptVariant();
	ScriptVariant(Types type);

	template <class T>
	inline ScriptVariant(const T& value){
		 _ValueChanged = false;
		 setValue(value, T_AUTO );
	}
	inline ScriptVariant(const char* value){
		 _ValueChanged = false;
		 setValue(std::string(value), T_string );
	}
	~ScriptVariant();

	bool readFromByteStream(ByteOrderDataStreamReader& storage);
	void writeToByteStream(ByteOrderDataStreamWriter& storage) const;

	bool operator ==(const ScriptVariant &Another) const;
	bool operator !=(const ScriptVariant &Another) const;

	Types getType() const;
	void setType(Types type);

	char* getDataPointerInternal() const;
	size_t getDataPointerSize() const;
	size_t getStorageSize() const;

	static Types string2type(const std::string& str);
	static std::string type2string(Types type);

private:
	union {
		bool f_bool;
		float f_float32;
		double f_float64;
		int8_t f_int8_t;
		uint8_t f_uint8_t;
		int16_t f_int16_t;
		uint16_t f_uint16_t;
		int32_t f_int32_t;
		uint32_t f_uint32_t;
		int64_t f_int64_t;
		uint64_t f_uint64_t;
		AddressPtr f_ptr;
		char      *f_str_char;
	} _Data;
	std::shared_ptr<std::string> f_str;
	std::vector<ScriptVariant> f_array;
	std::map<std::string, ScriptVariant> f_map;
	std::vector<std::string> f_map_keys;
	static const int MAX_REFERENCE_DEPTH = 32;
	template<class T>
	inline Types determine(const T& ){
		return T_UNDEFINED;
	}

	inline Types determine(const bool& ) {return T_bool;}
	inline Types determine(const float& ) {return T_float32;}
	inline Types determine(const double& ) {return T_float64;}
	inline Types determine(const int8_t& ) {return T_int8_t;}
	inline Types determine(const uint8_t& ) {return T_uint8_t;}
	inline Types determine(const int16_t& ) {return T_int16_t;}
	inline Types determine(const uint16_t& ) {return T_uint16_t;}
	inline Types determine(const int32_t& ) {return T_int32_t;}
	inline Types determine(const uint32_t& ) {return T_uint32_t;}
	inline Types determine(const int64_t& ) {return T_int64_t;}
	inline Types determine(const uint64_t& ) {return T_uint64_t;}
	inline Types determine(const ScriptVariant*& ) {return T_ptr;}
	inline Types determine(const std::string& ) {return T_string;}

	std::string ConvertToString(bool useType = true,bool usePhysical = false, int maxRefCount = MAX_REFERENCE_DEPTH) const;
	bool ConvertFromString(const std::string &Input);

	 static std::string typenames[TYPES_COUNT];
	 static const std::map<std::string, Types> name2type;
	 static std::map<std::string, Types> name2typeF();
};

ByteOrderDataStreamWriter& operator <<(ByteOrderDataStreamWriter& of,const ScriptVariant& opv);
ByteOrderDataStreamReader& operator >>(ByteOrderDataStreamReader& ifs,ScriptVariant& opv);

std::ostream& operator <<( std::ostream & debug,const ScriptVariant& opv);


template<class T>
T ScriptVariant::getValue() const
{
	return ScriptVariantGetter<T>::get(this, MAX_REFERENCE_DEPTH);
}
template<class T>
T ScriptVariant::getValueCounted(int maxRefCount) const
{
	return ScriptVariantGetter<T>::get(this, maxRefCount);
}

template <class T>
inline T ScriptVariantGetter<T>::get(const ScriptVariant* opv, int maxRefCount){
	if (maxRefCount < 0) {
		throw std::runtime_error("cyclic reference.");
	}
	switch (opv->_Type){
		case ScriptVariant::T_bool:       return T(opv->_Data.f_bool);
		case ScriptVariant::T_float32:    return T(opv->_Data.f_float32);
		case ScriptVariant::T_float64:    return T(opv->_Data.f_float64);
		case ScriptVariant::T_int8_t:     return T(opv->_Data.f_int8_t);
		case ScriptVariant::T_uint8_t:    return T(opv->_Data.f_uint8_t);
		case ScriptVariant::T_int16_t:    return T(opv->_Data.f_int16_t);
		case ScriptVariant::T_uint16_t:   return T(opv->_Data.f_uint16_t);
		case ScriptVariant::T_int32_t:    return T(opv->_Data.f_int32_t);
		case ScriptVariant::T_uint32_t:   return T(opv->_Data.f_uint32_t);
		case ScriptVariant::T_int64_t:    return T(opv->_Data.f_int64_t);
		case ScriptVariant::T_uint64_t:   return T(opv->_Data.f_uint64_t);
		case ScriptVariant::T_string_char:   return T(opv->_Data.f_str_char? *opv->_Data.f_str_char : 0);
		case ScriptVariant::T_ptr: {
			const ScriptVariant* o =opv->_Data.f_ptr.get();
			return o->getValueCounted<T>(maxRefCount -1);
		}
		case ScriptVariant::T_string: {
		   ScriptVariant tmp;
		   tmp.setValue(T(), ScriptVariant::T_AUTO);
		   if (opv->f_str) tmp.setValue(*(opv->f_str));
		   return tmp.getValue<T>();
		}

	}
	return T(0);
}
template<>
inline std::string ScriptVariantGetter<std::string>::get(const ScriptVariant* opv, int maxRefCount)
{
	switch (opv->_Type){
		case ScriptVariant::T_bool:
		case ScriptVariant::T_float32:
		case ScriptVariant::T_float64:
		case ScriptVariant::T_int8_t:
		case ScriptVariant::T_uint8_t:
		case ScriptVariant::T_int16_t:
		case ScriptVariant::T_uint16_t:
		case ScriptVariant::T_int32_t:
		case ScriptVariant::T_uint32_t:
		case ScriptVariant::T_int64_t:
		case ScriptVariant::T_uint64_t:
		case ScriptVariant::T_string_char:{
			return opv->getString(false);
		}
		case ScriptVariant::T_ptr: return opv->_Data.f_ptr.get()->getValueCounted<std::string>(maxRefCount-1);
		case ScriptVariant::T_string:
		   return *(opv->f_str);
	}
	return std::string();
}

template <class T>
inline void ScriptVariantSetter<T>::set(ScriptVariant* opv, const T& value){
	switch (opv->_Type){
		case ScriptVariant::T_bool: opv->_Data.f_bool            = (value ? true : false); break;
		case ScriptVariant::T_float32: opv->_Data.f_float32      = float(value); break;
		case ScriptVariant::T_float64: opv->_Data.f_float64      = double(value); break;
		case ScriptVariant::T_int8_t: opv->_Data.f_int8_t        = int8_t(value); break;
		case ScriptVariant::T_uint8_t: opv->_Data.f_uint8_t      = uint8_t(value); break;
		case ScriptVariant::T_int16_t:  opv->_Data.f_int16_t     = int16_t(value); break;
		case ScriptVariant::T_uint16_t: opv->_Data.f_uint16_t    = uint16_t(value); break;
		case ScriptVariant::T_int32_t: opv->_Data.f_int32_t      = int32_t(value); break;
		case ScriptVariant::T_uint32_t: opv->_Data.f_uint32_t    = uint32_t(value); break;
		case ScriptVariant::T_int64_t: opv->_Data.f_int64_t      = int64_t(value); break;
		case ScriptVariant::T_uint64_t: opv->_Data.f_uint64_t    = uint64_t(value); break;
		case ScriptVariant::T_ptr: opv->_Data.f_ptr.get()->setValue(value); break;
		case ScriptVariant::T_string: {
			ScriptVariant tmp;
			tmp.setValue(value, ScriptVariant::T_AUTO);
			opv->f_str.reset( new std::string(tmp.getValue<std::string>()));
		 }break;
		case ScriptVariant::T_string_char:
			if (opv->_Data.f_str_char)  *opv->_Data.f_str_char=value;
		break;

	}
}

template <>
inline void ScriptVariantSetter<std::string>::set(ScriptVariant* opv, const std::string& value){
	switch (opv->_Type){
		case ScriptVariant::T_bool:
		case ScriptVariant::T_float32:
		case ScriptVariant::T_float64:
		case ScriptVariant::T_int8_t:
		case ScriptVariant::T_uint8_t:
		case ScriptVariant::T_int16_t:
		case ScriptVariant::T_uint16_t:
		case ScriptVariant::T_int32_t:
		case ScriptVariant::T_uint32_t:
		case ScriptVariant::T_int64_t:
		case ScriptVariant::T_uint64_t:
		case ScriptVariant::T_string_char:
			opv->setString(value);
		break;

		case ScriptVariant::T_ptr: opv->_Data.f_ptr.get()->setValue(value); break;
		case ScriptVariant::T_string: {
			opv->f_str.reset( new std::string(value));
		 }break;
	}
}

template<class T>
void ScriptVariant::getValue(T& val) const
{
	val = getValue<T>();
}

template<class T>
void ScriptVariant::setValue(const T& value, unsigned char newType)
{
	if (newType == T_AUTO){
		newType = determine(value);

	}
	if (newType < T_UNDEFINED){
		_Type = newType;
	}
	ScriptVariantSetter<T>::set(this, value);

}
