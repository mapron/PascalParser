program a;

var i: int;
begin

for i := 1 to 5 do
begin
    if i >= 3 then break;
    writeln('i=' + i);
end;

for i := 1 to 5 do
begin
    if i = 3 then continue;
    writeln('i=' + i);
end;

end.
