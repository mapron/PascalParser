program test;

var str1:string;
    a:integer;

begin
a := ReadModbusRegister('test', 2) ;
str1 := 'five:';
str1 := str1 + a;
writeln(str1);

end.
