program a;

var i,j: int;
    test: array[4..8] of int;
begin

for i := 1 to 5 do
begin
    if i < 3 then writeln('less than 3:' + i)
    else          writeln('greater or equal than 3:' + i);
end;

test[4] := 44;
test[5] := 55;
test[6] := 66;
test[7] := 77;
test[8] := -88;


for i := 8 downto 4 do
begin
    writeln('test[' + i + ']=' + test[i] );
end;


end.
