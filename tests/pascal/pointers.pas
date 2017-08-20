program A;

type REC = record
  x,y:integer;
  end;


var
  X: array[0..2] of integer;
  PX: ^integer;
  PX2: ^integer;
  PX3: array[0..1] of ^integer;
  point: REC;
  p_point: ^REC;

begin
PX:=@X[1];
PX:=@X[0];
X[0]:=5;

writeln('PX=' + PX^); // 5
PX^ := 7;
writeln('X[0]=' + X[0]); // 7
PX2 := PX;
writeln('PX2=' + PX2^); // 7
inc(PX2);
//PX2 := @X[1];

PX2^ := 8;
writeln('X[0]=' + X[0]); // 7
writeln('X[1]=' + X[1]); // 8

PX3[0]:= @X[0];

p_point := @point;
p_point^.x :=4;

writeln('point.x=' + point.x); // 4
end.
