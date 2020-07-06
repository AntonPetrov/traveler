
/*
 * File: gted.cpp
 *
 * Copyright (C) 2016 Richard Eliáš <richard.elias@matfyz.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */


#include "gted.hpp"
#include "mapping.hpp"


using namespace std;

#define BAD                 0xBADF00D

#define get_table(str, tblname) \
(str.is_left() ? (tblname).left : \
(str.is_right() ? (tblname).right : \
(tblname).heavy))

#define valid(iter) (rna_tree::is_valid(iter))


gted::gted(
           const rna_tree& _t1,
           const rna_tree& _t2)
: t1(_t1), t2(_t2)
{ }

void gted::run(
               const strategy_table_type& _str)
{
    APP_DEBUG_FNAME;
    
    INFO("BEG: Running GTED for RNAs %s and %s", t1.name(), t2.name());
    
    STR = _str;
    
    check_ids_postorder();
    
    tdist.resize(t1.size(), vector<size_t>(t2.size(), BAD));
    
    compute_distance_recursive(t1.begin(), t2.begin());
    
    INFO("Computed Tree-Edit-Distance between RNAs: tdist[%s][%s] = %s",
         label(t1.begin()), label(t2.begin()),
         tdist[id(t1.begin())][id(t2.begin())]);
    
    INFO("END: Running GTED for RNAs %s and %s", t1.name(), t2.name());
}

void gted::compute_distance_recursive(
                                      iterator root1,
                                      iterator root2)
{
    // using keyroots
    t1.check_same_tree(root1);
    t2.check_same_tree(root2);
    
    strategy str = STR[id(root1)][id(root2)];
    
    if (str.is_heavy())
    {
        // heavy decomposition is not implemented. Use left or right one.
        vector<rted_strategy> strategies = {RTED_T1_LEFT, RTED_T1_RIGHT, RTED_T2_LEFT, RTED_T2_RIGHT};
        str = strategy(strategies.at(rand() % strategies.size()));
    }
    actual_str = str;
    
    if (str.is_T1())
    {
        for (const auto& val :
             get_table(actual_str, t1.get_keyroots(root1)))
        {
            compute_distance_recursive(val, root2);
        }
    }
    else
    {
        for (const auto& val :
             get_table(actual_str, t2.get_keyroots(root2)))
        {
            compute_distance_recursive(root1, val);
        }
    }
    
    actual_str = str;
    single_path_function(root1, root2);
}

void gted::single_path_function(
                                iterator root1,
                                iterator root2)
{
    // using subforests
    
    if (actual_str.is_T1())
    {
        for (const auto& val :
             get_table(actual_str, t2.get_subforests(root2)))
            compute_distance(root1, val);
    }
    else
    {
        for (const auto& val :
             get_table(actual_str, t1.get_subforests(root1)))
            compute_distance(val, root2);
    }
    
    compute_distance(root1, root2);
}

gted::forest_distance_table_type gted::compute_distance(
                                                        iterator root1,
                                                        iterator root2)
{
    tree_type *t1ptr = &t1;
    tree_type *t2ptr = &t2;
    forest_distance_table_type table;
    
    if (actual_str.is_T2())
    {
        // if T2 -> iterate with T2's iterators first..
        swap(t1ptr, t2ptr);
        swap(root1, root2);
    }
    
    t1ptr->check_same_tree(root1);
    t2ptr->check_same_tree(root2);
    
    if (actual_str.is_left())
    {
        auto leaf_funct = [](tree_type& t, iterator root) {
            return t.get_leafs(root).left;
        };
        table = compute_distance_LR<post_order_iterator>(
                                                         root1, root2, *t1ptr, *t2ptr, leaf_funct);
    }
    else if (actual_str.is_right())
    {
        auto leaf_funct = [](const tree_type& t, const iterator& root) {
            return t.get_leafs(root).right;
        };
        table = compute_distance_LR<rev_post_order_iterator> (
                                                              root1, root2, *t1ptr, *t2ptr, leaf_funct);
    }
    
    return table;
}

