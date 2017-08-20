program testProgr;

Type MY_TYPE = class
public
	field1 : real;
	field2 : integer;

	Procedure setField2(a:integer);

	Function  getField2() : integer;

end;

Procedure MY_TYPE.setField2(a:integer);
begin
	field2 := 123456789 + a;
  //  writeln('THIS IS MEMBEEEEEER!!!!');
end;

Function MY_TYPE.getField2() : integer;
begin
	getField2 := self.field2;
end;


Procedure setField2(); begin writeln('THIS IS PROCEDUUUUREE!!!!'); end;

// -----------------------------------------------------------------------------
var qqq : MY_TYPE;
begin;
   // qqq.setField2(5);
	setField2();
	writeln(qqq.field2);
end.
