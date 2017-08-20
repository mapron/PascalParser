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

#include <QVariant>
#include <QStringList>
#include <QMap>
#include <QDataStream>

/**
*  \brief TreeVariant extends QVariant interface, allowing adding lists and maps and getting NON-CONST references to them.
*  Also, operator [] is added for map and list easy access.
*/
class TreeVariant : public QVariant
{
public:
	using TreeVariantMap = QMap<QString, TreeVariant>;
	using TreeVariantList = QList<TreeVariant>;

	static const TreeVariant emptyValue;
	static QStringList hints;
	   enum TypeHint {TH_none, TH_bool, TH_byte, TH_int16, TH_int, TH_int64, TH_string, TH_float, TH_double };
	enum VariantType {Scalar, List, Map};
	explicit TreeVariant();
	inline TreeVariant(const QVariant &variant) : QVariant(variant) {}
	inline TreeVariant(const QString &variant) : QVariant(variant) {}
	explicit TreeVariant(const bool &variant) : QVariant(variant) {}
	explicit TreeVariant(const int &variant) : QVariant(variant) {}
	TreeVariant(const char* variant) : QVariant(variant) {}
	TreeVariant(const QString& val, bool isMap) : QVariant(val) { _type = isMap ? Map : List;}

	/* types and conversion */
	TreeVariantList& asList() ;
	TreeVariantMap& asMap() ;
	const TreeVariantList& asList() const;
	const TreeVariantMap& asMap() const;
	VariantType vtype() const;
	void setVtype(VariantType vtype) ;
	static TreeVariant fromQVariant(QVariant val);
	QMap<QString,QString> asStringMap() const;
	QVariantMap asVariantMap() const;
	QStringList asStringList() const;
	QVariant asVariant() const;

	/* container access */
	TreeVariant& operator[] (const QString& index) ;
	const TreeVariant operator [](const QString& index) const;
	TreeVariant& operator[] (const int index);
	const TreeVariant &operator[] (const int index) const;

	int listSize() const;

	int mapSize() const;

	bool contains(const QString &key) const;

	TreeVariant* findBy(QString field, TreeVariant value);

	inline bool fetchIfContains(const QString &key, QVariant &val) const;
	inline bool fetchIfContains(const QString &key, bool &val) const;
	inline bool fetchIfContains(const QString &key, QString &val, QString def="") const;
	inline bool fetchIfContains(const QString &key, int &val) const;
	inline bool fetchIfContains(const QString &key, double &val) const;
	inline bool fetchIfContains(const QString &key, QStringList &val) const;

	QStringList keys() const;


	/* manipulation */

	void remove(const QString &key);
	void append(const TreeVariant &val);
	void addLists(QString val1="",QString val2="",QString val3="",QString val4="",QString val5="") ;
	void addMaps(QString val1="",QString val2="",QString val3="",QString val4="",QString val5="") ;
	/*! \brief Unites two TreeVarinat, replacing values recursively. */
	void merge(const TreeVariant& overwrite,const QStringList& except=QStringList(), bool owerwrite_lists=true, bool trunc_lists=false);

	void clear();


	/* input-output */

	bool saveToFile(QString filename, QString type="extensionGuess", QDataStream::ByteOrder byteOrder= QDataStream::BigEndian) const;
	bool saveToDevice(QIODevice* device, QString type, QDataStream::ByteOrder byteOrder= QDataStream::BigEndian) const;
	bool saveToByteArray(QByteArray& buffer, QString type, QDataStream::ByteOrder byteOrder= QDataStream::BigEndian) const;


	friend QDebug operator<<(QDebug dbg, const TreeVariant &c);
	friend QDebug operator<<(QDebug dbg, const TreeVariant::VariantType &c);

	const TreeVariant& operator = (const QStringList a);
	bool operator== (const TreeVariant& a) const;
	bool operator!= (const TreeVariant& a) const;

	void setTypeHint(QString hint);
	void setTypeHintKeys(QString hint);

	void setTypeHint(TypeHint hint);
	void setTypeHintKeys(TypeHint hint);

	static TypeHint getTypeHintByName(QString hint);
	QString getJoinedValues(const QStringList &keys, const QString &glue) const;
	QString getJoinedValues(const QList<int> &keys, const QString &glue) const;
	bool toYamlString(QString &ret,bool json = false, int level=0) const;

	/// Removes null-values in maps.
	void optimize();
	bool isZero();

protected:
	QString escape(QString str) const;
	QString unescape(QString str) const;

	void toUbjson(QDataStream& ds) const;

	bool isFlatList()  const;
	TreeVariantList _list;
	TreeVariantMap _map;
	QStringList _mapKeys;
	VariantType _type = Scalar;
	TypeHint _typeHint = TH_none;
	TypeHint _typeHintKeys = TH_none;
};


inline bool TreeVariant::fetchIfContains(const QString &key, QVariant &val) const {
	if (_map.contains(key)) {val = _map[key];return true;}
	return false;
}
inline bool TreeVariant::fetchIfContains(const QString &key, bool &val)  const {
	if (_map.contains(key)) {val = _map[key].toBool();return true;}
	return false;
}
inline bool TreeVariant::fetchIfContains(const QString &key, QString &val, QString def) const {
	if (_map.contains(key)) {val = _map[key].toString();return true;}
	else if (!def.isEmpty()) val = def;
	return false;
}
inline bool TreeVariant::fetchIfContains(const QString &key, int &val)  const{
	if (_map.contains(key)) {val = _map[key].toInt();return true;}
	return false;
}
inline bool TreeVariant::fetchIfContains(const QString &key, double &val)  const{

	if (!_map.contains(key)) return false;
	QString d = _map[key].toString();
	d.replace(".", ",");
	val = d.toDouble();
	return true;
}
inline bool TreeVariant::fetchIfContains(const QString &key, QStringList &val)  const{
	if (_map.contains(key)) {val = _map[key].asStringList();return true;}
	return false;
}
