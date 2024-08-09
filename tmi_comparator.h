// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Parts of this file have been copied from LLVM's libc++ and modified
// to work here. Their license applies to those functions. They are intended
// to serve only as a proof of concept for the red-black tree and should be
// rewritten asap.

#ifndef TMI_COMPARATOR_H_
#define TMI_COMPARATOR_H_

#include "tminode.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <tuple>

namespace tmi {

template <typename T, typename Node, typename Comparator, typename Parent, int I>
class tmi_comparator
{
    using node_type = Node;
    using base_type = typename node_type::base_type;
    using Color = typename base_type::Color;
    friend Parent;

    struct insert_hints {
        base_type* m_parent{nullptr};
        bool m_inserted_left{false};
    };

    struct premodify_cache{};
    static constexpr bool requires_premodify_cache() { return false; }

    Parent& m_parent;

    base_type m_roots;
    Comparator m_comparator;

    tmi_comparator(Parent& parent) : m_parent(parent){}

    base_type* get_root_base() const
    {
        return m_roots.template left<I>();
    }

    void set_root_node(node_type* node)
    {
        m_roots.template set_left<I>(node->get_base());
    }

    static void set_right(base_type* lhs, base_type* rhs)
    {
        lhs->template set_right<I>(rhs);
    }

    static void set_left(base_type* lhs, base_type* rhs)
    {
        lhs->template set_left<I>(rhs);
    }

    static base_type* get_parent(base_type* base)
    {
        return base->template parent<I>();
    }

    static void set_parent(base_type* lhs, base_type* rhs)
    {
        lhs->template set_parent<I>(rhs);
    }

    static Color get_color(base_type* base)
    {
        return base->template color<I>();
    }

    static void set_color(base_type* base, Color color)
    {
        base->template set_color<I>(color);
    }

    static base_type* get_left(base_type* base)
    {
        return base->template left<I>();
    }

    static base_type* get_right(base_type* base)
    {
        return base->template right<I>();
    }

    /*

    The below insert/erase impls were copied from libc++

    Their base type inheritance was dropped in order to come closer to
    providing a standard layout. Keeping T as he first member allows us to
    convert Between T* and node_type*. This is the trick that makes iterator_to
    work.

    Their root node trick is borrowed as well. The following is adapted from
    their comment:

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

    The algorithms taking base_type* are red black tree algorithms.  Those
    algorithms taking a parameter named root should assume that root
    points to a proper red black tree (unless otherwise specified).

    Each algorithm herein assumes that root->m_parent points to a non-null
    structure which has a member m_left which points back to root.  No other
    member is read or written to at root->m_parent.

    root->m_parent_ will be referred to below (in comments only) as m_roots[I].
    m_roots[I]->m_left is an externably accessible lvalue for root, and can be
    changed by node insertion and removal (without explicit reference to
    m_roots[I]).

    All nodes (with the exception of m_roots[I]), even the node referred to as
    root, have a non-null m_parent field.

*/

    static base_type* tree_max(base_type* x) {
      while (x->template right<I>() != nullptr)
        x = x->template right<I>();
      return x;
    }

    static bool tree_is_left_child(base_type* x)
    {
        return x == x->template parent<I>()->template left<I>();
    }

    static base_type* tree_min(base_type* x)
    {
        while (x->template left<I>() != nullptr)
            x = x->template left<I>();
        return x;
    }

    static base_type* tree_next(base_type* x)
    {
        if (x->template right<I>() != nullptr)
            return tree_min(x->template right<I>());
        while (!tree_is_left_child(x))
            x = x->template parent<I>();
        return x->template parent<I>();
    }

    static base_type* tree_prev(base_type* x) {
      if (x->template left<I>() != nullptr)
        return tree_max(x->template left<I>());
      while (tree_is_left_child(x))
        x = x->template parent<I>();
      return x->template parent<I>();
    }

