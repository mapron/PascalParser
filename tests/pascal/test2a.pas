program a;

var b: word;
begin
b := ReadModbusRegister('test', 1);
writeln(b);

end.
