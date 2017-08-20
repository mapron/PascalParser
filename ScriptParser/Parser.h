/*----------------------------------------------------------------------
Compiler Generator Coco/R,
Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
extended by M. Loeberbauer & A. Woess, Univ. of Linz
ported to C++ by Csaba Balazs, University of Szeged
with improvements by Pat Terry, Rhodes University

This program is free software; you can redistribute it and/or modify it 
under the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 2, or (at your option) any 
later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
for more details.

You should have received a copy of the GNU General Public License along 
with this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

As an exception, it is allowed to write an extension of Coco/R that is
used as a plugin in non-free software.

If not otherwise stated, any source code generated by Coco/R (other than 
Coco/R itself) does not fall under the GNU General Public License.
-----------------------------------------------------------------------*/


#if !defined(PascalLike_COCO_PARSER_H__)
#define PascalLike_COCO_PARSER_H__

#include "wchar.h"
#include "ast.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>


#include "Scanner.h"
#include "ast.h"
#include <QString>
#include <QStringList>

namespace PascalLike {




class Parser {
private:
	enum {
		_EOF=0,
		_ident=1,
		_string=2,
		_character=3,
		_intcon=4,
		_realcon=5,
		_bincon=6,
		_octcon=7,
		_hexcon=8,
		_semicol=9,
		_colon=10,
		_dotdot=11,
		_dot=12,
		_ABSTRACT=13,
		_AND=14,
		_ARRAY=15,
		_BEGIN=16,
		_BREAK=17,
		_CASE=18,
		_CLASS=19,
		_CONST=20,
		_CONSTANT=21,
		_CONSTRUCTOR=22,
		_CONTINUE=23,
		_DEC=24,
		_DESTRUCTOR=25,
		_DIV=26,
		_DO=27,
		_DOWNTO=28,
		_ELSE=29,
		_END_=30,
		_EXCEPT=31,
		_EXTERNAL=32,
		_EXIT=33,
		_FALSE=34,
		_FAR=35,
		_FILE=36,
		_FINALIZATION=37,
		_FINALLY=38,
		_FOR=39,
		_FORWARD=40,
		_FUNCTION=41,
		_GOTO=42,
		_IDENT=43,
		_IF_=44,
		_INC=45,
		_INHERITED=46,
		_HALT=47,
		_HIGH=48,
		_IMPLEMENTATION=49,
		_INTERFACE=50,
		_INITIALIZATION=51,
		_IN=52,
		_LABEL=53,
		_LOOPHOLE=54,
		_LOW=55,
		_MOD=56,
		_NOT=57,
		_OBJECT=58,
		_OF=59,
		_ON=60,
		_OR=61,
		_ORIGIN=62,
		_OTHERWISE=63,
		_OVERLOAD=64,
		_OVERRIDE=65,
		_PACKED=66,
		_PRIVATE=67,
		_PROCEDURE=68,
		_PROGRAM=69,
		_PROTECTED=70,
		_PUBLIC=71,
		_RAISE_=72,
		_RECORD=73,
		_REPEAT=74,
		_RETURN=75,
		_SET=76,
		_SHL=77,
		_SHR=78,
		_STATIC=79,
		_STR=80,
		_THEN=81,
		_TO_=82,
		_TRUE=83,
		_TRY=84,
		_TYPE=85,
		_UNTIL=86,
		_UNIT=87,
		_USES=88,
		_VAR=89,
		_VIRTUAL=90,
		_WHILE=91,
		_WITH=92,
		_XOR=93,
		_READ=94,
		_READLN=95,
		_WRITE=96,
		_WRITELN=97,
		_PLUS=98,
		_MINUS=99,
		_MUL=100,
		_DIVR=101,
		_LET=102,
		_LETADD=103,
		_LETSUB=104,
		_LETDIV=105,
		_LETMUL=106,
		_LETSHL=107,
		_LETSHR=108,
		_LETAND=109,
		_LETOR=110,
		_GT=111,
		_LT=112,
		_GE=113,
		_LE=114,
		_EQ=115,
		_NE=116,
		_C_NOT=117,
		_C_OR=118,
		_C_AND=119,
		_C_ORBIN=120,
		_C_ANDBIN=121,
		_C_SHL=122,
		_C_SHR=123,
		_C_MOD=124,
		_C_EQ=125,
		_C_NEQ=126,
		_CIRCUM=127,
		_FUNCTION_BLOCK=128,
		_END_FUNCTION_BLOCK=129,
		_VAR_INPUT=130,
		_VAR_OUTPUT=131,
		_VAR_IN_OUT=132,
		_VAR_EXTERNAL=133,
		_VAR_CONST=134,
		_END_VAR=135,
		_END_FUNCTION=136,
		_END_IF=137,
		_END_FOR=138,
		_END_WHILE=139,
		_END_REPEAT=140,
		_END_CASE=141,
		_END_PROGRAM=142,
		_END_TYPE=143,
		_END_RECORD=144,
		_END_CLASS=145,
		_RETAIN=146,
		_M_ADD=147,
		_M_CONCAT=148,
		_M_EQ=149,
		_M_NE=150,
		_M_GE=151,
		_M_GT=152,
		_M_LE=153,
		_M_LT=154,
		_M_MAX=155,
		_M_MIN=156,
		_M_MUL=157,
		_M_MUX=158,
		_OPENPAR=159,
		_CLOSEPAR=160,
		_OPEN_ANNO=161,
		_CLOSE_ANNO=162,
		_OPEN_BRACE=163,
		_CLOSE_BRACE=164
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