    static void tree_left_rotate(base_type* x)
    {
        base_type* y = x->template right<I>();
        x->template set_right<I>(y->template left<I>());
        if (x->template right<I>() != nullptr)
            x->template right<I>()->template set_parent<I>(x);
        y->template set_parent<I>(x->template parent<I>());
        if (tree_is_left_child(x))
            x->template parent<I>()->template set_left<I>(y);
        else
            x->template parent<I>()->template set_right<I>(y);
        y->template set_left<I>(x);
        x->template set_parent<I>(y);
    }

    static void tree_right_rotate(base_type* x)
    {
        base_type* y = x->template left<I>();
        x->template set_left<I>(y->template right<I>());
        if (x->template left<I>() != nullptr)
            x->template left<I>()->template set_parent<I>(x);
        y->template set_parent<I>(x->template parent<I>());
        if (tree_is_left_child(x))
            x->template parent<I>()->template set_left<I>(y);
        else
            x->template parent<I>()->template set_right<I>(y);
        y->template set_right<I>(x);
        x->template set_parent<I>(y);
    }


    // Precondition:  root != nullptr && z != nullptr.
    //                tree_invariant(root) == true.
    //                z == root or == a direct or indirect child of root.
    // Effects:  unlinks z from the tree rooted at root, rebalancing as needed.
    // Postcondition: tree_invariant(end_node->template left<I>()) == true && end_node->template left<I>()
    //                nor any of its children refer to z.  end_node->template left<I>()
    //                may be different than the value passed in as root.
    void tree_remove(base_type* z)
    {
        base_type* root = get_root_base();
        assert(root);
        assert(z);
        // z will be removed from the tree.  Client still needs to destruct/deallocate it
        // y is either z, or if z has two children, tree_next(z).
        // y will have at most one child.
        // y will be the initial hole in the tree (make the hole at a leaf)
        base_type* y = (z->template left<I>() == nullptr || z->template right<I>() == nullptr) ?
                        z : tree_next(z);
        // x is y's possibly null single child
        base_type* x = y->template left<I>() != nullptr ? y->template left<I>() : y->template right<I>();
        // w is x's possibly null uncle (will become x's sibling)
        base_type* w = nullptr;
        // link x to y's parent, and find w
        if (x != nullptr)
            x->template set_parent<I>(y->template parent<I>());
        if (tree_is_left_child(y))
        {
            y->template parent<I>()->template set_left<I>(x);
            if (y != root)
                w = y->template parent<I>()->template right<I>();
            else
                root = x;  // w == nullptr
        }
        else
        {
            y->template parent<I>()->template set_right<I>(x);
            // y can't be root if it is a right child
            w = y->template parent<I>()->template left<I>();
        }
        bool removed_black = y->template color<I>() == Color::BLACK;
        // If we didn't remove z, do so now by splicing in y for z,
        //    but copy z's color.  This does not impact x or w.
        if (y != z)
        {
            // z->template left<I>() != nulptr but z->template right<I>() might == x == nullptr
            y->template set_parent<I>(z->template parent<I>());
            if (tree_is_left_child(z))
                y->template parent<I>()->template set_left<I>(y);
            else
                y->template parent<I>()->template set_right<I>(y);
            y->template set_left<I>(z->template left<I>());
            y->template left<I>()->template set_parent<I>(y);
            y->template set_right<I>(z->template right<I>());
            if (y->template right<I>() != nullptr)
                y->template right<I>()->template set_parent<I>(y);
            y->template set_color<I>(z->template color<I>());
            if (root == z)
                root = y;
        }
        if (removed_black && root != nullptr)
        {
            // Rebalance:
            // x has an implicit black color (transferred from the removed y)
            //    associated with it, no matter what its color is.
            // If x is root (in which case it can't be null), it is supposed
            //    to be black anyway, and if it is doubly black, then the double
            //    can just be ignored.
            // If x is red (in which case it can't be null), then it can absorb
            //    the implicit black just by setting its color to black.
            // Since y was black and only had one child (which x points to), x
            //   is either red with no children, else null, otherwise y would have
            //   different black heights under left and right pointers.
            // if (x == root || x != nullptr && !x->is_black_)
            if (x != nullptr)
                x->template set_color<I>(Color::BLACK);
            else {
                //  Else x isn't root, and is "doubly black", even though it may
                //     be null.  w can not be null here, else the parent would
                //     see a black height >= 2 on the x side and a black height
                //     of 1 on the w side (w must be a non-null black or a red
                //     with a non-null black child).
                fixup_after_remove(root, w);
            }
        }
    }

