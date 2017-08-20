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
#include <QString>
#include <QList>
#include <QMap>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <BytecodeVM.h>

#define AST_CONSTRUCT(clas) clas():_node(CodeLocation()) {} \
		clas(const CodeLocation& loc):_node(loc) {} \
		void serialize(TreeVariant& data, SerializationDirection direction) override;

/// Abstract syntax tree for Pascal sources.

class TreeVariant;
namespace AST {

struct TToken {
	enum Type { tNone, tKeyword, tOperator, tString, tNumber, tIdent };
	enum KwType { kwNone, kwBlockStart, kwBlockEnd };
	Type _type;
	int _kwType;
	int _charPos;
	int _tlen;

	TToken() : _type(tNone), _kwType(kwNone) {}
};

struct CodeLocation {
	int _file;
	int _line;
	int _col;
	int _offset;
	CodeLocation();
	CodeLocation(int l, int c = -1, int o = -1);
	CodeLocation(int f, int l, int c, int o);
	bool operator < (const CodeLocation& another) const;
	QString toString() const;
};
/**
 * \brief parser or compiler message, bound to AST node.
 */
struct CodeMessage {
	enum Type { Error, Warning, Info };
	CodeLocation _loc;
	Type _type;
	QString _message;
	CodeMessage();
	CodeMessage(QString message);
	CodeMessage(int line, int col, QString message);
	CodeMessage(Type type, int line, int col, QString message);
	CodeMessage(Type type, QString message);
	CodeMessage(Type type, const CodeLocation& loc, QString message);
	QString toString(bool useType = true) const;
};
/**
 * \brief compiler message list.
 */
class CodeMessages {
public:
	QList<CodeMessage> _messages;
	QMap<CodeLocation, QSet<QString> > _texts;

	int errorsCount;
	int warningsCount;
	void clear();
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Error(int line, int col, const QString& s);
	void Error(const CodeLocation& loc, const QString& s);

	void Warning(int line, int col, const QString& s);
	void Warning(const CodeLocation& loc, const QString& s);
	void Warning(const wchar_t *s);

	void Info(const CodeLocation& loc, const QString& s);
	void Add(const CodeMessage& msg);
};
/**
 * \brief Common AST node. Has source code location.
 */
struct _node {
	CodeLocation _loc;
	int _flags;
	_node(const CodeLocation& loc):_loc(loc),_flags(0) {}
	virtual ~_node() {}
	enum SerializationDirection {sdLoad = 0x01, sdSave= 0x02, sdLocation = 0x04 };

	TreeVariant save(bool saveLocation = false);
	void load(const TreeVariant& data);

