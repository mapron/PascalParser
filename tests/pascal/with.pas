program n_body;

type
  Body = record
    x, y, z,
    vx, vy, vz,
    mass : double;
  end;

type
  tbody = array[3..4] of Body;

var
  n: tbody;


begin


with n[4] do
begin
  y := 2;
end;
writeln('n[4].y=' + n[4].y); // 2
end.
