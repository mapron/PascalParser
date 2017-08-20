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

#include "StringVisitor.h"

#include <QMap>
#include <QSet>
// =============================== PRIMITIVES =============================================

QString StringVisitor::operator ()(const bool &val) const
{
	return val ? "true" : "false";
}

QString StringVisitor::operator ()(const int64_t &val) const
{
	return QString::number(val);
}

QString StringVisitor::operator ()(const double &val) const
{
	return QString::number(val, 'g', 15);
}

QString StringVisitor::operator ()(const QString &val) const
{
	QString ret = val;
	ret.replace("'", "\\'");
	ret.replace("\"", "\\\"");
	if (_type == otC) {
		ret.replace("\t", "\\t");
		ret.replace("\r", "\\r");
		ret.replace("\n", "\\n");
		ret = "\"" + ret + "\"";

	}
	if (_type == otPascal) {
		ret.replace("\t", "'#9'");
		ret.replace("\r", "'#13'");
		ret.replace("\n", "'#10'");
		ret = "'" + ret + "'";
	}
	return ret;
}

QString StringVisitor::operator ()(const AST::set_construct &val) const
{
	return "[" + (*this)(val._expr_list) + "]";
}

QString StringVisitor::operator()(const AST::ident &val) const
{
	static QMap<QString, QString> lowerToReal = []() ->QMap<QString, QString> {
		QMap<QString, QString> r;
		r["integer"]="int";
		r["real"]="double";
		r["double"]="double";
		r["boolean"]="bool";
		r["string"]="std::string";
		r["result"]="Result";
		r["nil"]="NULL";
		return r;
	}();
	if (_type == otC){
		QString lowerIdent = val._ident.toLower();
		if (lowerToReal.contains(lowerIdent)){
			return lowerToReal.value(lowerIdent);
		}else{
			lowerToReal[lowerIdent]=val._ident;
		}
	}

	return val._ident;
}

QString StringVisitor::operator ()(const AST::idents &val) const
{
	QStringList parts;
	foreach (const AST::ident &i, val)
		parts << (*this)(i);
	return parts.join(", ");
}

// =============================== EXPRESSIONS =============================================

QString StringVisitor::operator()(const AST::expr &val) const
{
	if (val._expr.which() == 0) return "?expr?";
	return boost::apply_visitor((*this), val._expr);
}


QString StringVisitor::operator()(const AST::expr_list &val) const
{
	QStringList parts;
	foreach (const AST::expr& p, val._exprs)
		parts << (*this)(p);
	return (_type == otC && _isDeclaration ? "{ " : "( ") + parts.join(", ") + (_type == otC && _isDeclaration ? " }" : " )");
}


QString StringVisitor::operator()(const AST::primary &val) const
{
	if (val._expr.which() == 0) return "?primary?";
	QString ret = boost::apply_visitor((*this), val._expr);
	foreach (const AST::primary_accessor &p, val._accessors){
		if (p._primary_accessor.which() == AST::primary_accessor::t_address_expr){
			if (_type == otC){
				ret = "&("+ret +")";
			}else{
				ret = "@"+ret;
			}
			continue;
		}

		if (_type == otC){
			if (p._primary_accessor.which() == AST::primary_accessor::t_deref_expr){
				ret = "(*"+ret +")";
				continue;
			}
		}
		ret += (*this)(p);
	}
	return ret;
}

QString StringVisitor::operator ()(const AST::primary_accessor &val) const
{
	if (val._primary_accessor.which() == 0) return "?primary_accessor?";
	QString ret = boost::apply_visitor((*this), val._primary_accessor);
	return ret;
}


QString StringVisitor::operator()(const AST::unary &val) const
{
	static QStringList ops = QStringList()
			<< "+"
			<< "-"
			<< "not"
			<< "~"
			<< "++"
			<< "--"
			   ;

	 QString op = ops.value(val._op, QString("?(%1)").arg(val._op));
	return op + (*this)(val._expr);
}
QString StringVisitor::operator()(const AST::binary &val) const
{
	QStringList ops = QStringList()
			<< "+"
			<< "-"
			<< "mod"
			<< "*"
			<< "div"
			<< "/"

			<< "and" //6
			<< "shl"
			<< "shr"
			<< "or"
			<< "xor"

			 << "and"
			 << "or"

			<< ">"//13
			<< "<"
			<< ">="
			<< "<="
			<< "="//17
			<< "<>"
			<< "in";
	if (_type == otC){
		ops[2]="%";
		ops[4]="/";

		ops[6]="&";
		ops[7]="<<";
		ops[8]=">>";
		ops[9]="|";
		ops[10]="^";

		ops[11]="&&";
		ops[12]="||";

		ops[17]="==";
		ops[18]="!=";
	}
	QString left = (*this)(val._left);
	QString right = (*this)(val._right);
	QString op = ops.value(val._op, QString("?(%1)").arg(val._op));

	return left + QString(" %1 ").arg(op) + right;
}

