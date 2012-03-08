/*
 * plang logic programming language
 * Copyright (C) 2011,2012  Southern Storm Software, Pty Ltd.
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

#include "testcase.h"
#include "rbtree-priv.h"

P_TEST_DECLARE();

/* Very simple random number generator for creating a repeatable
 * sequence of random numbers for the tests */
static unsigned int seed = 314159265;
static int test_rand(int range)
{
    seed = seed * 1103515245 + 12345;
    return (int)(seed % (unsigned int)range);
}

/* Generate a random sequence of numbers between 0 and 1023
 * while generating each number only once */
static int values[1024];
static int left_to_go;
static void reset_seq(void)
{
    int index;
    for (index = 0; index < 1024; ++index)
        values[index] = index;
    left_to_go = 1024;
}
static int next_seq(void)
{
    int posn, value;
    if (left_to_go <= 1)
        return values[0];
    posn = test_rand(left_to_go);
    value = values[posn];
    values[posn] = values[left_to_go - 1];
    --left_to_go;
    return value;
}

static int max_node_height(const p_rbnode *node)
{
    int lheight = 1;
    int rheight = 1;
    if (node->left)
        lheight += max_node_height(node->left);
    if (node->right)
        rheight += max_node_height(node->right);
    if (lheight > rheight)
        return lheight;
    else
        return rheight;
}
static int max_tree_height(const p_rbtree *tree)
{
    if (tree->root)
        return max_node_height(tree->root);
    else
        return 0;
}

static int min_node_height(const p_rbnode *node, int current, int value)
{
    int temp;
    if (!node)
        return current;
    temp = min_node_height(node->left, current + 1, value);
    if (temp < value)
        value = temp;
    temp = min_node_height(node->right, current + 1, value);
    if (temp < value)
        value = temp;
    return value;
}
static int min_tree_height(const p_rbtree *tree)
{
    if (tree->root)
        return min_node_height(tree->root, 0, 10000);
    else
        return 0;
}

static void test_key_init()
{
    p_rbkey key;
    p_term *name;
    p_term *term;

    P_VERIFY(!_p_rbkey_init(&key, 0));
    P_VERIFY(!_p_rbkey_init(&key, p_term_create_variable(context)));

    name = p_term_create_atom(context, "foo");
    P_VERIFY(_p_rbkey_init(&key, name));
    P_COMPARE(key.type, P_TERM_ATOM);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, name);

    term = p_term_create_functor(context, name, 2);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_FUNCTOR);
    P_COMPARE(key.size, 2);
    P_COMPARE(key.name, name);

    term = p_term_create_list(context, 0, 0);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_LIST);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, 0);

    term = p_term_create_list(context, name, 0);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_LIST);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, 0);

    term = p_term_create_list
        (context, p_term_create_variable(context), 0);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_LIST);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, 0);

    term = p_term_create_list
        (context, p_term_create_functor(context, name, 2), 0);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_LIST);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, 0);

    term = p_term_create_string(context, "bar");
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_STRING);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, term);

    term = p_term_create_real(context, 1.5);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_REAL);
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, term);

    term = p_term_create_integer(context, 15);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_COMPARE(key.type, P_TERM_INTEGER);
#if defined(P_TERM_64BIT)
    P_COMPARE(key.size, 15);
    P_COMPARE(key.name, 0);
#else
    P_COMPARE(key.size, 0);
    P_COMPARE(key.name, term);
#endif
}

p_term *_p_context_test_goal(p_context *context);

static p_term *parse_term(const char *source)
{
    _p_context_test_goal(context);          /* Allow goal saving */
    if (p_context_consult_string(context, source) != 0)
        return 0;
    return _p_context_test_goal(context);   /* Fetch test goal */
}

#define TERM(x) ("\?\?-- " x ".\n")