    static void tree_balance_after_insert(base_type* root, base_type* x)
    {
        x->template set_color<I>(x == root ? Color::BLACK : Color::RED);
        while (x != root && x->template parent<I>()->template color<I>() == Color::RED)
        {
            // x->template parent<I>() != root because x->template parent<I>()->is_black == false
            if (tree_is_left_child(x->template parent<I>()))
            {
                base_type* y = x->template parent<I>()->template parent<I>()->template right<I>();
                if (y != nullptr && y->template color<I>() == Color::RED)
                {
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::BLACK);
                    x = x->template parent<I>();
                    x->template set_color<I>(x == root ? Color::BLACK : Color::RED);
                    y->template set_color<I>(Color::BLACK);
                }
                else
                {
                    if (!tree_is_left_child(x))
                    {
                        x = x->template parent<I>();
                        tree_left_rotate(x);
                    }
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::BLACK);
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::RED);
                    tree_right_rotate(x);
                    break;
                }
            }
            else
            {
                base_type* y = x->template parent<I>()->template parent<I>()->template left<I>();
                if (y != nullptr && y->template color<I>() == Color::RED)
                {
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::BLACK);
                    x = x->template parent<I>();
                    x->template set_color<I>(x == root ? Color::BLACK : Color::RED);
                    y->template set_color<I>(Color::BLACK);
                }
                else
                {
                    if (tree_is_left_child(x))
                    {
                        x = x->template parent<I>();
                        tree_right_rotate(x);
                    }
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::BLACK);
                    x = x->template parent<I>();
                    x->template set_color<I>(Color::RED);
                    tree_left_rotate(x);
                    break;
                }
            }
        }
    }

    static void fixup_after_remove(base_type* root, base_type* w)
    {
        base_type* x = nullptr;
        while (true)
        {
            if (!tree_is_left_child(w))  // if x is left child
            {
                if (w->template color<I>() == Color::RED)
                {
                    w->template set_color<I>(Color::BLACK);
                    w->template parent<I>()->template set_color<I>(Color::RED);
                    tree_left_rotate(w->template parent<I>());
                    // x is still valid
                    // reset root only if necessary
                    if (root == w->template left<I>())
                        root = w;
                    // reset sibling, and it still can't be null
                    w = w->template left<I>()->template right<I>();
                }
                // w->is_black_ is now true, w may have null children
                if ((w->template left<I>()  == nullptr || w->template left<I>()->template color<I>() == Color::BLACK) &&
                    (w->template right<I>() == nullptr || w->template right<I>()->template color<I>() == Color::BLACK))
                {
                    w->template set_color<I>(Color::RED);
                    x = w->template parent<I>();
                    // x can no longer be null
                    if (x == root || x->template color<I>() == Color::RED)
                    {
                        x->template set_color<I>(Color::BLACK);
                        break;
                    }
                    // reset sibling, and it still can't be null
                    w = tree_is_left_child(x) ?
                                x->template parent<I>()->template right<I>() :
                                x->template parent<I>()->template left<I>();
                    // continue;
                }
                else  // w has a red child
                {
                    if (w->template right<I>() == nullptr || w->template right<I>()->template color<I>() == Color::BLACK)
                    {
                        // w left child is non-null and red
                        w->template left<I>()->template set_color<I>(Color::BLACK);
                        w->template set_color<I>(Color::RED);
                        tree_right_rotate(w);
                        // w is known not to be root, so root hasn't changed
                        // reset sibling, and it still can't be null
                        w = w->template parent<I>();
                    }
                    // w has a right red child, left child may be null
                    w->template set_color<I>(w->template parent<I>()->template color<I>());
                    w->template parent<I>()->template set_color<I>(Color::BLACK);
                    w->template right<I>()->template set_color<I>(Color::BLACK);
                    tree_left_rotate(w->template parent<I>());
                    break;
                }
            }
            else
            {
                if (w->template color<I>() == Color::RED)
                {
                    w->template set_color<I>(Color::BLACK);
                    w->template parent<I>()->template set_color<I>(Color::RED);
                    tree_right_rotate(w->template parent<I>());
                    // x is still valid
                    // reset root only if necessary
                    if (root == w->template right<I>())
                        root = w;
                    // reset sibling, and it still can't be null
                    w = w->template right<I>()->template left<I>();
                }
                // w->is_black_ is now true, w may have null children
                if ((w->template left<I>()  == nullptr || w->template left<I>()->template color<I>() == Color::BLACK) &&
                    (w->template right<I>() == nullptr || w->template right<I>()->template color<I>() == Color::BLACK))
                {
                    w->template set_color<I>(Color::RED);
                    x = w->template parent<I>();
                    // x can no longer be null
                    if (x->template color<I>() == Color::RED || x == root)
                    {
                        x->template set_color<I>(Color::BLACK);
                        break;
                    }
                    // reset sibling, and it still can't be null
                    w = tree_is_left_child(x) ?
                                x->template parent<I>()->template right<I>() :
                                x->template parent<I>()->template left<I>();
                    // continue;
                }
                else  // w has a red child
                {
                    if (w->template left<I>() == nullptr || w->template left<I>()->template color<I>() == Color::BLACK)
                    {
                        // w right child is non-null and red
                        w->template right<I>()->template set_color<I>(Color::BLACK);
                        w->template set_color<I>(Color::RED);
                        tree_left_rotate(w);
                        // w is known not to be root, so root hasn't changed
                        // reset sibling, and it still can't be null
                        w = w->template parent<I>();
                    }
                    // w has a left red child, right child may be null
                    w->template set_color<I>(w->template parent<I>()->template color<I>());
                    w->template parent<I>()->template set_color<I>(Color::BLACK);
                    w->template left<I>()->template set_color<I>(Color::BLACK);
                    tree_right_rotate(w->template parent<I>());
                    break;
                }
            }
        }
    }