QString StringVisitor::operator()(const AST::call_expr &val) const
{
	return  (*this)(val._args);
}

QString StringVisitor::operator()(const AST::constant &val) const
{
	if (!val._constant.which()) return "?constant?";
	return boost::apply_visitor(*this, val._constant);

}

QString StringVisitor::operator ()(const AST::deref_expr &val) const
{
	return (_type == otC ? "*" : "^");
}

QString StringVisitor::operator ()(const AST::address_expr &val) const
{
	return "";
}

QString StringVisitor::operator ()(const AST::idx_expr &val) const
{
	if (_type == otC){
		QStringList parts;
		foreach (const AST::expr& expr, val._indexes._exprs){
			parts << "[" +  (*this)(expr) +"]" ;
		}
		return parts.join("");
	}
	return "[" +  (*this)(val._indexes) +"]" ;
}

QString StringVisitor::operator ()(const AST::access_expr &val) const
{
	return "." +  (*this)(val._field)  ;
}

QString StringVisitor::operator ()(const AST::internalexpr &val) const
{
	if (val._type == AST::internalexpr::tHigh){
		return "High" + (*this)(val._expr_list);
	}
	if (val._type == AST::internalexpr::tLow){
		return "Low" + (*this)(val._expr_list);
	}
	if (val._type == AST::internalexpr::tInc){
		return "Inc" + (*this)(val._expr_list);
	}
	return "?internalexpr?";
}

// =============================== STATEMENTS  =============================================
QString StringVisitor::operator()(const AST::statement &val) const
{
	static QSet<AST::statement::Type> cIgnoredSimicolon { AST::statement::t_compoundst,
				AST::statement::t_forst,
				AST::statement::t_ifst,
				AST::statement::t_switchst,
				AST::statement::t_whilest,
				AST::statement::t_withst };
	return boost::apply_visitor(*this, val._statement) +
			(_type == otC && !cIgnoredSimicolon.contains((AST::statement::Type)val._statement.which()) ? ";" : "" );
}


QString StringVisitor::operator()(const AST::compoundst &val) const
{
	QStringList out;


	out << (_type == otPascal ? "begin" : "{");
   _level++;
	out << (*this)(val._sequence);
	_level--;
	out << idn() + (_type == otPascal ? "end" : "}");

	return out.join("\r\n");
}

QString StringVisitor::operator()(const AST::assignmentst &val) const
{
	QString left = (*this)(val._left);
	QString let = _type == otC ? "=" : ":=";
	QString right = (*this)(val._right);
	if (left.toLower() == _currentFunction) left = "Result";
	return left + " " + let +" " + right ;
}
QString StringVisitor::operator ()(const AST::gotost &val) const
{
	return "goto " + QString::number(val._iconst);
}

QString StringVisitor::operator ()(const AST::switchst &val) const
{
	QStringList parts;
	parts << "CASE " + (*this)(val._case) + " OF ";
	_level++;
	foreach (const AST::case_list_elem &elem, val._case_list){
		parts << idn() + (*this)(elem._expr_list) +  ":";
		_level++;
		parts << idn() + (*this)(elem._statement);
		_level--;
	}
	_level--;
	parts<< idn() + "END";
	return parts.join("\r\n");
}