static int fix_compare(const p_term *term1, const p_term *term2, int result)
{
    if (result != -2)
        return result;
    if (p_term_type(term1) == P_TERM_LIST) {
        term1 = p_term_deref(term1->list.head);
        term2 = p_term_deref(term2->list.head);
    }
    if (p_term_type(term1) == P_TERM_FUNCTOR) {
        if (term1->header.size < term2->header.size)
            return -1;
        else if (term1->header.size > term2->header.size)
            return -1;
        term1 = term1->functor.functor_name;
        term2 = term2->functor.functor_name;
    }
    if (term1 < term2)
        return -1;
    else if (term1 > term2)
        return 1;
    else
        return 0;
}

static int same_compare(int cmp1, int cmp2)
{
    if (cmp1 < 0)
        return cmp2 < 0;
    else if (cmp1 > 0)
        return cmp2 > 0;
    else
        return cmp2 == 0;
}

static void test_key_compare()
{
    struct compare_type
    {
        const char *row;
        const char *term1;
        const char *term2;
        int result;
    };
    static struct compare_type const compare_data[] = {
        {"atom_1", TERM("a"), TERM("b"), -2},
        {"atom_2", TERM("b"), TERM("a"), -2},
        {"atom_3", TERM("a"), TERM("a"), 0},

        {"functor_1", TERM("f(a)"), TERM("f(b)"), 0},
        {"functor_2", TERM("f(a, b)"), TERM("f(b)"), 1},
        {"functor_3", TERM("f(a)"), TERM("f(a, b)"), -1},
        {"functor_4", TERM("f(a)"), TERM("g(a)"), -2},
        {"functor_5", TERM("g(a)"), TERM("f(a)"), -2},

        {"string_1", TERM("\"a\""), TERM("\"b\""), -1},
        {"string_2", TERM("\"b\""), TERM("\"a\""), 1},
        {"string_3", TERM("\"a\""), TERM("\"a\""), 0},

        {"real_1", TERM("1.5"), TERM("2.5"), -1},
        {"real_2", TERM("2.5"), TERM("1.5"), 1},
        {"real_3", TERM("2.5"), TERM("2.5"), 0},

        {"integer_1", TERM("1"), TERM("2"), -1},
        {"integer_2", TERM("2"), TERM("1"), 1},
        {"integer_3", TERM("2"), TERM("2"), 0},

        {"atom_integer_1", TERM("a"), TERM("2"), -1},
        {"atom_integer_2", TERM("2"), TERM("a"), 1},

        {"list_1", TERM("[H1|T1]"), TERM("[H2|T2]"), 0},
        {"list_2", TERM("[[a]|T1]"), TERM("[[b]|T2]"), 0},

        {"list_of_atom_1", TERM("[a|T]"), TERM("[b|T]"), 0},
        {"list_of_atom_2", TERM("[b|T]"), TERM("[a|T]"), 0},
        {"list_of_atom_3", TERM("[a|T]"), TERM("[a|T]"), 0},

        {"list_of_functor_1", TERM("[f(a)|T]"), TERM("[f(b)|T]"), 0},
        {"list_of_functor_2", TERM("[f(a, b)|T]"), TERM("[f(b)|T]"), 0},
        {"list_of_functor_3", TERM("[f(a)|T]"), TERM("[f(a, b)|T]"), 0},
        {"list_of_functor_4", TERM("[f(a)|T]"), TERM("[g(a)|T]"), 0},
        {"list_of_functor_5", TERM("[g(a)|T]"), TERM("[f(a)|T]"), 0},

        {"list_of_string_1", TERM("[\"a\"|T]"), TERM("[\"b\"|T]"), 0},
        {"list_of_string_2", TERM("[\"b\"|T]"), TERM("[\"a\"|T]"), 0},
        {"list_of_string_3", TERM("[\"a\"|T]"), TERM("[\"a\"|T]"), 0},

        {"list_of_real_1", TERM("[1.5|T]"), TERM("[2.5|T]"), 0},
        {"list_of_real_2", TERM("[2.5|T]"), TERM("[1.5|T]"), 0},
        {"list_of_real_3", TERM("[2.5|T]"), TERM("[2.5|T]"), 0},

        {"list_of_integer_1", TERM("[1|T]"), TERM("[2|T]"), 0},
        {"list_of_integer_2", TERM("[2|T]"), TERM("[1|T]"), 0},
        {"list_of_integer_3", TERM("[2|T]"), TERM("[2|T]"), 0},

        {"list_of_atom_integer_1", TERM("[a|T]"), TERM("[2|T]"), 0},
        {"list_of_atom_integer_2", TERM("[2|T]"), TERM("[a|T]"), 0},
    };
    #define compare_data_len (sizeof(compare_data) / sizeof(struct compare_type))

    size_t index;
    p_term *term1;
    p_term *term2;
    int compare_result;
    int expected_result;
    p_rbkey key1;
    p_rbkey key2;
    for (index = 0; index < compare_data_len; ++index) {
        P_TEST_SET_ROW(compare_data[index].row);
        term1 = parse_term(compare_data[index].term1);
        term2 = parse_term(compare_data[index].term2);
        P_VERIFY(_p_rbkey_init(&key1, term1));
        P_VERIFY(_p_rbkey_init(&key2, term2));
        expected_result = fix_compare
            (term1, term2, compare_data[index].result);
        compare_result = _p_rbkey_compare_keys(&key1, &key2);
        P_VERIFY(same_compare(compare_result, expected_result));
    }
}