	virtual void serialize(TreeVariant& data, SerializationDirection direction);
};

// Forward declarations.
struct type;
struct simple_type;
struct array_type;
struct set_type;
struct file_type;
struct pointer_type;
struct subrange_type;
struct enum_type;
struct function_type;
struct class_type;
struct statement ;

struct compoundst ;
struct assignmentst ;
struct gotost ;
struct switchst ;
struct ifst;
struct forst ;
struct whilest ;
struct repeatst ;
struct procst ;
struct withst ;
struct labelst ;
struct tryst ;
struct writest ;
struct readst;
struct strst ;
struct raisest;
struct inheritedst;
struct onst;
struct primary;
struct unary;
struct binary;
struct internalexpr;


struct ident : public _node {
	AST_CONSTRUCT(ident)
	QString _ident;
};
using idents = QList<ident>;
/*
//=============================================================================
// Expression level syntax:
//   constant      ::= integer | real | string | set_construct | ident
//   set_elem      ::= expr | expr '..' expr
//   set_construct ::= '[' set_elem { ',' set_elem } ']'
//   expr_group    ::= '(' expr_list ')'
//   expr_list     ::= expr { ',' expr }
//   primary       ::= '(' expr ')' | call_expr
//                   | deref_expr | idx_expr | access_expr | constant
//   access_expr   ::= expr '.' ident
//   deref_expr    ::= expr '^'
//   idx_expr      ::= expr '[' expr_list ']'
//   call_expr     ::= expr expr_group
//   binary        ::= expr op expr
//   unary         ::= op expr
//   expr          ::= primary | unary | binary
//   write_list    ::= '(' write_param { ',' write_param } ')'
//   write_param   ::= expr [ ':' constant [ ':' constant ] ]
//=============================================================================
*/

struct expr : public _node{
	AST_CONSTRUCT(expr)
	enum Type { t_None, t_primary, t_unary, t_binary };
	boost::variant<boost::blank, boost::recursive_wrapper< primary>, boost::recursive_wrapper< unary>,
		boost::recursive_wrapper< binary> > _expr;

};
struct expr_list : public _node {
	AST_CONSTRUCT(expr_list)
	QList<expr> _exprs;
	idents _idents;
};

struct set_construct : public _node {
	AST_CONSTRUCT(set_construct)
	expr_list _expr_list;
};
struct constant : public _node {
	 AST_CONSTRUCT(constant)
	 enum Type { t_None, t_bool, t_int64_t, t_double, t_QString, t_set_construct };
	 boost::variant<boost::blank, bool, int64_t, double, QString, set_construct > _constant;
};
struct deref_expr : public _node  { // x^
	AST_CONSTRUCT(deref_expr)
};
struct address_expr : public _node  { // @x
	AST_CONSTRUCT(address_expr)
};

struct idx_expr : public _node  { // x[]
	AST_CONSTRUCT(idx_expr)
	expr_list _indexes;
};

struct access_expr : public _node  { //x.
	 AST_CONSTRUCT(access_expr)
	 ident _field;
};
struct call_expr : public _node  {// x()
	AST_CONSTRUCT(call_expr)
	expr_list _args;
};

struct primary_accessor : public _node  {
	AST_CONSTRUCT(primary_accessor)
	enum Which { t_blank, t_address_expr, t_call_expr,  t_deref_expr ,t_idx_expr, t_access_expr };
	boost::variant<boost::blank, address_expr, call_expr,  deref_expr ,idx_expr, access_expr> _primary_accessor;
};

struct internalexpr : public _node  {
	AST_CONSTRUCT(internalexpr)
	enum Type {tLow, tHigh, tInc, tDec, tAdd, tAnd, tConcat, tEq, tNe, tGe, tGt, tLe, tLt, tMax, tMin, tMul, tOr, tXor, tMux,
			   START_BIN = tAdd, END_BIN = tXor} _type;
	expr_list _expr_list;

};

struct primary : public _node  {
	AST_CONSTRUCT(primary)
	enum Type { t_blank,        t_ident, t_expr_list, t_constant, t_internalexpr };
	boost::variant<boost::blank, ident, expr_list,   constant,     internalexpr > _expr;
	QList<primary_accessor> _accessors;
};


struct unary : public _node  {
	AST_CONSTRUCT(unary)

	BytecodeVM::UnOp _op;
	expr _expr;
};

struct binary : public _node  {
	AST_CONSTRUCT(binary)
	expr _left;
	expr _right;

