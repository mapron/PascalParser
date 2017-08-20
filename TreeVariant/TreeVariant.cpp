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
#include "TreeVariant.h"

#include <ByteOrderStream.h>

#include <QFile>
#include <QBuffer>
#include <QDebug>

const TreeVariant TreeVariant::emptyValue;

// ----------------------------------------------

TreeVariant::TreeVariant() :
	QVariant()
{
}

//--------------------------------------------------------------------------------
TreeVariant::TreeVariantList& TreeVariant:: asList()
{
	_type = List;
	return _list;
}

//--------------------------------------------------------------------------------
TreeVariant::TreeVariantMap &TreeVariant::asMap()
{
	_type = Map;
	return _map;
}

const TreeVariant::TreeVariantList &TreeVariant::asList() const
{
	return _list;
}

const TreeVariant::TreeVariantMap &TreeVariant::asMap() const
{
	return _map;
}

QStringList TreeVariant::keys() const
{
	return _mapKeys;
}

//--------------------------------------------------------------------------------
TreeVariant::VariantType TreeVariant::vtype() const
{return _type;}

void TreeVariant::setVtype(TreeVariant::VariantType type)
{_type=type;}

//--------------------------------------------------------------------------------
TreeVariant TreeVariant::fromQVariant(QVariant val)
{
	TreeVariant return_value = val;
	if (val.type() == QVariant::Map)
	{
		QVariantMap vm = val.toMap();
		QMapIterator<QString, QVariant> i(vm);
		while (i.hasNext())
		{
			i.next();
			return_value._map.insert(i.key(), TreeVariant::fromQVariant(i.value())) ;
			return_value._mapKeys << i.key();
		}
		return_value._type = Map;
	}
	if (val.type() == QVariant::List || val.type() == QVariant::StringList)
	{
		QVariantList vm = val.toList();
		QListIterator<QVariant> i(vm);
		while (i.hasNext())
		{
			return_value._list.push_back( TreeVariant::fromQVariant(i.next()) );
		}
		return_value._type = List;
	}
	return return_value;
}

//--------------------------------------------------------------------------------
TreeVariant &TreeVariant::operator [](const QString& index)
{
	_type = Map;
	if (!_map.contains(index))
		_mapKeys << index;

	return _map[index];

}
//--------------------------------------------------------------------------------
const TreeVariant TreeVariant::operator [](const QString& index) const
{
	if (_type != Map)
		return emptyValue;

	if (!_map.contains(index))
		return emptyValue;
	return _map.value(index);

}
//--------------------------------------------------------------------------------
TreeVariant &TreeVariant::operator [](const int index)
{
	while(_list.size() <= index)
		_list.append(TreeVariant());
	_type = List;
	return _list[index];
}
//--------------------------------------------------------------------------------
const TreeVariant& TreeVariant::operator [](const int index) const
{
	if(_list.size() <= index || index < 0)
		return emptyValue;
	return _list[index];
}

//--------------------------------------------------------------------------------
QMap<QString, QString> TreeVariant::asStringMap() const
{
	QMap<QString, QString> ret;
	if (vtype() == TreeVariant::Map)
	{
		const TreeVariantMap& vm = asMap();
		foreach (QString key, vm.keys())
			ret.insert(key, vm[key].toString());
	}
	return ret;
}

QVariantMap TreeVariant::asVariantMap() const
{
	QVariantMap ret;
	if (vtype() == TreeVariant::Map)
	{
		const TreeVariantMap& vm = asMap();
		foreach (QString key, vm.keys())
			ret.insert(key, vm[key].asVariant());
	}
	return ret;
}

//--------------------------------------------------------------------------------
QStringList TreeVariant::asStringList() const
{
	QStringList ret;
	if (vtype() == TreeVariant::List)
	{
		const TreeVariantList& vm = this->asList();
		for (int i=0;i<vm.size();i++)
			ret << vm[i].toString() ;
	}
	return ret;
}

QVariant TreeVariant::asVariant() const
{
	if (vtype() == TreeVariant::List)
	{
		QVariantList ret;
		const TreeVariantList& vm = this->asList();
		for (int i=0;i<vm.size();i++)
			ret << vm[i].asVariant();
		return ret;
	}
	if (vtype() == TreeVariant::Map)
	{
		return asVariantMap();
	}
	return *this;
}

//--------------------------------------------------------------------------------
void TreeVariant::remove(const QString& key)
{
	_type = Map;
	_map.remove(key);
	_mapKeys.removeAll(key);
}

//--------------------------------------------------------------------------------
void TreeVariant::append(const TreeVariant &val)
{
	_type = List;
	_list.append(val);
}

//--------------------------------------------------------------------------------
int TreeVariant::listSize() const
{
	return _list.size();
}

int TreeVariant::mapSize() const
{
	return _map.size();
}