//===----------------------------------------------------------------------===//
//
//   LLVM Code ends here
//
//===----------------------------------------------------------------------===//


    void remove_node(node_type* node)
    {
        tree_remove(node->get_base());
    }

    node_type* preinsert_node(node_type* node, insert_hints& hints)
    {
        base_type* parent = nullptr;
        base_type* curr = get_root_base();

        bool inserted_left = false;
        while (curr != nullptr) {
            parent = curr;

            if constexpr (Comparator::sorted_unique()) {
                if (m_comparator(node->value(), curr->node()->value())) {
                    curr = curr->template left<I>();
                    inserted_left = true;
                } else if (m_comparator(curr->node()->value(), node->value())) {
                    curr = curr->template right<I>();
                    inserted_left = false;
                } else {
                    return curr->node();
                }
            } else {
                if (m_comparator(node->value(), curr->node()->value())) {
                    curr = curr->template left<I>();
                    inserted_left = true;
                } else {
                    curr = curr->template right<I>();
                    inserted_left = false;
                }
            }
        }
        hints.m_inserted_left = inserted_left;
        hints.m_parent = parent;
        return nullptr;
    }

    void insert_node(node_type* node, const insert_hints& hints)
    {
        base_type* base = node->get_base();
        base_type* parent = hints.m_parent;
        if (!parent) {
            set_root_node(node);
            base->template set_parent<I>(&m_roots);
        } else if (hints.m_inserted_left) {
            base->template set_parent<I>(parent);
            parent->template set_left<I>(base);
        } else {
            base->template set_parent<I>(parent);
            parent->template set_right<I>(base);
        }
        tree_balance_after_insert(get_root_base(), base);
    }

    bool erase_if_modified(node_type* node, const premodify_cache&)
    {
        base_type* base = node->get_base();
        base_type* next_ptr = nullptr;
        base_type* prev_ptr = nullptr;
        base_type* root = get_root_base();

        if (base != tree_min(root))
            prev_ptr = tree_prev(base);
        if (base != tree_max(root))
            next_ptr = tree_next(base);

        bool needs_resort = ((next_ptr != nullptr && m_comparator(next_ptr->node()->value(), node->value())) ||
                             (prev_ptr != nullptr && m_comparator(node->value(), prev_ptr->node()->value())));
        if (needs_resort) {
            tree_remove(base);
            base->template set_parent<I>(nullptr);
            base->template set_left<I>(nullptr);
            base->template set_right<I>(nullptr);
            base->template set_color<I>(Color::RED);
            return true;
        }
        return false;
    }

    void do_clear()
    {
        m_roots = {};
    }