QString StringVisitor::operator ()(const AST::ifst &val) const
{

	QStringList ret;
	if (_type == otC)
		ret << kw("IF (") + (*this)(val._expr)  + ") ";
	else
		ret << kw("IF ") + (*this)(val._expr)  + " THEN ";
	_level++;
	ret << idn() + (*this)(val._if) ;
	_level--;
	if (val._else._statement.which()){
		ret << idn() + kw("ELSE ");
		_level++;
		ret << idn() +(*this)(val._else) ;
		_level--;
	}
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::forst &val) const
{
	QStringList ret;
	if (_type == otC){
		QString incType = val._downto ? "--" : "++";
		QString cmpType = val._downto ? ">=" : "<=";
		QString i = (*this)(val._ident);
		ret << "for (int " + i  + " = " + (*this)(val._fromExpr) + "; "
			   + i + cmpType   + (*this)(val._toExpr) + "; "
			   + i + incType+") ";
	}else{
		QString incType = val._downto ? " DOWNTO " : " TO ";
		ret << "FOR " + (*this)(val._ident) + " = " + (*this)(val._fromExpr) + incType + (*this)(val._toExpr) + " DO ";
	}
	_level++;
	ret << idn() + (*this)(val._statement) ;
	_level--;
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::whilest &val) const
{
	QStringList ret;
	ret << "WHILE " + (*this)(val._expr) + " DO ";
	_level++;
	ret << idn() + (*this)(val._statement) ;
	_level--;
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::repeatst &val) const
{
	QStringList ret;
	ret << "REPEAT ";
	_level++;
	ret << idn() + (*this)(val._sequence) ;
	_level--;
	ret << "UNTIL " + (*this)(val._expr);
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::procst &val) const
{
	return (*this)(val._primary);
}

QString StringVisitor::operator ()(const AST::withst &val) const
{
	if (_type == otC)
	{
		QStringList ret;

		PTypeDef exprType = _codeTypes[val._expr._loc].getType();
		QString ln = "_l" + QString::number(_level);
		ret << "{ auto &"+ ln + " = "  + (*this)(val._expr)  + ";";
		QStringList fields = exprType ? exprType->_childNamesArray : QStringList() ;
		foreach (QString alias, fields)
			ret << idn() +  " auto &"+ alias + " = " +ln + "." + alias + ";";

		_level++;
		ret << idn() + (*this)(val._statement) ;
		_level--;
		 ret << idn() + "}";
		return ret.join("\r\n");
	}

	QStringList ret;
	ret << "WITH " + (*this)(val._expr) + " DO ";
	_level++;
	ret << idn() + (*this)(val._statement) ;
	_level--;
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::labelst &val) const
{
	return "LABEL " +(*this)(val._ident) ;
}

QString StringVisitor::operator ()(const AST::tryst &val) const
{
	if (_type == otC) {
		QStringList ret;
		ret << "try { " ;
		_level++;
		ret << (*this)(val._try_) ;
		_level--;
		ret << idn() +"}";
		if (val._flags & AST::tryst::HasExcept){
			ret << idn() +"catch (...) { /*";
			 _level++;
			_type = otPascal;
			ret << (*this)(val._except_) ;
			_type = otC;
			_level--;
			ret << idn() + " */ }";
		}
		if (val._flags & AST::tryst::HasFinally){
			ret << idn() + "/* FINALLY */ ";
			 _level++;
			ret << (*this)(val._finally_) ;
			_level--;
			ret << idn() + "/* / FINALLY */ ";
		}
		return ret.join("\r\n");
	}

	QStringList ret;
	ret << "TRY " ;
	_level++;
	ret << idn() + (*this)(val._try_) ;
	_level--;
	if (val._flags & AST::tryst::HasExcept){
		ret << "EXCEPT ";
		 _level++;
		ret << idn() + (*this)(val._except_) ;
		_level--;
	}
	if (val._flags & AST::tryst::HasFinally){
		ret << "FINALLY ";
		 _level++;
		ret << idn() + (*this)(val._finally_) ;
		_level--;
	}
	ret << "END";
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::writest &val) const
{
	QString cmd = val._isLn ? "WRITELN" :"WRITE";
	return cmd+" (" + (*this)(val._write_list) + ")";
}

QString StringVisitor::operator ()(const AST::readst &val) const
{
	QString cmd = val._isLn ? "READLN" :"READ";
	return cmd+" (" + (*this)(val._expr_list) + ")";
}

QString StringVisitor::operator ()(const AST::strst &val) const
{
	return "STR (" + (*this)(val._write_list) +")";
}

QString StringVisitor::operator ()(const AST::raisest &val) const
{
	return "RAISE " + (*this)(val._expr);
}

QString StringVisitor::operator ()(const AST::inheritedst &val) const
{
	return "INHERITED " + (*this)(val._ident);
}

QString StringVisitor::operator ()(const AST::onst &val) const
{
	QStringList ret;
	ret << "ON " + (*this)(val._ident) + ": " + (*this)(val._type.get())  + " DO ";
	_level++;
	ret << idn() + (*this)(val._statement) ;
	_level--;
	return ret.join("\r\n");
}

