program testProgr;

Type MY_TYPE = class
public
    field1 : real;
    field2 : integer;

    Procedure forwardDecl(a:integer);

    Procedure  commonDecl(a:integer) ;

end;

Procedure MY_TYPE.commonDecl(a:integer);
begin
    forwardDecl(a+1);
end;

Procedure MY_TYPE.forwardDecl(a:integer) ;
begin
    writeln('a='+a);
end;

// -----------------------------------------------------------------------------
var qqq : MY_TYPE;
begin;
    qqq.commonDecl(1);
end.