//--------------------------------------------------------------------------------
bool TreeVariant::contains(const QString &key) const
{
	if ( _type != Map)
		return false;
	return  _map.contains(key);
}

//--------------------------------------------------------------------------------
bool TreeVariant::saveToFile(QString filename, QString type, QDataStream::ByteOrder byteOrder) const
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	if (type == "extensionGuess")
	{
		if (filename.right(5) == ".yaml"|| filename.right(5) == "-yaml")
			type = "yaml";

		if (filename.right(5) == ".json")
			type = "json";

		if (filename.right(7) == ".ubjson")
			type = "ubjson";

	}
	const bool res = saveToDevice(&file, type, byteOrder);

	file.close();
	return res;
}

//--------------------------------------------------------------------------------
bool TreeVariant::saveToDevice(QIODevice *device, QString type, QDataStream::ByteOrder byteOrder) const
{
	QString yaml;
	if (type == "yaml")
	{
		this->toYamlString(yaml);
		device->write(yaml.toUtf8());
	}
	else if (type == "json")
	{
		this->toYamlString(yaml, true);
		device->write(yaml.toUtf8());
	}
	else if (type == "ubjson")
	{
		QDataStream ds(device);
		ds.setByteOrder(byteOrder);
		this->toUbjson(ds);
	}
	else
		return false;
	return true;
}

//--------------------------------------------------------------------------------
bool TreeVariant::saveToByteArray(QByteArray &buffer, QString type, QDataStream::ByteOrder byteOrder) const
{
	QBuffer buf(&buffer);
	buf.open(QIODevice::WriteOnly);
	bool res = this->saveToDevice(&buf, type, byteOrder);
	return res;
}

//--------------------------------------------------------------------------------
QDebug operator<<(QDebug dbg, const TreeVariant &c)
{
	QString yaml;
	c.toYamlString(yaml);
	dbg.nospace() << yaml.toUtf8().constData();
	return  dbg.nospace();
}

QDebug operator<<(QDebug dbg, const TreeVariant::VariantType &c)
{
	switch (c)
	{
	case TreeVariant::Scalar: dbg.nospace() << "scalar";
		break;
	case TreeVariant::Map: dbg.nospace() << "map";
		break;
	case TreeVariant::List: dbg.nospace() << "list";
		break;

	}
	return dbg.space();
}

//--------------------------------------------------------------------------------
TreeVariant *TreeVariant::findBy(QString field, TreeVariant value)
{
	TreeVariant *return_value=nullptr;
	if (_type == TreeVariant::List)
	{
		for (int i=0;i<_list.size();i++)
		{
			if (_list[i].contains(field) && _list[i][field] == value)
				return &(_list[i]);
		}
	}
	return return_value;
}


//--------------------------------------------------------------------------------
void TreeVariant::merge(const TreeVariant &overwrite,const QStringList& except, bool owerwrite_lists, bool trunc_lists)
{
	if (_type == Map)
	{
		foreach (QString key, overwrite._mapKeys)
		{
			TreeVariant val = overwrite._map[key];
			bool isExcept = except.contains(key);
			if (!isExcept && ( val._type == Map || val._type == List)) {
				(*this)[key].merge(val, except, owerwrite_lists, trunc_lists);
			} else {
				(*this)[key] = val;
			}
		}
	}
	else if (_type == List)
	{
		if (trunc_lists)
		{
			while (_list.size() > overwrite._list.size())
				_list.removeLast();
		}
		for (int j=0; j< overwrite._list.size(); j++)
		{
			if (owerwrite_lists && _list.size() > j)
				_list[j].merge(overwrite._list[j], except,  owerwrite_lists, trunc_lists);
			else
				_list.append(overwrite._list[j]);

		}
	}
	else // simple copy
	{
		_type = overwrite._type;
		(*this) = overwrite;
	}
}

void TreeVariant::clear()
{
	QVariant::clear();
	_list.clear();
	_map.clear();
	_mapKeys.clear();
}