/* Create a tree by random insertion */
static void create_random_tree(p_rbtree *tree)
{
    int value, value2;
    p_term *term;
    p_rbkey key;
    p_rbnode *node;

    reset_seq();
    for (value = 0; value < 1024; ++value) {
        value2 = next_seq();

        term = p_term_create_integer(context, value2);
        P_VERIFY(_p_rbkey_init(&key, term));

        node = _p_rbtree_insert(tree, &key);
        P_VERIFY(node != 0);
        P_VERIFY(node->value == 0);

        node->value = term;
    }
}

static void test_insert()
{
    p_rbtree tree;
    int value;
    p_term *term;
    p_rbkey key;
    p_rbnode *node;
    p_rbnode *node2;

    _p_rbtree_init(&tree);
    P_VERIFY(!(tree.root));

    /* Insert values in order, generating a worst-case tree */
    for (value = 0; value < 1024; ++value) {
        term = p_term_create_integer(context, value);
        P_VERIFY(_p_rbkey_init(&key, term));

        node = _p_rbtree_insert(&tree, &key);
        P_VERIFY(node != 0);
        P_VERIFY(node->value == 0);

        node->value = term;
    }

    /* Worst case height should be no more than 2 * log2(N + 1)
     * which is about 20.00282 for N = 1024.  See Wikipedia for
     * more on the worst-case height:
     *      http://en.wikipedia.org/wiki/Red-black_tree */
    P_VERIFY(max_tree_height(&tree) <= 21);

    /* Check that everything was added correctly */
    for (value = 1023; value >= 0; --value) {
        term = p_term_create_integer(context, value);
        P_VERIFY(_p_rbkey_init(&key, term));

        node = _p_rbtree_lookup(&tree, &key);
        P_VERIFY(node != 0);
        P_COMPARE(p_term_integer_value(node->value), value);
    }

    /* Search for a value not in the tree */
    term = p_term_create_integer(context, 2048);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_VERIFY(!_p_rbtree_lookup(&tree, &key));

    /* Clean up the tree */
    _p_rbtree_free(&tree);
    P_VERIFY(!(tree.root));

    /* Add values in random order to test arbitrary insertions */
    create_random_tree(&tree);

    /* We expect a smaller maximum height this time */
    P_VERIFY(max_tree_height(&tree) <= 12);

    /* Check that everything was added correctly, and that
     * inserting an entry with the same value will return
     * the pre-existing node */
    for (value = 1023; value >= 0; --value) {
        term = p_term_create_integer(context, value);
        P_VERIFY(_p_rbkey_init(&key, term));

        node = _p_rbtree_lookup(&tree, &key);
        P_VERIFY(node != 0);
        P_COMPARE(p_term_integer_value(node->value), value);

        node2 = _p_rbtree_insert(&tree, &key);
        P_COMPARE(node2, node);
    }

    /* Clean up the tree */
    _p_rbtree_free(&tree);
    P_VERIFY(!(tree.root));
}

