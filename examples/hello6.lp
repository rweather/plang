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

main()
{
    findall(D, descendent(D, "Frank"), Descendents);
    stdout::writeln("Frank's descendents: " + Descendents);
}
