:- import(stdout).
:- import(findall).

person("Frank").
person("Mary").
person("George").
person("Amy").

main(Args)
{
    findall(P, person(P), People);
    stdout::writeln("People: " + People);
}