    void Err(const QString& msg);
	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
    AST::CodeMessages  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

AST::pascalSource   _pascal;
    bool _StSemantic;
    int _currentFile;

    void Err(wchar_t* msg) {
        errors->Error(la->line, la->col, msg);
    }

    bool IsLabelSt() {
       scanner->ResetPeek();
        Token *x = la;
        while ( x->kind == _ident)
            x = scanner->Peek();
        return x->kind == _colon;
    }

    bool IsBoundSt() {
       scanner->ResetPeek();
        Token *x = la;
        while ( x->kind == _ident ||  x->kind == _IN)
            x = scanner->Peek();
        return x->kind == _colon ||  x->kind == _LET;
    }


    bool IsAssignment() {
        scanner->ResetPeek();
        static QSet<int> breakSet = QSet<int>() << _EOF << _FOR << _DO << _WITH << _REPEAT << _IF_ << _BEGIN  << _TRY << _RAISE_ << _CASE ;
        Token *x = la;
        int parLevel = 0;
        while ( x->kind != _semicol ){
           if (breakSet.contains(x->kind)) return false;
           if (x->kind == _OPENPAR) parLevel ++;
           if (x->kind == _CLOSEPAR) parLevel --;
            x = scanner->Peek();
            if (x->kind == _LET && parLevel ==0) return true;
        }
        return false;
    }
    AST::CodeLocation cur() {
        if (la) return AST::CodeLocation (_currentFile, la->line, la->col, la->charPos);
        return AST::CodeLocation (_currentFile, t->line, t->col, t->charPos);
    }

    bool StSemantic() {
        return _StSemantic;
    }


