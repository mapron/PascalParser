program testProgr;

Type MY_TYPE = class
private
    proxyField : bool;
    field1 : real;
    field2 : QWORD;

    Procedure setField1(zzz : integer);
    Function  getField1() : real;
    Procedure setField2(sss : integer);
    Function  getField2() : QWORD;


end;

Type MY_TYPE_2 = class
private

    fieldT : MY_TYPE;

    Procedure setField1(zzz : real);
    Function  getFieldT() : MY_TYPE;


end;

Type DERIVED_TYPE_3 = class(MY_TYPE)
public

    fieldDerived : boolean;

end;


Procedure MY_TYPE.setField1(zzz : integer);
begin
    field1 := zzz;
end;

Function MY_TYPE.getField1() : real;
begin
    getField1 := self.field1;
end;

Procedure MY_TYPE.setField2(sss : integer);
begin
    self.field2 := sss;
end;

Function MY_TYPE.getField2() : QWORD;
begin
    getField2 := field2;
end;


function MY_TYPE_2.setField1(zzz : real);
begin
    fieldT.field1 := zzz;
end;

Function MY_TYPE_2.getField2() : MY_TYPE;
begin
    result := fieldT;
end;


Procedure setObjField1(var objRef : MY_TYPE);
begin

    objRef.setField1(545646);
end;

// -----------------------------------------------------------------------------
var
    realVar : real;
    qqq : array [2] of MY_TYPE;
    container : MY_TYPE_2;
    derived: DERIVED_TYPE_3;
begin;

    qqq[1].field1 := 2123;

    setObjField1(qqq[1]);

    writeln(qqq[1].field1);

   realVar := 111.111;

    qqq[1].field1 := realVar;
    writeln(qqq[1].field1);
    qqq[1].setField2(123);
    qqq[2].setField1(456);
    writeln(qqq[0].getField1());
    writeln(qqq[1].getField2());
    writeln(qqq[2].getField1());


    container.fieldT.field1:=1.5;
    writeln(container.getField2().getField1()); // 1.5

    derived.fieldDerived := 5;
    derived.setField1(123);
    writeln(derived.field1); // 123
end.
