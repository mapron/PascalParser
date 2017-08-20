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
#include "ast.h"
#include <TreeVariant.h>
#include <stdexcept>
#include <QSet>

AST::CodeLocation::CodeLocation(): _file(-1),_line(-1),_col(-1),_offset(-1) {}

AST::CodeLocation::CodeLocation(int l, int c, int o): _file(-1),_line(l),_col(c),_offset(o) {}

AST::CodeLocation::CodeLocation(int f, int l, int c, int o):_file(f),_line(l),_col(c),_offset(o)
{

}

bool AST::CodeLocation::operator <(const AST::CodeLocation &another) const
{
	if (_file < another._file) return true;
	if (_file > another._file) return false;
	if (_line < another._line) return true;
	if (_line > another._line) return false;
	return _col < another._col;
}

QString AST::CodeLocation::toString() const { return QString("%1:%2").arg(_line).arg(_col);}


AST::CodeMessage::CodeMessage() {}

AST::CodeMessage::CodeMessage(QString message) : _message(message), _type(Error) {}

AST::CodeMessage::CodeMessage(int line, int col, QString message) : _loc(line, col), _message(message), _type(Error) {}

AST::CodeMessage::CodeMessage(AST::CodeMessage::Type type, int line, int col, QString message) : _loc(line, col), _message(message), _type(type) {}

AST::CodeMessage::CodeMessage(AST::CodeMessage::Type type, QString message) : _message(message), _type(type) {}

AST::CodeMessage::CodeMessage(AST::CodeMessage::Type type, const AST::CodeLocation &loc, QString message) : _loc(loc),_message(message), _type(type) {}

QString AST::CodeMessage::toString(bool useType) const
{
	QString type;
	if (useType && _type == Error) type = "Error: ";
	if (useType && _type == Warning) type = "Warning: ";
	if (useType && _type == Info) type = "Info: ";
	return  type + _loc.toString()+": " +_message;
}


void AST::CodeMessages::clear() {
	errorsCount = 0;
	warningsCount= 0;
	_messages.clear();
	_texts.clear();
}

void AST::CodeMessages::Error(int line, int col, const wchar_t *s) {
	Add(CodeMessage(line,col, QString::fromWCharArray(s) ));
}

void AST::CodeMessages::Warning(int line, int col, const wchar_t *s) {
	Add(CodeMessage(CodeMessage::Warning, line,col, QString::fromWCharArray(s) ));
}

void AST::CodeMessages::Error(int line, int col, const QString &s) {
	Add(CodeMessage(line,col, s ));
}

void AST::CodeMessages::Error(const AST::CodeLocation &loc, const QString &s) {
	Add(CodeMessage(CodeMessage::Error,loc, s ));
}

void AST::CodeMessages::Warning(int line, int col, const QString &s) {
	Add(CodeMessage(CodeMessage::Warning, line,col, s ));
}

void AST::CodeMessages::Warning(const AST::CodeLocation &loc, const QString &s) {
	Add(CodeMessage(CodeMessage::Warning,loc, s ));
}

void AST::CodeMessages::Warning(const wchar_t *s) {
	Add(CodeMessage(CodeMessage::Warning, QString::fromWCharArray(s) ));

}

void AST::CodeMessages::Info(const AST::CodeLocation &loc, const QString &s) {
	Add(CodeMessage(CodeMessage::Info, loc, s ));
}

void AST::CodeMessages::Add(const AST::CodeMessage &msg)
{
	if (_texts[msg._loc].contains(msg._message)) return;
	if (msg._type == CodeMessage::Warning) warningsCount++;
	if (msg._type == CodeMessage::Error) errorsCount++;
	_texts[msg._loc] << msg._message;
	_messages << msg;

}
//====================================================================