    void MakeTokenList(QList<AST::TToken> &list ) {

        static QSet<int> kw = QSet<int>() <<         _ABSTRACT<< _AND<< _ARRAY<< _BEGIN<< _BREAK<< _CASE<< _CLASS<< _CONST << _CONSTANT << _CONSTRUCTOR<< _CONTINUE<< _DEC<< _DESTRUCTOR<< _DIV<< _DO<<
                                                     _DOWNTO<< _ELSE<< _END_<< _EXCEPT<< _EXTERNAL<< _EXIT<< _FALSE<< _FAR<< _FILE<< _FINALIZATION<< _FINALLY<< _FOR<< _FORWARD<< _FUNCTION<<
                                                     _GOTO<< _IDENT<< _IF_<< _INC<< _INHERITED<< _HALT<< _HIGH<< _IMPLEMENTATION<< _INTERFACE<< _INITIALIZATION<< _IN<< _LABEL<< _LOOPHOLE<<
                                                     _LOW<< _MOD<< _NOT<< _OBJECT<< _OF<< _ON<< _OR<< _ORIGIN<< _OTHERWISE<< _OVERLOAD<< _OVERRIDE<< _PACKED<< _PRIVATE<< _PROCEDURE<< _PROGRAM<<
                                                     _PROTECTED<< _PUBLIC<< _RAISE_<< _RECORD<< _REPEAT<< _RETURN<< _SET<< _SHL<< _SHR<< _STATIC<< _STR<< _THEN<< _TO_<< _TRUE<< _TRY<<
                                                     _TYPE<< _UNTIL<< _UNIT<< _USES<< _VAR<< _VIRTUAL<< _WHILE<< _WITH<< _XOR<< _READ<< _READLN<< _WRITE<< _WRITELN<< _FUNCTION_BLOCK<<
                                                     _END_FUNCTION_BLOCK<< _VAR_INPUT << _VAR_OUTPUT<< _VAR_IN_OUT<< _VAR_EXTERNAL << _VAR_CONST << _END_VAR<<_END_FUNCTION<< _END_IF<<_END_FOR<<_END_WHILE<< _END_REPEAT<<
                                                     _END_CASE << _END_PROGRAM << _END_TYPE << _END_CLASS << _RETAIN ;

        static QSet<int> ops = QSet<int>() <<   _PLUS<< _MINUS<<_MUL<<_DIVR<<_LET<<_GT<<_LT<<_LE<<_GE<<_EQ<<_NE;

         static QSet<int> blockStartKeyWords = QSet<int>() <<   _IF_<< _FOR<<_WHILE<< _PROGRAM << _FUNCTION << _FUNCTION_BLOCK << _PROCEDURE << _TYPE << _ELSE
                                                             << _VAR  << _VAR_INPUT << _VAR_OUTPUT << _VAR_IN_OUT << _VAR_EXTERNAL  << _VAR_CONST << _BEGIN;
         static QSet<int> blockEndKeyWords = QSet<int>() << _ELSE  << _END_IF << _END_FOR << _END_WHILE << _END_PROGRAM
                                  << _END_FUNCTION << _END_FUNCTION_BLOCK << _END_TYPE << _END_VAR << _END_CLASS << _END_;


        t = NULL;
        la = dummyToken;
       // la->val = coco_string_create(L"Dummy Token");
        Get();
        while (la->kind != 0){
            AST::TToken tk;
            tk._charPos = la->charPos;
            tk._tlen = la->valSize;
            if (blockStartKeyWords.contains(la->kind)) {
                tk._kwType |= AST::TToken::kwBlockStart;
            }
            if (blockEndKeyWords.contains(la->kind)) {
                tk._kwType |= AST::TToken::kwBlockEnd;
            }
            if (kw.contains(la->kind))
                tk._type = AST::TToken::tKeyword;
            if (ops.contains(la->kind))
                tk._type = AST::TToken::tOperator;
            if (la->kind == _ident)
                tk._type = AST::TToken::tIdent;
            if (la->kind == _string || la->kind == _character)
                tk._type = AST::TToken::tString;
            if (la->kind == _realcon || la->kind == _intcon)
                tk._type = AST::TToken::tNumber;
            list << tk;
            Get();
        }
    }

    void ParseExpression(AST::expr& expr){
        t = NULL;
        la = dummyToken;
        Get();
        Expr(expr);
        Expect(0);
    }
    void ParseAssignment(AST::assignmentst& assignmentst){
        t = NULL;
        la = dummyToken;
        Get();
        Assignmentst(assignmentst);
        Expect(0);
    }

    QString ReadString(Token* t, bool hasQuotes = false, bool removeUnderscore = false)
    {
        QString ret;
        if (t->val && t->valSize)
            ret= QString::fromWCharArray(t->val + int(hasQuotes), t->valSize - int(hasQuotes)*2);
        if (removeUnderscore)
            ret.replace('_', "");
        return ret;
    }

    int64_t ReadInteger(Token* t, int offset =0, int base = 10)
    {
        return ReadString(t, false, true).mid(offset).toLongLong(0, base);
    }



    Token* Peek() {
        scanner->ResetPeek();
        return scanner->Peek();
    }



    Parser(Scanner *scanner,AST::CodeMessages *errors);
	~Parser();
	void SemErr(const wchar_t* msg);

