:- import(stdout).
:- import(findall).

person("Frank").
person("Mary").
person("George").
person("Amy").

parent("Frank", "Mary").
parent("Frank", "George").
parent("Mary", "Amy").

main()
{
    findall(C, parent("Frank", C), Children);
    stdout::writeln("Frank's children: " + Children);
}