	BytecodeVM::BinOp _op;
};


struct annotation : public _node {
	 AST_CONSTRUCT(annotation)
	 expr_list _expr_list;
};


//=============================================================================
// Statement level grammar
//   statement       ::= compoundst | assignmentst | gotost | switchst | ifst
//                    | forst | whilest | repeatst | procst | returnst
//                    | withst | labelst | emtyst | writest | readst
//   sequence        ::= statement { ';' statement }
//   compoundst      ::= BEGIN sequence END
//   assignmentst    ::= expr ':=' expr
//   gotost          ::= GOTO iconst
//   labelst         ::= iconst ':'
//   switchst        ::= CASE expr OF case_list END
//   case_list       ::= case_list_elem { ';' case-list-elem } [ ';' ]
//   case_list_elem  ::= expr_list ':' statement | OTHERWISE statement
//   ifst            ::= IF expr THEN statement [ELSE statement]


//=============================================================================

struct internalfst : public _node  {
	AST_CONSTRUCT(internalfst)
	enum Type {tBreak, tContinue, tExit} _type;
  //  expr _expr;
};
struct statement : public _node  {
	AST_CONSTRUCT(statement)
	enum Type { t_blank, t_compoundst, t_assignmentst, t_gotost, t_switchst, t_ifst,
			   t_forst, t_whilest, t_repeatst, t_procst, t_withst, t_labelst, t_tryst, t_writest, t_readst,
			   t_strst, t_raisest, t_inheritedst,t_onst, t_internalfst};
	boost::variant<boost::blank,
	boost::recursive_wrapper<compoundst> ,
	boost::recursive_wrapper<assignmentst> ,
	boost::recursive_wrapper<gotost> ,
	boost::recursive_wrapper<switchst>,
	boost::recursive_wrapper<ifst> ,
	boost::recursive_wrapper<forst> ,
	boost::recursive_wrapper<whilest>,
	boost::recursive_wrapper<repeatst>,
	boost::recursive_wrapper<procst>,
	boost::recursive_wrapper<withst>,
	boost::recursive_wrapper<labelst>,
	boost::recursive_wrapper<tryst>,
	boost::recursive_wrapper<writest>,
	boost::recursive_wrapper<readst>,
	boost::recursive_wrapper<strst>,
	boost::recursive_wrapper<raisest>,
	boost::recursive_wrapper<inheritedst>,
	boost::recursive_wrapper<onst>,
	internalfst
	> _statement;

};
struct sequence : public _node  {
	AST_CONSTRUCT(sequence)
	QList<statement> _statements;
};
struct compoundst : public _node  {
	AST_CONSTRUCT(compoundst)
	sequence _sequence;
};

struct assignmentst : public _node  {
	AST_CONSTRUCT(assignmentst)
	expr _left;
	expr _right;
};
struct gotost : public _node  {
	AST_CONSTRUCT(gotost)
	int _iconst;
};
struct labelst : public _node  {
	AST_CONSTRUCT(labelst)
	ident _ident;
};
struct case_list_elem : public _node  {
	AST_CONSTRUCT(case_list_elem)
	expr_list _expr_list;
	statement _statement;
};
using case_list = QList<case_list_elem>;

struct switchst : public _node  {
	AST_CONSTRUCT(switchst)
	expr _case;
	case_list _case_list;
	sequence _else;
};
struct ifst : public _node  {
	AST_CONSTRUCT(ifst)
	expr _expr;
	statement _if;
	statement _else;
};
struct forst : public _node  {
	AST_CONSTRUCT(forst)
	ident _ident;
	bool  _downto;
	expr _fromExpr;
	expr _toExpr;
	statement _statement;
};

struct whilest : public _node  {
	AST_CONSTRUCT(whilest)
	expr _expr;
	statement _statement;
};

struct repeatst : public _node  {
	AST_CONSTRUCT(repeatst)
	expr _expr;
	sequence _sequence;
};


struct procst : public _node  {// procedure call statement
	AST_CONSTRUCT(procst)
	primary _primary;
};

struct withst  : public _node  {
	AST_CONSTRUCT(withst)
	primary _expr;
	statement _statement;
};

struct tryst : public _node  {
	AST_CONSTRUCT(tryst)
	enum Flags { HasExcept = 0x01, HasFinally = 0x02 };
	sequence _try_;
	sequence _except_;
	sequence _finally_;
};
struct write_param : public _node  {
	AST_CONSTRUCT(write_param)
	expr _expr;
	QList<constant> _constants;
};

struct write_list : public _node  {
	AST_CONSTRUCT(write_list)
	QList<write_param> _write_params;
};


struct writest : public _node  {
	AST_CONSTRUCT(writest)
	bool _isLn;
	write_list _write_list;
};
struct strst  : public _node {
	AST_CONSTRUCT(strst)
	write_list _write_list;
};

struct readst : public _node  {
	AST_CONSTRUCT(readst)
	bool _isLn;
	expr_list _expr_list;
};

struct raisest  : public _node {
	AST_CONSTRUCT(raisest)
	expr _expr;
};
struct inheritedst : public _node  {
	AST_CONSTRUCT(inheritedst)
	ident _ident;
};

struct onst  : public _node {
	AST_CONSTRUCT(onst)
	ident _ident;
	boost::recursive_wrapper<type> _type;
	statement _statement;
};




/*
//=============================================================================
// Types syntax:
//   type             ::= simple_type | array_type | class_type | set_type |
//                        file_type | pointer_type | subrange_type | enum_type
//   subrange_type    ::= '[' expr '..' expr ']'
//   enum_type        ::= '(' expr { ',' expr } ')'
//   pointer_type     ::= '^' type
//   file_type        ::= FILE OF type_denoter
//   set_type         ::= SET OF type_denoter
//   class_type       ::= RECORD field_list END
//   field_list       ::= [ (fixed_part [';' variant_part] | variant_part) [;] ]
//   fixed_part       ::= var_decls
//   variant_part     ::= CASE selector OF
//                        variant { ';' variant }
//   selector         ::= [ tag_field ':' ] tag_type
//   variant          ::= constant { ',' constant } ':' '(' field_list ')'
//   simple_type      ::= ident
//   array_type       ::= ARRAY '[' index ']' OF type
//   index            ::= conformant_index | fixed_index
//   conformant_index ::= var_decls
//   fixed_index      ::= range { ',' range }
//   range            ::= expr '..' expr | type
//=============================================================================
*/




struct label_decl_part : public _node  {
	AST_CONSTRUCT(label_decl_part)
	idents _idents;
};


//   type             ::= simple_type | array_type | record_type | set_type |
//                        file_type | pointer_type | subrange_type | enum_type
struct type : public _node  {
	AST_CONSTRUCT(type)
	enum Type { t_None,
				t_simple_type,
				t_array_type,
				t_set_type,
				t_file_type,
				t_pointer_type,
				t_subrange_type,
				t_enum_type,
				t_function_type,
				t_class_type };
	boost::variant<boost::blank,
		boost::recursive_wrapper< simple_type>,
		boost::recursive_wrapper< array_type>,
		boost::recursive_wrapper< set_type>,
		boost::recursive_wrapper< file_type>,
		boost::recursive_wrapper< pointer_type>,
		boost::recursive_wrapper< subrange_type>,
		boost::recursive_wrapper< enum_type>,
		boost::recursive_wrapper< function_type>,
		boost::recursive_wrapper< class_type>
	 > _type;
};

struct simple_type : public _node  {
	AST_CONSTRUCT(simple_type)
	ident _ident;
};
struct array_range : public _node  {
	AST_CONSTRUCT(array_range)
	bool _hasLow;
	expr _boundLow;
	expr _boundHigh;
};
struct array_type : public _node  {
	AST_CONSTRUCT(array_type)
	QList<array_range> _array_ranges;
	type _type;
};

struct set_type : public _node {
	AST_CONSTRUCT(set_type)
	type _type;
};
struct file_type : public _node {
	AST_CONSTRUCT(file_type)
	type _type;
};
struct pointer_type : public _node {
	AST_CONSTRUCT(pointer_type)
	type _type;
};
struct subrange_type : public _node  {
	AST_CONSTRUCT(subrange_type)
	expr _boundLow;
	expr _boundHigh;
};
struct enum_type  : public _node {
	AST_CONSTRUCT(enum_type)
	expr_list _expr_list;
};
struct function_type  : public _node {
	AST_CONSTRUCT(function_type)
	bool _isProc;
	QList<type> _types;
};

/*
//=============================================================================
// Declaration syntax:
//   label_decl_part  ::= [ LABEL ident { ',' ident } ';' ]
//   const_def_part   ::= [ CONST const_def ';' { const_def ';' } ]
//   type_def_part    ::= [ TYPE type_def ';' { type_def ';' } ]
//   var_decl_part    ::= [ VAR var_decls ';' ]
//   proc_fwd_decl    ::= proc_decl ';' ident ';'
//   proc_decl        ::= (PROCEDURE | FUNCTION) ident [ formal_params ] [ ':' type ]
//   proc_def         ::= proc_decl ';' body ';'
//   formal_params    ::= '(' formal_param { ';' formal_param } ')'
//   formal_param     ::= VAR var_decl | var_decl | proc_decl
//   const_def        ::= ident '=' expr
//   type_def         ::= ident '=' type
//   var_decls        ::= var_decl { ';' var_decl }
//   var_decl         ::= ident { ',' ident } ':' type
//=============================================================================
*/
/*struct initializer;
using initializer_list = QList<initializer> ;

struct initializer {
	boost::variant<boost::blank, expr, initializer_list> _initializer;
};*/

struct const_def  : public _node {
	AST_CONSTRUCT(const_def)
	ident _ident;
	type _type;
	expr _initializer;
};


struct var_decl  : public _node {
	AST_CONSTRUCT(var_decl)
	enum Flags { None= 0x00, IsInput = 0x01, IsOutput = 0x02,IsStatic = 0x04,IsExternal = 0x08,IsConst = 0x10 };
	idents _idents;
	type _type;
	expr _initializer;
	annotation _annotation;
};
using var_decls = QList<var_decl>;

struct const_def_part : public _node  {
	AST_CONSTRUCT(const_def_part)
	QList<const_def> _const_defs;
};


struct type_def  : public _node {
	AST_CONSTRUCT(type_def)
	ident _ident;
	type _type;
};
struct type_def_part : public _node  {
	AST_CONSTRUCT(type_def_part)
	QList<type_def> _type_defs;
};
struct var_decl_part : public _node  {
	AST_CONSTRUCT(var_decl_part)
	var_decls _var_decls;
};
struct proc_decl;
struct formal_param : public _node  {
	AST_CONSTRUCT(formal_param)
	enum Type {None, t_var_decl, t_proc_decl };
	enum Flags { IsVar = 0x01, IsConst =0x02};
	boost::variant<boost::blank,
	   boost::recursive_wrapper< var_decl>,
	   boost::recursive_wrapper< proc_decl>
	> _formal_param;
};
using formal_params = QList<formal_param>;

struct proc_decl  : public _node {
	AST_CONSTRUCT(proc_decl)
	enum Flags { IsProcedure= 0x02, IsFunction= 0x04, IsConstructor= 0x08,
				 IsDestructor= 0x10, IsOverride= 0x20, IsOverload= 0x40, IsForward= 0x80 };
	ident _ident;
	ident _class;
	type _type;
	formal_params _formal_params;
};


struct proc_def;
using proc_fwd_decls = QList<proc_def>;
struct unit_spec  : public _node {
	AST_CONSTRUCT(unit_spec)

};
struct uses_part  : public _node {
	AST_CONSTRUCT(uses_part)
	idents _idents;
};

struct decl_part  : public _node {
	AST_CONSTRUCT(decl_part)
	enum Type {None, t_label_decl_part,  t_const_def_part, t_type_def_part, t_var_decl_part,
				t_proc_def, t_unit_spec, t_uses_part };
	boost::variant<boost::blank, label_decl_part , const_def_part,type_def_part, var_decl_part,
		boost::recursive_wrapper<proc_def>, unit_spec, uses_part> _decl_part;

};

using decl_parts = QList<decl_part>;
struct decl_part_list : public _node {
	AST_CONSTRUCT(decl_part_list)
	decl_parts _parts;
};


// ==== Types
struct class_section : public _node  {
	AST_CONSTRUCT(class_section)
	enum Flags {IsPublic, IsProtected, IsPrivate };
	var_decls _var_decls;
	proc_fwd_decls _proc_fwd_decls;
};

using class_sections = QList<class_section>;
struct class_type : public _node  {
	AST_CONSTRUCT(class_type)
	enum Flags { IsRecord = 0x01 };
	ident _parent;
	class_sections _sections;
	annotation _annotation;

};
// /  Types

/*
//=============================================================================
// Program level grammar:
//   program         ::= PROGRAM ident [ '(' parameter_list ')' ] block '.'
//   parameter_list  ::= ident { ',' ident }
//   block           ::= decl_part_list compoundst
//   decl_part_list  ::= { decl_part }
//   decl_part       ::= label_decl_part | const_def_part | type_def_part
//                       | var_decl_part | proc_decl | proc_fwd_decl
//=============================================================================
*/


struct block : public _node  {
	AST_CONSTRUCT(block)
	decl_part_list _decl_part_list;
	compoundst  _compoundst;
};
struct proc_def  : public _node {
	AST_CONSTRUCT(proc_def)
	proc_decl _proc_decl;
	block _block;
	bool _isForward;
};


struct program : public _node  {
	AST_CONSTRUCT(program)
	block _block;
	ident _ident;
};

struct unit : public _node  {
	AST_CONSTRUCT(unit)
	enum Flags { HasInterface = 0x01, HasImplementation = 0x02, HasBlock = 0x04, HasInit = 0x08, HasFin = 0x10 };

	ident _ident;
	decl_part_list _interface;
	decl_part_list _implementation;
	sequence _block;
	sequence _initialization;
	sequence _finalization;
};

struct stProgram : public _node  {
	AST_CONSTRUCT(stProgram)
	decl_part_list _decl_part_list;
	proc_def _program;
	bool _hasProgram;
};


struct pascalSource : public _node  {
	AST_CONSTRUCT(pascalSource)
	enum Type {None, t_program,  t_unit, t_stProgram };
	boost::variant<boost::blank, program, unit, stProgram> _pascal;
};

}