	void IdentRW(AST::ident &ident);
	void Ident(AST::ident &ident);
	void Constant(AST::constant &constant);
	void CString(QString& s);
	void ExprList(AST::expr_list &expr_list);
	void Expr(AST::expr &expr);
	void SimExpr(AST::expr &expr);
	void RelOp(BytecodeVM::BinOp &op);
	void Term(AST::expr &expr);
	void AddOp(BytecodeVM::BinOp &op);
	void Term2(AST::expr &expr);
	void MulOp(BytecodeVM::BinOp &op);
	void SimpleExpr(AST::expr &expr);
	void BinaryOp(BytecodeVM::BinOp &op);
	void PrimaryExprWrap(AST::expr &expr);
	void PrimaryExpr(AST::primary &primary);
	void Internalexpr(AST::internalexpr &internalexpr);
	void Primary_accessor(AST::primary_accessor &primary_accessor);
	void Annotation(AST::annotation &annotation);
	void Sequence(AST::sequence &sequence);
	void Statement(AST::statement &statement);
	void Labelst(AST::labelst &labelst);
	void Assignmentst(AST::assignmentst &assignmentst);
	void Procst(AST::procst &procst);
	void Compoundst(AST::compoundst &compoundst);
	void Gotost(AST::gotost &gotost);
	void Switchst(AST::switchst &switchst);
	void Ifst(AST::ifst &ifst);
	void Forst(AST::forst &forst);
	void Whilest(AST::whilest &whilest);
	void Repeatst(AST::repeatst &repeatst);
	void Withst(AST::withst &withst);
	void Tryst(AST::tryst &tryst);
	void Raisest(AST::raisest &raisest);
	void Inheritedst(AST::inheritedst &inheritedst);
	void Readst(AST::readst &readst);
	void Writest(AST::writest &writest);
	void Strst(AST::strst &strst);
	void Onst(AST::onst &onst);
	void Internalfst(AST::internalfst &internalfst);
	void Case_list(AST::case_list &case_list);
	void Case_list_elem(AST::case_list_elem &case_list_elem);
	void Write_list(AST::write_list &write_list);
	void Type(AST::type &type);
	void Write_param(AST::write_param &write_param);
	void Simple_type(AST::simple_type &simple_type);
	void Array_type(AST::array_type &array_type);
	void Record_type(AST::class_type &class_type);
	void Set_type(AST::set_type &set_type);
	void File_type(AST::file_type &file_type);
	void Pointer_type(AST::pointer_type &pointer_type);
	void Subrange_type(AST::subrange_type &subrange_type);
	void Enum_type(AST::enum_type &enum_type);
	void Function_type(AST::function_type &function_type);
	void Class_type(AST::class_type &class_type);
	void Array_range(AST::array_range &array_range);
	void Var_decls(AST::var_decls &var_decls);
	void Class_section(AST::class_section &class_section);
	void Class_sections(AST::class_sections &class_sections);
	void Proc_def_forward(AST::proc_def &proc_def);
	void Decl_part(AST::decl_part& decl_part);
	void Decl_part_common(AST::decl_part& decl_part);
	void Proc_def(AST::proc_def &proc_def);
	void Decl_part_forward(AST::decl_part& decl_part);
	void Label_decl_part(AST::label_decl_part& label_decl_part);
	void Const_def_part(AST::const_def_part &const_def_part);
	void Type_def_part(AST::type_def_part &type_def_part);
	void Var_decl_part(AST::var_decl_part &var_decl_part);
	void Uses_part(AST::uses_part& uses_part);
	void Const_def(AST::const_def &const_def);
	void Type_def(AST::type_def &type_def);
	void Decl_part_list(AST::decl_part_list& decl_part_list);
	void Decl_part_list_forward(AST::decl_part_list& decl_part_list);
	void Var_decl(AST::var_decl& var_decl);
	void Proc_decl(AST::proc_decl &proc_decl);
	void Formal_params(AST::formal_params &formal_params);
	void Block(AST::block& block);
	void Proc_def_params(AST::proc_def &proc_def);
	void Formal_param(AST::formal_param &formal_param);
	void Pascal();
	void StProgram(AST::stProgram& program);
	void Program(AST::program& program);
	void Unit(AST::unit& unit);
	void StDecl_part_list(AST::decl_part_list& decl_part_list);
	void FBlock(AST::proc_def& proc_def);
	void StVarDecl(AST::decl_parts& decl_parts);
	void StParamDecl(AST::formal_params& formal_params);
	void StType(AST::type &type);
	void StRecord_type(AST::class_type &class_type);
	void StClass_type(AST::class_type &class_type);
	void StClass_section(AST::class_section &class_section);
	void StClass_sections(AST::class_sections &class_sections);
	void StProc_def(AST::proc_def& proc_def);
	void StDecl_part(AST::decl_part& decl_part);
	void StType_def_part(AST::type_def_part &type_def_part);

	void Parse();

}; // end Parser

} // namespace


#endif

