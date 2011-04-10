
Plang logic programming language
================================

Plang (pronounced "P lang") is an experimental logic programming
language, which borrows the backtracking search of Prolog,
but with a completely different C-style syntax on top.
The aim is to explore alternatives to traditional languages
that can help make logic programming more attractive for
practical programming tasks.

## Obtaining Plang

The sources for Plang are available from the project
[git repository](https://github.com/rweather/plang).

## Dependencies

### libgc

Plang uses [libgc](http://www.hpl.hp.com/personal/Hans_Boehm/gc/)
to perform mark-and-sweep garbage collection on terms, clauses,
and other engine control structures.

The libgc library is required and must be installed on your
system prior to building Plang.  Your GNU/Linux distribution
probably already has a package for it: try "libgc" and
"libgc-devel" (or "libgc-dev").  You will need the "devel"
package to build Plang.

### WordNet

Plang can make use of the [WordNet](http://wordnet.princeton.edu/)
lexical database to provide access to huge amounts of information
about English words.  Natural language processing systems in
particular can benefit from being able to ask "Is this a
noun/verb/adverb/etc?".

Many GNU/Linux distributions have WordNet packages already.  You can
try installing "wordnet" and "wordnet-devel" (or "wordnet-dev")
to see if your distribution already has WordNet.  If not, download
and build it from the sources at the above URL.

Note: WordNet support is optional in Plang, but highly recommended.
If the "configure" script does not detect WordNet, then it will
not build the "words" module.  You will need the "devel" package
installed to build Plang with WordNet.

### Autotools

Plang uses automake, autoconf, libtool, etc for its build system.

## Building Plang

Once you have checked out the sources from the repository,
type the following to build and install it under /usr/local
on your system:

    $ ./auto_gen.sh
    $ ./configure
    $ make
    $ sudo make install

You can also type "make check" to run the unit tests before
the "make install" step to verify that Plang is working correctly.

The "make docs" rule will build the documentation.  It is assumed
that you have "doxygen" installed on your PATH to do this.

## Staying up to date with Plang

The [Southern Storm](http://southern-storm.blogspot.com/) blog
in the main place to find announcements about Plang progress.
Or contact the author via e-mail (use "git log" to find the
e-mail address).