public:

    class iterator
    {
        node_type* m_node{};

        iterator(node_type* node) : m_node(node) {}
    public:
        friend tmi_comparator;
        friend Parent;
        typedef T value_type;
        typedef T* pointer;
        typedef T& reference;
        using element_type = T;
        iterator() = default;
        T& operator*() const { return m_node->value(); }
        T* operator->() const { return &m_node->value(); }
        iterator& operator++()
        {
            m_node = tree_next(m_node->get_base())->node();
            return *this;
        }
        iterator& operator--()
        {
            m_node = tree_prev(m_node->get_base())->node();
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy(m_node);
            ++(*this);
            return copy;
        }
        iterator operator--(int)
        {
            iterator copy(m_node);
            --(*this);
            return copy;
        }
        bool operator==(iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(iterator rhs) const { return m_node != rhs.m_node; }
    };

    class const_iterator
    {
        const node_type* m_node{};
        const_iterator(const node_type* node) : m_node(node) {}

    public:
        typedef const T value_type;
        typedef const T* pointer;
        typedef const T& reference;
        using element_type = const T;
        const_iterator() = default;
        const T& operator*() const { return m_node->value(); }
        const T* operator->() const { return &m_node->value(); }
        const_iterator& operator++()
        {
            m_node = tree_next(m_node->get_base())->m_node();
            return *this;
        }
        const_iterator& operator--()
        {
            m_node = tree_prev(m_node->get_base())->node();
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator copy(m_node);
            ++(*this);
            return copy;
        }
        const_iterator operator--(int)
        {
            const_iterator copy(m_node);
            --(*this);
            return copy;
        }
        bool operator==(const_iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(const_iterator rhs) const { return m_node != rhs.m_node; }
    };

    template <typename... Args>
    std::pair<iterator,bool> emplace(Args&&... args)
    {
        auto [node, success] = m_parent.do_emplace(std::forward<Args>(args)...);
        return std::make_pair(iterator(node), success);
    }

    iterator begin() const
    {
        base_type* root = get_root_base();
        if (root == nullptr)
            return iterator(nullptr);
        return iterator(tree_min(root)->node());
    }

    iterator end() const
    {
        return iterator(nullptr);
    }


    iterator iterator_to(const T& entry) const
    {
        T& ref = const_cast<T&>(entry);
        node_type* node = reinterpret_cast<node_type*>(&ref);
        return iterator(node);
    }

    template <typename Callable>
    bool modify(iterator it, Callable&& func)
    {
        node_type* node = it.m_node;
        if (!node) return false;
        return m_parent.do_modify(node, std::forward<Callable>(func));
    }

    iterator find(const T& value) const
    {
        base_type* parent = nullptr;
        base_type* curr = get_root_base();
        while (curr != nullptr) {
            parent = curr;
            if (m_comparator(value, curr->node()->value())) {
                curr = curr->template left<I>();
            } else if (m_comparator(curr->node()->value(), value)) {
                curr = curr->template right<I>();
            } else {
                return iterator(curr->node());
            }
        }
        return end();
    }

    iterator erase(iterator it)
    {
        node_type* node = it.m_node;
        if (!node) return iterator(nullptr);
        node = tree_next(node->get_base())->node();
        iterator ret(node);
        m_parent.do_erase(node);
        return ret;
    }

    void clear()
    {
        m_parent.do_clear();
    }

private:

    iterator make_iterator(node_type* node) const
    {
        if (node) {
            return iterator(node);
        }
        return end();
    }

};

} // namespace tmi

#endif // TMI_COMPARATOR_H_