QString StringVisitor::operator ()(const AST::internalfst &val) const
{
	if (val._type == AST::internalfst::tBreak)
		return kw("BREAK");
	if (val._type == AST::internalfst::tContinue)
		return kw("CONTINUE");
	if (val._type == AST::internalfst::tExit){
		if (_type == otC){
			QString ret = "return ";
			if (_currentFunction.size()) ret += "Result";
			return ret;
		}
		return  kw("EXIT");
	}
	return "?controlst?";
}
QString StringVisitor::operator ()(const AST::write_list &val) const
{
	QStringList parts;
	foreach (const AST::write_param& p, val._write_params)
		parts << (*this)(p);
	return parts.join(", ");
}

QString StringVisitor::operator ()(const AST::write_param &val) const
{
	QStringList parts;
	parts << (*this)(val._expr);
	foreach (const AST::constant& p, val._constants)
		parts << (*this)(p);
	return  parts.join(":");
}

// ===============================    TYPES    =============================================
QString StringVisitor::operator ()(const AST::type &val) const
{
	if (val._type.which() == 0) return "?type?";
	return boost::apply_visitor((*this), val._type);
}

QString StringVisitor::operator ()(const AST::simple_type &val) const
{
	QString ident = (*this)(val._ident);
	return ident;
}

QString StringVisitor::operator ()(const AST::array_type &val) const
{
	QStringList indexes;
	foreach (const AST::array_range& range, val._array_ranges) {
		QString i;
		if (_type == otC)
			i ="[" + (!range._hasLow ? (*this)(range._boundHigh) : (*this)(range._boundHigh) + " - " + (*this)(range._boundLow)) + " + 1]";
		else
			i = (!range._hasLow ? (*this)(range._boundHigh) : (*this)(range._boundLow) + ".." + (*this)(range._boundHigh)) ;

		indexes << i;
	}

	QString indexes2 = indexes.size() ? "[" + indexes.join(", ") + "]" : "";
	if (_type == otC){
		if (indexes.size()){
			_typeSuffix = indexes.join("");
			return (*this)(val._type) + " ";
		}else{
			return "std::vector<" + (*this)(val._type) +">";
		}
	}
	return QString("ARRAY ") +indexes2+ " OF "  +(*this)(val._type);
}

QString StringVisitor::operator ()(const AST::set_type &val) const
{
	return "SET OF " + (*this)(val._type);
}

QString StringVisitor::operator ()(const AST::file_type &val) const
{
	return "FILE OF " + (*this)(val._type);
}

QString StringVisitor::operator ()(const AST::pointer_type &val) const
{
	return (_type == otC ? "" : "^") + (*this)(val._type) + (_type == otC ? "*" : "") ;
}

QString StringVisitor::operator ()(const AST::subrange_type &val) const
{
	return "?subrange_type?";
}

QString StringVisitor::operator ()(const AST::enum_type &val) const
{
	return "(" + (*this)(val._expr_list) +")";
}

QString StringVisitor::operator ()(const AST::function_type &val) const
{
	return "procedure";
}
struct Blah {};
typedef struct : public Blah { } Blah2 ;
QString StringVisitor::operator ()(const AST::class_type &val) const
{
	QStringList parts;
	QString parent;
	if (val._parent._ident.size())
		parent = _type == otC ? " : public " +(*this)(val._parent) :  "(" + (*this)(val._parent) + ")";
	if (_type == otC)
		parts << " struct " + parent + " { ";
	else
		parts << ((val._flags & AST::class_type::IsRecord)?  "RECORD " :  "CLASS ") + parent;
	QStringList classFields;
	for (const AST::class_section& sec: val._sections)
	{
		_level ++;
		for (const AST::var_decl& var_decl: sec._var_decls)
		{
			for (const auto & id : var_decl._idents)
				classFields << id._ident;
			parts << idn() + (*this)(var_decl) + (_type == otC ? ";" : "");
		}

		for (const AST::proc_def& proc_decl: sec._proc_fwd_decls)
			parts << idn() + (*this)(proc_decl) + (_type == otC ? ";" : "");
		_level --;
	}


	parts << idn() + (_type == otC ? "}" : "END");
	return parts.join("\r\n");
}

QString StringVisitor::operator ()(const AST::class_section &val) const
{
	return "";
}


// =============================== DECLARATIONS=============================================

QString StringVisitor::operator ()(const AST::decl_part_list &val) const
{
	QStringList parts;
	foreach (const AST::decl_part& p, val._parts)
		parts << (*this)(p);
	return parts.join("\r\n");
}

QString StringVisitor::operator ()(const AST::decl_part &val) const
{
	return boost::apply_visitor(*this, val._decl_part) + ";";
}