static void test_remove()
{
    p_rbtree tree;
    int value;
    int value2;
    p_term *term;
    p_rbkey key;

    _p_rbtree_init(&tree);
    P_VERIFY(!(tree.root));

    /* Add the values in random order to create the initial tree */
    create_random_tree(&tree);

    /* Remove all elements from the tree in order */
    for (value = 0; value < 1024; ++value) {
        term = p_term_create_integer(context, value);
        P_VERIFY(_p_rbkey_init(&key, term));

        term = _p_rbtree_remove(&tree, &key);
        P_VERIFY(term != 0);
        P_COMPARE(p_term_integer_value(term), value);
    }

    /* The tree should now be empty */
    P_VERIFY(!(tree.root));

    /* Create a random tree again and then remove randomly */
    create_random_tree(&tree);
    reset_seq();
    for (value = 0; value < 1024; ++value) {
        value2 = next_seq();
        term = p_term_create_integer(context, value2);
        P_VERIFY(_p_rbkey_init(&key, term));

        term = _p_rbtree_remove(&tree, &key);
        P_VERIFY(term != 0);
        P_COMPARE(p_term_integer_value(term), value2);
    }
    P_VERIFY(!(tree.root));

    /* Create a tree, chop the middle out of it, and then check
     * that the tree still appears to be balanced.  Balanced in
     * this case is defined as "no more than 4 in difference
     * between the minimum and maximum height" and "the maximum
     * tree height has actually decreased */
    create_random_tree(&tree);
    value2 = max_tree_height(&tree);
    P_VERIFY((value2 - min_tree_height(&tree)) <= 4);
    for (value = 128; value < 896; ++value) {
        term = p_term_create_integer(context, value);
        P_VERIFY(_p_rbkey_init(&key, term));
        _p_rbtree_remove(&tree, &key);
    }
    value = max_tree_height(&tree);
    P_VERIFY((value - min_tree_height(&tree)) <= 4);
    P_VERIFY(value < value2);

    /* Try to remove something that isn't in the tree any more */
    term = p_term_create_integer(context, 512);
    P_VERIFY(_p_rbkey_init(&key, term));
    P_VERIFY(!_p_rbtree_remove(&tree, &key));

    /* Clean up the tree */
    _p_rbtree_free(&tree);
    P_VERIFY(!(tree.root));
}

static void test_visit_all()
{
    p_rbtree tree;
    char visited[1024];
    int value;
    int count;
    p_rbnode *next;

    /* Create a random tree */
    _p_rbtree_init(&tree);
    create_random_tree(&tree);

    /* Clear the visit flags for all values */
    for (value = 0; value < 1024; ++value)
        visited[value] = 0;

    /* Visit all of the tree nodes */
    next = 0;
    count = 0;
    while ((next = _p_rbtree_visit_all(&tree, next)) != 0) {
        visited[p_term_integer_value(next->value)] = 1;
        ++count;
    }

    /* Verify that we visited every node once and only once */
    P_COMPARE(count, 1024);
    for (value = 0; value < 1024; ++value)
        P_VERIFY(visited[value]);

    /* Clean up the tree */
    _p_rbtree_free(&tree);
    P_VERIFY(!(tree.root));
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-rbtree");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(key_init);
    P_TEST_RUN(key_compare);
    P_TEST_RUN(insert);
    P_TEST_RUN(remove);
    P_TEST_RUN(visit_all);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
