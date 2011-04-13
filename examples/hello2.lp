:- import(stdout).

main(Args)
{
    if ("--bye" in Args)
        stdout::writeln("Bye World!");
    else
        stdout::writeln("Hello World!");
}