QString StringVisitor::operator ()(const AST::label_decl_part &val) const
{
	return idn() + "LABEL " +(*this)(val._idents);

}

QString StringVisitor::operator ()(const AST::const_def_part &val) const
{
	QStringList parts;
	_level++;
	foreach (const AST::const_def &i, val._const_defs){
		QString typexpr;
		if (i._type._type.which())
			typexpr = ":" + (*this)(i._type) ;
		parts << idn() + (_type == otC ? "const auto " : "") + (*this)(i._ident) + " = " + (*this)(i._initializer);
	}
	_level--;
	QString v = _type == otC ? "" : "CONST \r\n";
	return idn() + v + parts.join(";\r\n");
}

QString StringVisitor::operator ()(const AST::type_def_part &val) const
{
	QStringList parts;
	_level++;
	foreach (const AST::type_def &i, val._type_defs)
		parts << idn() +(*this)(i);
	_level--;
	QString v = _type == otC ? "" : "TYPE \r\n";
	return idn() + v + parts.join(";\r\n");
}

QString StringVisitor::operator ()(const AST::var_decl_part &val) const
{
	QStringList parts;
	_level++;
	foreach (const AST::var_decl &i, val._var_decls){
		parts << idn() +(*this)(i);
	}
	_level--;
	QString v = _type == otC ? "" : "VAR \r\n";
	return idn() + v + parts.join(";\r\n");
}

QString StringVisitor::operator ()(const AST::proc_def &val) const
{
	QString ret=  idn() + (*this)(val._proc_decl) ;
	if (!val._isForward){

		if (_type == otC && (val._proc_decl._flags & AST::proc_decl::IsFunction)){
			_currentFunction = val._proc_decl._ident._ident.toLower();
			_makeResult = (*this)(val._proc_decl._type);
		}
		ret += "\r\n" + idn() + (*this)(val._block);
		_currentFunction = "";
	}
	return ret;
}

QString StringVisitor::operator ()(const AST::unit_spec &val) const
{
	return "?";
}

QString StringVisitor::operator ()(const AST::uses_part &val) const
{
	QString rt = idn() + "USES " + (*this)(val._idents) ;
	if (_type == otC) rt = "//"+rt;
	return rt;
}

QString StringVisitor::operator ()(const AST::var_decl &val) const
{
	bool hasInit = val._initializer._expr.which() != 0;
	if (_type == otC){
		QString type = (*this)(val._type);
		QStringList parts;
		foreach (const AST::ident& ident, val._idents){
			parts << (*this)(ident)+_typeSuffix;
		}
		_typeSuffix = "";
		QString init;
		_isDeclaration = true;
		if (hasInit)
			init = " = " + (*this)(val._initializer);
		_isDeclaration = false;
		return  type + " " +parts.join(", ") + init;
	}
	return (*this)(val._idents) + " : " + (*this)(val._type) + (hasInit ?  " := " + (*this)(val._initializer) : "");
}

QString StringVisitor::operator ()(const AST::proc_decl &val) const
{
	if (_type == otC){
		bool isFunc = val._flags & AST::proc_decl::IsFunction;
	   QString ret = isFunc ? (*this)(val._type) : "void";
	   ret+= " " + (*this)(val._ident) ;

	   QStringList parts;
	   foreach (const AST::formal_param &i, val._formal_params){
		   const AST::var_decl* var_decl = boost::get<AST::var_decl>(&i._formal_param);
		   if (!var_decl) continue;
		   QString type = (*this)(var_decl->_type);
		   foreach (const AST::ident& ident, var_decl->_idents){

			   QString param = type;
			   if (i._flags & AST::formal_param::IsVar) param =  param+ "&";
			   if (i._flags & AST::formal_param::IsConst) param = "const " + param;
			   parts << param + " " + (*this)(ident);
		   }
	   }
	   ret += "(" + parts.join(", ") + ")";
	   return ret;
	}else {

		QString ret = "PROCEDURE ";
		bool isFunc = val._flags & AST::proc_decl::IsFunction;
		if (isFunc) ret = "FUNCTION ";
		if (val._flags & AST::proc_decl::IsConstructor)
			 ret = "CONSTRUCTOR ";
		if (val._flags & AST::proc_decl::IsDestructor)
			 ret = "DESTRUCTOR ";

		QStringList parts;
		foreach (const AST::formal_param &i, val._formal_params){
			QString param = boost::apply_visitor(*this,  i._formal_param);
			if (i._flags & AST::formal_param::IsVar) param = "VAR " + param;
			if (i._flags & AST::formal_param::IsConst) param = "CONST " + param;
			parts << param;
		}
		ret += (*this)(val._ident) + "(" + parts.join("; ") + ")";
		if (isFunc) ret+= ": " +(*this)(val._type);
		ret+=";";
		if (val._flags & AST::proc_decl::IsForward)
			 ret += "FORWARD;";
		if (val._flags & AST::proc_decl::IsOverload)
			 ret += "OVERLOAD;";
		if (val._flags & AST::proc_decl::IsOverride)
			 ret += "OVERRIDE;";
		return ret ;
   }
}
QString StringVisitor::operator ()(const AST::type_def &val) const
{
	if (_type == otC){
		return "typedef " + (*this)(val._type) + " " + (*this)(val._ident);
	}
	return (*this)(val._ident) + " = " + (*this)(val._type);
}

