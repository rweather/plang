/*
 * plang logic programming language
 * Copyright (C) 2011  Southern Storm Software, Pty Ltd.
 *
 * The plang package is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * The plang package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libcompiler library.  If not,
 * see <http://www.gnu.org/licenses/>.
 */

:- import(testcase).

class foo
{
};

new_object()
{
    new foo (F);
}

decl_errors()
{
    verify_error(new_class(Name, [], [], []), instantiation_error);
    verify_error(new_class(1.5, [], [], []), type_error(atom, 1.5));
    verify_error(new_class(foo, [], [], []), permission_error(create, class, foo));
    verify_error(new_class(baz, Parent, [], []), instantiation_error);
    verify_error(new_class(baz, 1.5, [], []), type_error(atom, 1.5));
    verify_error(new_class(baz, bay, [], []), existence_error(class, bay));
}

main(_)
{
    init("test-class");
    run(new_object);
    run(decl_errors);
    report();
}