#define EMPTY_SERIALIZATION(clas) \
	void AST::clas::serialize(TreeVariant &data, SerializationDirection direction)\
	{\
	 throw std::runtime_error(#clas " not defined");\
	}


template <class T>
static inline void setVarValue(T& var, const TreeVariant &data){
	 var = data.value<T>();
}

#define SR_PRIMITIVE(field) \
	if (direction & sdSave) \
	 data[#field].setValue( this->field );\
	else \
	 setVarValue(field, data[#field])

#define SR_PRIMITIVE_ENUM(field, type) \
	if (direction & sdSave) \
	 data[#field].setValue( (int)this->field );\
	else \
	 field = type(data[#field].toInt())

#define SR_AST(field) \
	this->field.serialize(data[#field], direction)

#define SR_FIELD_LIST(field, type) \
	if (direction & sdSave){ \
	  for (int field##i = 0; field##i < field.size(); field##i++) {\
		TreeVariant data##field;\
		field[field##i].serialize(data##field, direction);\
		data[#field].append(data##field);\
	  }\
	}else{ \
	for (int field##i = 0; field##i < data[#field].listSize(); field##i++) {\
	  type child;\
	  child.serialize(data[#field][field##i], direction);\
	  field << child;\
	}\
	}

#define SR_FIELD_SWITCH(fieldName) \
	int typeIndex##fieldName = fieldName.which(); \
	if (direction & sdLoad) {\
		typeIndex##fieldName = typeNames.indexOf(data[#fieldName "_type" ].toString());\
	}

#define SR_FIELD_SWCASE(fieldName, typeName) \
	if (typeIndex##fieldName == t_##typeName) {\
		if (direction & sdSave) {\
			boost::get<AST::typeName>( fieldName ).serialize(data[#fieldName], direction);\
			data[#fieldName "_type" ] = #typeName;\
		}else{\
			AST::typeName v;\
			v.serialize(data[#fieldName], direction);\
			fieldName = v;\
		}\
	}


#define SR_FIELD_SWCASE_PRIMITIVE(fieldName, typeName) \
	if (typeIndex##fieldName == t_##typeName) {\
		if (direction & sdSave) {\
			data[#fieldName ].setValue(boost::get<typeName>( fieldName ) );\
			data[#fieldName "_type" ] = #typeName;\
		}else{\
			typeName v;\
			setVarValue(v, data[#fieldName]);\
			fieldName = v;\
		}\
	}



#define SERIALIZE_BASE \
	 AST::_node::serialize(data, direction)


//====================================================================


void AST::_node::serialize(TreeVariant &data, SerializationDirection direction)
{
	if (direction & sdSave){
		if (_flags != 0) {
			data["_flags"].setValue(_flags);
		}
		if (direction & sdLocation){
			if (_loc._line != -1) {
				data["l"].setValue(_loc._line);
			}
			if (_loc._col != -1) {
				data["c"].setValue(_loc._col);
			}
		}
	}
	else{
		data.fetchIfContains("l", _loc._line);
		data.fetchIfContains("c", _loc._col);
		data.fetchIfContains("_flags", _flags);
	}
}

TreeVariant AST::_node::save(bool saveLocation)
{
	TreeVariant data;
	this->serialize(data, saveLocation ? SerializationDirection(sdSave | sdLocation) :  sdSave);
	data.optimize();
	return data;
}

void AST::_node::load(const TreeVariant &data)
{
	 this->serialize(const_cast<TreeVariant&>(data), sdLoad);
}

void AST::ident::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	if (direction & sdSave) {
		if (!_ident.isEmpty())
			data["_ident"].setValue( _ident );
	}else
	 setVarValue(_ident, data["_ident"]);

}


void AST::expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "primary" << "unary" << "binary";
	SR_FIELD_SWITCH(_expr);
	SR_FIELD_SWCASE(_expr, primary)
	SR_FIELD_SWCASE(_expr, unary)
	SR_FIELD_SWCASE(_expr, binary)
}
void AST::expr_list::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_exprs, expr);
	SR_FIELD_LIST(_idents, ident);
}


EMPTY_SERIALIZATION(set_construct)
void AST::constant::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "bool" << "int64_t" << "double"<< "QString"<< "set_construct";
	SR_FIELD_SWITCH(_constant);
	SR_FIELD_SWCASE_PRIMITIVE(_constant, bool)
	SR_FIELD_SWCASE_PRIMITIVE(_constant, int64_t)
	SR_FIELD_SWCASE_PRIMITIVE(_constant, double)
	SR_FIELD_SWCASE_PRIMITIVE(_constant, QString)
	SR_FIELD_SWCASE(_constant, set_construct)
}

void AST::deref_expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
}
void AST::address_expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
}
void AST::idx_expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_indexes);
}
void AST::access_expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_field);
}
void AST::call_expr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_args);
}
void AST::primary_accessor::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "address_expr" << "call_expr" << "deref_expr"<< "idx_expr"<< "access_expr";
	SR_FIELD_SWITCH(_primary_accessor);
	SR_FIELD_SWCASE(_primary_accessor, address_expr);
	SR_FIELD_SWCASE(_primary_accessor, call_expr);
	SR_FIELD_SWCASE(_primary_accessor, deref_expr);
	SR_FIELD_SWCASE(_primary_accessor, idx_expr);
	SR_FIELD_SWCASE(_primary_accessor, access_expr);
}
void AST::internalexpr::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_PRIMITIVE_ENUM(_type, Type);
	SR_AST(_expr_list);
}

void AST::primary::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "ident" << "expr_list" << "constant"<< "internalexpr";
	SR_FIELD_SWITCH(_expr);
	SR_FIELD_SWCASE(_expr, ident)
	SR_FIELD_SWCASE(_expr, expr_list)
	SR_FIELD_SWCASE(_expr, constant)
	SR_FIELD_SWCASE(_expr, internalexpr)

	SR_FIELD_LIST(_accessors, primary_accessor);
}

void AST::unary::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr);
	SR_PRIMITIVE_ENUM(_op, BytecodeVM::UnOp);
}
void AST::binary::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_left);
	SR_AST(_right);
	SR_PRIMITIVE_ENUM(_op, BytecodeVM::BinOp);
}
void AST::annotation::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr_list);
}
//--------------------------
void AST::internalfst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_PRIMITIVE_ENUM(_type, Type);
}
void AST::statement::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "compoundst"   << "assignmentst"   << "gotost"  << "switchst"  << "ifst"
													  << "forst"        << "whilest"        << "repeatst"<< "procst"    << "withst"
													  << "labelst"      << "tryst"          << "writest" << "readst"    << "strst"
													  << "raisest"      << "inheritedst"    << "onst"    << "internalfst"
														 ;
	SR_FIELD_SWITCH(_statement);
	SR_FIELD_SWCASE(_statement, compoundst)
	SR_FIELD_SWCASE(_statement, assignmentst)
	SR_FIELD_SWCASE(_statement, gotost)
	SR_FIELD_SWCASE(_statement, switchst)
	SR_FIELD_SWCASE(_statement, ifst)

	SR_FIELD_SWCASE(_statement, forst)
	SR_FIELD_SWCASE(_statement, whilest)
	SR_FIELD_SWCASE(_statement, repeatst)
	SR_FIELD_SWCASE(_statement, procst)
	SR_FIELD_SWCASE(_statement, withst)

	SR_FIELD_SWCASE(_statement, labelst)
	SR_FIELD_SWCASE(_statement, tryst)
	SR_FIELD_SWCASE(_statement, writest)
	SR_FIELD_SWCASE(_statement, readst)
	SR_FIELD_SWCASE(_statement, strst)

	SR_FIELD_SWCASE(_statement, raisest)
	SR_FIELD_SWCASE(_statement, inheritedst)
	SR_FIELD_SWCASE(_statement, onst)
	SR_FIELD_SWCASE(_statement, internalfst)
}
void AST::sequence::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_statements, statement);
}
void AST::compoundst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_sequence);
}
void AST::assignmentst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_left);
	SR_AST(_right);
}
void AST::gotost::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_PRIMITIVE(_iconst);
}
void AST::labelst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
}
void AST::case_list_elem::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr_list);
	SR_AST(_statement);
}
void AST::switchst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_case);
	SR_FIELD_LIST(_case_list, case_list_elem);
	SR_AST(_else);
}
void AST::ifst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr);
	SR_AST(_if);
	SR_AST(_else);
}
void AST::forst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
	SR_PRIMITIVE(_downto);
	SR_AST(_fromExpr);
	SR_AST(_toExpr);
	SR_AST(_statement);
}
void AST::whilest::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr);
	SR_AST(_statement);
}
void AST::repeatst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr);
	SR_AST(_sequence);
}