// ===============================  PROGRAM    =============================================
QString StringVisitor::operator ()(const AST::pascalSource &val) const
{
	return boost::apply_visitor(*this, val._pascal);
}

QString StringVisitor::operator()(const AST::program &val) const
{
	if (_type == otC)
	{
		QString commonDefines =
				"#define Low(x) 0   \r\n"
				"#define High(x) (sizeof(x)/sizeof(x[0])-1) \r\n"
				"#define Inc(x) ++x \r\n"
				"#define WRITELN(x) std::cout << x << std::endl \r\n"
				"#define sqr(x) ((x) * (x)) \r\n"
				"#include <cmath> \r\n"
				"#include <iostream> \r\n"
				;
		QString decls = (*this)(val._block._decl_part_list);
		QString st = (*this)(val._block._compoundst);

		return QString("%1\r\n%2\r\n int main()\r\n%3\r\n").arg(commonDefines).arg(decls).arg(st);
	}
	return QString("program %1;\r\n%2.")
			.arg((*this)(val._ident))
			.arg((*this)(val._block));
}

QString StringVisitor::operator ()(const AST::stProgram &val) const
{
	QStringList parts;
	foreach (const AST::decl_part& decl_part, val._decl_part_list._parts)
		parts << (*this)(decl_part);

	parts << (*this)(val._program);
	return parts.join("\r\n");
}

QString StringVisitor::operator ()(const AST::unit &val) const
{
	QStringList parts;
	if (_type == otPascal)
	parts << QString("unit %1;").arg((*this)(val._ident));
	if (val._flags & AST::unit::HasInterface){
		if (_type == otPascal) parts << "interface";
		parts << (*this)(val._interface);
	}
	if (val._flags & AST::unit::HasImplementation){
		if (_type == otPascal) parts << "implementation";
		parts << (*this)(val._implementation);
	}
	if (val._flags & AST::unit::HasInit){
		if (_type == otC)  parts << "/*";
		parts << "initialization";
		parts << (*this)(val._initialization);
		if (_type == otC)  parts << "*/";
	}
	if (val._flags & AST::unit::HasFin){
		if (_type == otC)  parts << "/*";
		parts << "finalization";
		parts << (*this)(val._finalization);
		if (_type == otC)  parts << "*/";
	}
	if (val._flags & AST::unit::HasBlock){
		if (_type == otC)  parts << "/*";
		if (_type == otPascal) parts << "begin";
		parts << (*this)(val._block);
		if (_type == otC)  parts << "*/";
	}
	if (_type == otPascal)
	parts << ".";
	return parts.join("\r\n");
}

QString StringVisitor::operator()(const AST::sequence &val) const
{
	QStringList out;
	foreach (const AST::statement& s, val._statements)
		out << idn() + (*this)(s) +  (_type == otC ? "" : ";" );
	return out.join("\r\n");
}

QString StringVisitor::operator()(const AST::block &val) const
{
	QString decl = (*this)(val._decl_part_list);
	if (_type == otC){
		QStringList out;
		out <<  "{";
	   _level++;
	   const bool hasResult = _makeResult.size();
	   if (hasResult){
		   out << idn() + _makeResult+" Result;";
		   _makeResult = "";
	   }
		out << decl;
		out << (*this)(val._compoundst._sequence);
		if (hasResult){
			out << idn() + "return Result;";
		}
		_level--;
		out << idn() + "}";
		return out.join("\r\n");
	}

	if (decl.size()) decl +="\r\n";
	return
			 decl +
			(*this)(val._compoundst);
}

QString StringVisitor::operator ()(const boost::blank &val) const
{
	return "?";
}
