program test;

var regs:array[4] of word;
    a:integer;
    b:float;

begin
regs[0]:=1;
regs[1]:=3;
regs[2]:=5;
regs[3]:=7;
a:=1000;

a:=ReadInt(regs[0], regs[1], true);
writeln('a='+a);
a:=ReadInt(regs[0], regs[1], false);
writeln('a='+a);
a:=ReadInt(regs[1], regs[0], true);
writeln('a='+a);
a:=ReadInt(regs[1], regs[0], false);
writeln('a='+a);

b:=ReadFloat(regs[0], regs[1], true);
writeln('b='+b);
b:=ReadFloat(regs[0], regs[1], false);
writeln('b='+b);
b:=ReadFloat(regs[1], regs[0], true);
writeln('b='+b);
b:=ReadFloat(regs[1], regs[0], false);
writeln('b='+b);
writeln(regs);
end.