void AST::procst::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_primary);
}
EMPTY_SERIALIZATION(withst)
EMPTY_SERIALIZATION(tryst)

void AST::write_param::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_expr);
	SR_FIELD_LIST(_constants, constant);
}
void AST::write_list::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_write_params, write_param);
}
void AST::writest::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_PRIMITIVE(_isLn);
	SR_AST(_write_list);
}
EMPTY_SERIALIZATION(strst)
EMPTY_SERIALIZATION(readst)
EMPTY_SERIALIZATION(raisest)
EMPTY_SERIALIZATION(inheritedst)
EMPTY_SERIALIZATION(onst)
EMPTY_SERIALIZATION(label_decl_part)

void AST::type::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "simple_type" << "array_type"      << "set_type"      << "file_type"
													 << "pointer_type" << "subrange_type"   << "enum_type"  << "function_type" << "class_type"
												   ;
	SR_FIELD_SWITCH(_type);
	SR_FIELD_SWCASE(_type, simple_type)
	SR_FIELD_SWCASE(_type, array_type)
	SR_FIELD_SWCASE(_type, set_type)
	SR_FIELD_SWCASE(_type, file_type)

	SR_FIELD_SWCASE(_type, pointer_type)
	SR_FIELD_SWCASE(_type, subrange_type)
	SR_FIELD_SWCASE(_type, enum_type)
	SR_FIELD_SWCASE(_type, function_type)
	SR_FIELD_SWCASE(_type, class_type)
}
void AST::simple_type::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
}
void AST::array_range::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_PRIMITIVE(_hasLow);
	SR_AST(_boundLow);
	SR_AST(_boundHigh);
}
void AST::array_type::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_array_ranges, array_range);
	SR_AST(_type);
}
EMPTY_SERIALIZATION(set_type)
EMPTY_SERIALIZATION(file_type)
EMPTY_SERIALIZATION(pointer_type)
EMPTY_SERIALIZATION(subrange_type)
EMPTY_SERIALIZATION(enum_type)
EMPTY_SERIALIZATION(function_type)
void AST::const_def::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
	SR_AST(_type);
	SR_AST(_initializer);
}
void AST::var_decl::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_idents, ident);
	SR_AST(_type);
	SR_AST(_initializer);
}
void AST::const_def_part::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_const_defs, const_def);
}
void AST::type_def::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
	SR_AST(_type);
}
void AST::type_def_part::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_type_defs, type_def);
}
void AST::var_decl_part::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_var_decls, var_decl);
}
void AST::formal_param::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList typeNames = QStringList() << "" << "var_decl" << "proc_decl"      ;
	SR_FIELD_SWITCH(_formal_param);
	SR_FIELD_SWCASE(_formal_param, var_decl);
	SR_FIELD_SWCASE(_formal_param, proc_decl);
}
void AST::proc_decl::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
	SR_AST(_class);
	SR_AST(_type);
	SR_FIELD_LIST(_formal_params, formal_param);
}