//--------------------------------------------------------------------------------
bool TreeVariant::toYamlString(QString &ret, bool json, int level)  const
{
	// Old ugly code. Function supports json and yaml output (yaml may be compressed to inline)
	QString indent_str = QString("  ").repeated(level);
	bool ok;
	this->toDouble(&ok);
	switch (_type)
	{
	case TreeVariant::Scalar:{
		if (!this->isValid())
			ret += "~";
		else if (this->type() == QVariant::Bool)
			ret += this->toBool() ? "true" : "false";
		else if (ok)
			ret += this->toString();
		else
			ret += escape(toString());

		break;
	}case TreeVariant::Map: {
		if (!json && isFlatList()){
			ret += "{";
			int i=0;
			foreach (const QString& key,_mapKeys)
			{
				if (i++ > 0) ret += ", ";
				ret +=  escape(key) + ": " ;
				_map[key].toYamlString(ret, json, level + 1);
			}
			ret += "}";
			break;
		}
		if (json) ret += "{\r\n";
		int i=0;
		foreach (const QString& key,_mapKeys)
		{
			const QString escapedKey = escape(key);
			const bool nonFlat = !json && !_map[key].isFlatList();
			ret += indent_str + escapedKey + ": " ;
			if (nonFlat) ret += "\r\n";
			_map[key].toYamlString(ret, json, level + 1);
			if (nonFlat) ret +=  (json &&  i< _map.size()-1 ?",":"");
			else ret +=  QString(json && i< _map.size()-1 ?",":"") + "\r\n";
			i++;
		}
		if (json) ret += indent_str + "}"+ "\r\n";
	}
		break;
	case TreeVariant::List: {
		if (isFlatList())
		{
			ret += "[";
			for (int i=0; i< _list.size(); i++)
			{
				if (i >0) ret += ", ";
				_list[i].toYamlString(ret, json, level + 1);
			}
			ret += "]";
			break;
		}
		if (json) ret += "[\r\n";
		for (int i=0; i< _list.size(); i++)
		{
			ret += indent_str;
			if (!json) ret += "- ";
			const bool nonFlat = !json && !_list[i].isFlatList();
			if (nonFlat) ret += "\r\n";
			_list[i].toYamlString(ret, json, level + 1) ;
			if (nonFlat) {;}
			else ret += QString(json && i< _list.size()-1 ?",":"") + "\r\n";


		}
		if (json) ret += indent_str + "]"+"\r\n";
	}
		break;
	}
	return true;
}

void TreeVariant::optimize()
{
	switch (_type)
	{
	case TreeVariant::Scalar: break;
	case TreeVariant::List:
		for (int i=0;i<_list.size();i++)
			_list[i].optimize();

		break;
	case TreeVariant::Map:   {
		foreach (const QString& key, _mapKeys)
		{
			_map[key].optimize();
			if (_map[key].isZero()) {
				_map.remove(key);
				_mapKeys.removeAll(key);
			}
		}
	}

	}
}

bool TreeVariant::isZero()
{
	switch (_type)
	{
	case TreeVariant::Scalar: return !this->isValid();
	case TreeVariant::List:   return !_list.size();
	case TreeVariant::Map:   return !_map.size();
	}
	return true;
}

void TreeVariant::toUbjson(QDataStream &ds) const
{
	// QByteArray ret;
	// QByteArray t;

	switch (_type)
	{
	case TreeVariant::Scalar:
	{
		bool okD;
		double valueD = this->toDouble(&okD);
		bool okI;
		int valueI = this->toInt(&okI);
		if (_typeHint!=TH_none){
			okI = false;
			okD = false;
		}

		if (this->type() == QVariant::Bool || _typeHint == TH_bool){
			ds << (quint8)(this->toBool() ? 'T' : 'F');
		}else if (this->type() == QVariant::Invalid || this->isNull()){
			ds << (quint8)'Z' ;
		}else if (okD || _typeHint == TH_float || _typeHint == TH_double){
			if (_typeHint == TH_float){
				ds << (quint8)'d' << (float) valueD;
			}else{
				ds << (quint8)'D' << valueD;
			}
		}else if (okI || _typeHint == TH_int || _typeHint == TH_int16|| _typeHint == TH_byte|| _typeHint == TH_int64){
			if (_typeHint == TH_byte){
				ds << (quint8)'B';
				unsigned char value = valueI;
				ds << value;
			}else if (_typeHint == TH_int16){
				ds << (quint8)'i';
				short value = valueI;
				ds << value;
			}else{
				ds << (quint8)'I' <<valueI;
			}
		}else{
			QByteArray str = this->toString().toUtf8();
			str += '\0';
			if (str.size() < 254){
				ds << (quint8)'s';
				unsigned char sizeC = (unsigned char) str.size();
				ds << sizeC;
				ds.writeRawData(str.constData(), str.size());

			}else{
				ds << (quint8)'S';
				int sizeI = (int) str.size();
				ds << sizeI;
				ds.writeRawData(str.constData(), str.size());
			}
		}
	}
		break;
	case TreeVariant::Map: {
		if (_map.size() < 254){
			ds << (quint8)'o';
			unsigned char sizeC = (unsigned char) _map.size();
			ds << sizeC;

		}else{
			ds << (quint8)'O';
			int sizeI = (int) _map.size();
			ds << sizeI;
		}
		foreach (const QString& k, _mapKeys)
		{
			TreeVariant key(k);
			key._typeHint=_typeHintKeys;
			key.toUbjson(ds);
			_map[k].toUbjson(ds);
		}

	}
		break;
	case TreeVariant::List: {
		if (_list.size() < 254){
			ds << (quint8)'a';
			unsigned char sizeC = (unsigned char) _list.size();
			ds << sizeC;

		}else{
			ds << (quint8)'A';
			int sizeI = (int) _list.size();
			ds <<sizeI;
		}
		for (int i=0; i< _list.size(); i++)
		{
			_list[i].toUbjson(ds);
		}
	}
		break;
	}
}

