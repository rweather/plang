:- import(stdout).

main(Args)
{
    stdout::writeln("Args: " + Args);
    if ("--bye" in Args)
        stdout::writeln("Bye World!");
    else
        stdout::writeln("Hello World!");
}
