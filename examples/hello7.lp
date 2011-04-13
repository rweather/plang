:- import(stdout).
:- import(findall).

person("Frank").
person("Mary").
person("George").
person("Amy").

parent("Frank", "Mary").
parent("Frank", "George").
parent("Mary", "Amy").

descendent(C, P)
{
    parent(P, C);
}
descendent(GC, P)
{
    parent(P, C);
    descendent(GC, C);
}

main(Args)
{
    findall(P, person(P), People);
    stdout::writeln("People: " + People);

    findall(C, parent("Frank", C), Children);
    stdout::writeln("Frank's children: " + Children);

    findall(D, descendent(D, "Frank"), Descendents);
    stdout::writeln("Frank's descendents: " + Descendents);
}