//--------------------------------------------------------------------------------
bool TreeVariant::isFlatList()  const
{
	if (_type == Scalar) return true;
	if (_type == List)
		for (int i=0; i < _list.size(); i++){
			if (_list[i]._type != Scalar) return false;
		}
	if (_type == Map){
		if ( _map.size() > 5) return false;
		foreach (const QString k, _mapKeys){
			if (_map[k]._type != Scalar) return false;
		}
	}
	return true;
}


//--------------------------------------------------------------------------------
void TreeVariant::addLists(QString val1,QString val2,QString val3,QString val4,QString val5) {
	_type = Map;
	if (!val1.isEmpty()) (*this)[val1].setVtype(List);
	if (!val2.isEmpty()) (*this)[val2].setVtype(List);
	if (!val3.isEmpty()) (*this)[val3].setVtype(List);
	if (!val4.isEmpty()) (*this)[val4].setVtype(List);
	if (!val5.isEmpty()) (*this)[val5].setVtype(List);
}

//--------------------------------------------------------------------------------
void TreeVariant::addMaps(QString val1, QString val2, QString val3, QString val4, QString val5)
{
	_type = Map;
	if (!val1.isEmpty()) (*this)[val1].setVtype(Map);
	if (!val2.isEmpty()) (*this)[val2].setVtype(Map);
	if (!val3.isEmpty()) (*this)[val3].setVtype(Map);
	if (!val4.isEmpty()) (*this)[val4].setVtype(Map);
	if (!val5.isEmpty()) (*this)[val5].setVtype(Map);
}

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
const TreeVariant &TreeVariant::operator=(const QStringList a)
{
	this->_list.clear();
	foreach (QString s, a){
		this->append(TreeVariant(s));
	}

	return *this;
}

bool TreeVariant::operator==(const TreeVariant &a) const
{
	bool typeEquals = _type == a._type ;
	bool listRquals =_type != List || _list == a._list;
	bool mapEquals = _type != Map || _map == a._map ;
	bool varEquals = _type != Scalar || QVariant::operator ==(a);

	return typeEquals && listRquals &&  mapEquals && varEquals;
}

bool TreeVariant::operator!=(const TreeVariant &a) const
{
	return _type != a._type || _list != a._list || _map != a._map || QVariant::operator !=(a);
}
QStringList TreeVariant::hints =QStringList() << ""<< "bool"<< "byte"<< "int16"<< "int"<< "int64"<< "string"<< "float"<< "double";
void TreeVariant::setTypeHint(QString hint)
{
	setTypeHint(getTypeHintByName(hint));
}

void TreeVariant::setTypeHintKeys(QString hint)
{
	setTypeHintKeys(getTypeHintByName(hint));
}

void TreeVariant::setTypeHint(TreeVariant::TypeHint hint)
{
	_typeHint = hint;
}

void TreeVariant::setTypeHintKeys(TreeVariant::TypeHint hint)
{
	_typeHintKeys = hint;
}

TreeVariant::TypeHint TreeVariant::getTypeHintByName(QString hint)
{
	TreeVariant::TypeHint typeHint = TypeHint(hints.indexOf(hint));
	if (typeHint < 0) typeHint =TH_none;
	return typeHint;
}

QString TreeVariant::getJoinedValues(const QStringList &keys, const QString &glue) const
{
	QStringList parts;
	foreach (const QString &key, keys)
		parts << _map.value(key).toString();
	return parts.join(glue);
}

QString TreeVariant::getJoinedValues(const QList<int> &keys, const QString &glue) const
{
	QStringList parts;
	foreach (int key, keys)
		parts << _list.value(key).toString();
	return parts.join(glue);
}

QString TreeVariant::escape(QString str) const
{
	static const QLatin1String f1("\\\""), f2("\\r"), f3("\\n"), f4("\\\\");
	if (str.size() < 25 && QRegExp("[0-9A-Za-z_]+").exactMatch(str)) return str;
	return QString("\"") + str.replace('\\',f4).replace('\"',f1).replace('\r',f2).replace('\n',f3) + "\"";
}

QString TreeVariant::unescape(QString str) const
{
	static const QLatin1String f1("\\\""),t1("\""), f2("\\r"),t2("\r"), f3("\\n"), t3("\n"), f4("\\\\"), t4("\\");
	return str.trimmed().replace(f1, t1).replace(f2, t2).replace(f3, t3).replace(f4, t4);
}