template <typename iterator_type, typename funct_get_begin>
gted::forest_distance_table_type gted::compute_distance_LR(
                                                           iterator root1,
                                                           iterator root2,
                                                           tree_type& t1,
                                                           tree_type& t2,
                                                           funct_get_begin get_begin_leaf)
{
    // subtree has id-s (id(leafs.left) ... id(root1))
    
    forest_distance_table_type fdist(
                                     t1.get_size(root1) + 1,
                                     vector<size_t>(t2.get_size(root2) + 1, BAD));
    vector<size_t> vec(3);
    
    iterator_type it1, it2;
    const iterator_type empty;
    const iterator_type beg1 = get_begin_leaf(t1, root1);
    const iterator_type beg2 = get_begin_leaf(t2, root2);
    const iterator_type end1 = ++iterator_type(root1);
    const iterator_type end2 = ++iterator_type(root2);
    const size_t id1 = id(t1.get_leafs(root1).left);
    const size_t id2 = id(t2.get_leafs(root2).left);
    
    
#define prev(num) \
((it ##num == beg ##num) ? empty : --iterator_type(it ##num))
#define set_fdist(iter1, iter2, value) \
set_fdist(fdist, iter1, iter2, value, id1, id2)
#define get_fdist(iter1, iter2) \
get_fdist(fdist, iter1, iter2, id1, id2)
    
    set_fdist(empty, empty, 0);
    for (it1 = beg1; it1 != end1; ++it1)
        set_fdist(it1, empty, get_fdist(prev(1), empty) + costs::del(it1));
    for (it2 = beg2; it2 != end2; ++it2)
        set_fdist(empty, it2, get_fdist(empty, prev(2)) + costs::ins(it2));
    
    for (it1 = beg1; it1 != end1; ++it1)
    {
        for (it2 = beg2; it2 != end2; ++it2)
        {
            size_t min;
            bool b = get_begin_leaf(t1, it1) == beg1 &&
            get_begin_leaf(t2, it2) == beg2;
            
            vec[0] = get_fdist(prev(1), it2) + costs::del(it1); // delete `it1`
            vec[1] = get_fdist(it1, prev(2)) + costs::del(it2); // delete `it2`

            // modify iff both it-s are subtree roots
            if (b)
                vec[2] = get_fdist(prev(1), prev(2)) + costs::upd(it1, it2); // modify `it1` ~> `it2`
            else
            {
                // previous subtree visited root == --leaf
                //
                iterator_type prev_root1 = get_begin_leaf(t1, it1);
                iterator_type prev_root2 = get_begin_leaf(t2, it2);

                if (prev_root1 != beg1)
                    --prev_root1;
                else
                    prev_root1 = empty;
                if (prev_root2 != beg2)
                    --prev_root2;
                else
                    prev_root2 = empty;

                vec[2] = get_tdist(it1, it2) + get_fdist(prev_root1, prev_root2);
                // ^^ if (prev_root != empty) =>
                // prev_root is in sibling branch of it
                // and we computed this subtree yet
            }

            min = *min_element(vec.begin(), vec.end());
            
            set_fdist(it1, it2, min);
            if (b) // i am in subtree roots
                set_tdist(it1, it2, min);
        }
    }
    
    return fdist;
    
#undef set_fdist
#undef get_fdist
#undef prev
}

mapping gted::get_mapping()
{
    APP_DEBUG_FNAME;
    
    INFO("BEG: Computing mapping between RNAs %s and %s",
         t1.name(), t2.name());
    
    typedef post_order_iterator iterator_type;
    
    check_ids_postorder();
    
    mapping map;
    vector<pair<iterator_type, iterator_type>> to_be_matched;
    forest_distance_table_type fdist;
    const iterator_type empty;
    size_t id1, id2;
    iterator_type root1, root2, it1, it2, beg1, beg2;
    
    auto get_begin_leaf =
    [](const tree_type& t, const iterator_type& root) {
        return t.get_leafs(root).left;
    };
    auto compute_distance_local =
    [this](const iterator_type& root1, const iterator_type& root2) {
        tree_distance_table_type oldtdist = tdist;
        
        auto fdist = compute_distance(root1, root2);
        
        assert(tdist == oldtdist);
        return fdist;
    };
    auto get_left_subtree =
    [&empty](iterator_type iter, const iterator_type& root) {
        while (tree_type::is_first_child(iter) && iter != root)
            iter = tree_type::parent(iter);
        
        if (iter == root)
            iter = empty;
        else if (!tree_type::is_first_child(iter))
            iter = --sibling_iterator(iter);
        else
            iter = empty;
        
        return iter;
    };
    
    to_be_matched.push_back({t1.begin(), t2.begin()});
    actual_str = strategy(RTED_T1_LEFT);
    
    while (!to_be_matched.empty())
    {
        root1 = to_be_matched.back().first;
        root2 = to_be_matched.back().second;
        to_be_matched.pop_back();
        
        DEBUG("Matching subtrees: \n%s\n%s",
              tree_type::print_subtree(root1, false),
              tree_type::print_subtree(root2, false));
        
        fdist = compute_distance_local(root1, root2);
        
        beg1 = get_begin_leaf(t1, root1);
        beg2 = get_begin_leaf(t2, root2);
        
        id1 = id(beg1);
        id2 = id(beg2);
        
        it1 = root1;
        it2 = root2;
        
#define prev(num) \
((it ##num == beg ##num) ? empty : --iterator_type(it ##num))
#define get_fdist(iter1, iter2) \
get_fdist(fdist, iter1, iter2, id1, id2)
        while (tree_type::is_valid(it1) || tree_type::is_valid(it2))
        {
            if (tree_type::is_valid(it1) &&
                get_fdist(prev(1), it2) + costs::del(it1) ==
                get_fdist(it1, it2))
            {
                DEBUG("delete %s:%u", label(it1), id(it1));
                
                map.map.push_back({id(it1) + 1, 0});
                
                it1 = prev(1);
            }
            else if (tree_type::is_valid(it2) &&
                     get_fdist(it1, prev(2)) + costs::ins(it2) ==
                     get_fdist(it1, it2))
            {
                DEBUG("insert %s:%u", label(it2), id(it2));
                
                map.map.push_back({0, id(it2) + 1});
                
                it2 = prev(2);
            }
            else
            {
                if (get_begin_leaf(t1, it1) == beg1 &&
                    get_begin_leaf(t2, it2) == beg2)
                {
                    DEBUG("match %s:%u -> %s:%u",
                          label(it1), id(it1),
                          label(it2), id(it2));
                    
                    map.map.push_back({id(it1) + 1, id(it2) + 1});
                    
                    it1 = prev(1);
                    it2 = prev(2);
                }
                else
                {
                    DEBUG("To be matched:\n%s\n%s",
                          tree_type::print_subtree(it1, false),
                          tree_type::print_subtree(it2, false));
                    
                    to_be_matched.push_back({it1, it2});
                    
                    it1 = get_left_subtree(it1, root1);
                    it2 = get_left_subtree(it2, root2);
                }
            }
        }
        assert(!tree_type::is_valid(it1) && !tree_type::is_valid(it2));
    }
    
    assert(t1.size() + map.get_to_insert().size() ==
           t2.size() + map.get_to_remove().size());
    
    map.distance = map.get_to_insert().size() + map.get_to_remove().size();
    
    sort(map.map.begin(), map.map.end());
    
    INFO("END: Computing mapping between RNAs %s and %s",
         t1.name(), t2.name());
    
    
    return map;
    
#undef prev
#undef get_fdist
}





/* inline */ size_t gted::get_tdist(
                                    iterator it1,
                                    iterator it2)
{
    assert(valid(it1) && valid(it2));
    
    size_t i1, i2, out;
    
    if (actual_str.is_T2())
        swap(it1, it2);
    t1.check_same_tree(it1);
    t2.check_same_tree(it2);
    
    i1 = id(it1);
    i2 = id(it2);
    
    assert(i1 < tdist.size() && i2 < tdist[i1].size());
    
    out = tdist[i1][i2];
    
    assert(out != BAD);
    
    return out;
}

/* inline */ void gted::set_tdist(
                                  iterator it1,
                                  iterator it2,
                                  size_t value)
{
    assert(valid(it1) && valid(it2));
    
    size_t i1, i2;
    
    if (actual_str.is_T2())
        swap(it1, it2);
    t1.check_same_tree(it1);
    t2.check_same_tree(it2);
    
    i1 = id(it1);
    i2 = id(it2);
    
    assert(i1 < tdist.size() && i2 < tdist[i1].size());
    assert(value != BAD);
    
    tdist[i1][i2] = value;
}

/* inline */ size_t gted::get_fdist(
                                    const forest_distance_table_type& fdist,
                                    const iterator& it1,
                                    const iterator& it2,
                                    size_t idleft1,
                                    size_t idleft2)
{
    size_t i1, i2, out;
    
    i1 = valid(it1) ? id(it1) - idleft1 + 1 : 0;
    i2 = valid(it2) ? id(it2) - idleft2 + 1 : 0;
    
    assert((int)i1 >= 0 && (int)i2 >= 0);
    assert(i1 < fdist.size() && i2 < fdist[i1].size());
    
    out = fdist[i1][i2];
    
    assert(out != BAD);
    return out;
}

/* inline */ void gted::set_fdist(
                                  forest_distance_table_type& fdist,
                                  const iterator& it1,
                                  const iterator& it2,
                                  size_t value,
                                  size_t idleft1,
                                  size_t idleft2)
{
    size_t i1, i2;
    
    i1 = valid(it1) ? id(it1) - idleft1 + 1 : 0;
    i2 = valid(it2) ? id(it2) - idleft2 + 1 : 0;
    
    assert((int)i1 >= 0 && (int)i2 >= 0);
    assert(i1 < fdist.size() && i2 < fdist[i1].size());
    
    fdist[i1][i2] = value;
}

/* inline */ void gted::check_ids_postorder()
{
    size_t i;
    
    i = 0;
    for (auto it = t1.begin_post(); it != t1.end_post(); ++it, ++i)
        assert(id(it) == i);
    i = 0;
    for (auto it = t2.begin_post(); it != t2.end_post(); ++it, ++i)
        assert(id(it) == i);
}





// costs
#define GTED_COST_MODIFY    0
#define GTED_COST_DELETE    1
#define GTED_COST_INSERT    1
#define GTED_COST_ROOT      10000

#define get_cost(iter, value) \
(rna_tree::is_root(iter) ? GTED_COST_ROOT : value)

/* static */ size_t gted::costs::del(
                                     iterator it)
{
    return get_cost(it, GTED_COST_DELETE);
}

/* static */ size_t gted::costs::ins(
                                     iterator it)
{
    return get_cost(it, GTED_COST_INSERT);
}

/* static */ size_t gted::costs::upd(
                                     iterator it1,
                                     iterator it2)
{
    // if one of them is root, but second is not (update root to unrooted node)
    if (rna_tree::is_root(it1) != rna_tree::is_root(it2) || it1->paired() != it2->paired())
        return GTED_COST_ROOT;
    else
        return GTED_COST_MODIFY;
}