EMPTY_SERIALIZATION(unit_spec)
EMPTY_SERIALIZATION(uses_part)
void AST::decl_part::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "label_decl_part" << "const_def_part" <<"type_def_part"
												<< "var_decl_part"  << "proc_def"<< "unit_spec"<< "uses_part";
	SR_FIELD_SWITCH(_decl_part);
	SR_FIELD_SWCASE(_decl_part, label_decl_part)
	SR_FIELD_SWCASE(_decl_part, const_def_part)
	SR_FIELD_SWCASE(_decl_part, type_def_part)
	SR_FIELD_SWCASE(_decl_part, var_decl_part)
	SR_FIELD_SWCASE(_decl_part, proc_def)
	SR_FIELD_SWCASE(_decl_part, unit_spec)
	SR_FIELD_SWCASE(_decl_part, uses_part)
}
void AST::decl_part_list::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_parts, decl_part);
}
void AST::class_section::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_FIELD_LIST(_var_decls, var_decl);
	SR_FIELD_LIST(_proc_fwd_decls, proc_def);
}
void AST::class_type::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_parent);
	SR_FIELD_LIST(_sections, class_section);
	SR_AST(_annotation);
}
void AST::block::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_decl_part_list);
	SR_AST(_compoundst);
}
void AST::proc_def::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_proc_decl);
	SR_AST(_block);
	SR_PRIMITIVE(_isForward);
}


void AST::program::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_ident);
	SR_AST(_block);

}

EMPTY_SERIALIZATION(unit)
void AST::stProgram::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	SR_AST(_decl_part_list);
	SR_AST(_program);
	SR_PRIMITIVE(_hasProgram);

}
void AST::pascalSource::serialize(TreeVariant &data, SerializationDirection direction)
{
	SERIALIZE_BASE;
	static const QStringList  typeNames = QStringList() << "" << "program" << "unit" <<"stProgram";
	SR_FIELD_SWITCH(_pascal);
	SR_FIELD_SWCASE(_pascal, program)
	SR_FIELD_SWCASE(_pascal, unit)
	SR_FIELD_SWCASE(_pascal, stProgram)

}
