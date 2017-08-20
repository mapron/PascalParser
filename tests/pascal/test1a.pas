program test;

var c:integer;
function sum(a,b:integer):integer;
begin
   Result :=a+b;
end;

procedure inc1(var a:integer);
begin
   a :=a+1;
end;

procedure inc2(a:integer);
begin
   a :=a+1;
end;

begin
    c:=sum(2, 3);
    writeln(c);
    inc(c);
    writeln(c);
    inc1(c);
    writeln(c);
    inc2(c);
    writeln(c);
end.
