/*
  Readable AVL tree implementation with in-line proofs of correctness.

  Author: Madars Virza <madars@mit.edu> (c) 2024
  License: MIT
 */
#include <algorithm>
#include <iostream>
#include <cassert>
#include <queue>
#include <random>
#include <string>
#include <vector>

using KeyType = int;
using ValueType = std::string;

#define DEBUG

/*
  document:
  - that this works for duplicates
  - unless otherwise specified also handles root == nullptr everywhere
  - that this only uses operator<
 */

struct AVLnode {
    KeyType K_;
    ValueType V_;

    AVLnode *left_, *right_, *parent_;
    bool heavy_; // if this node is a heavy (height+2) subtree for its parent

#ifdef DEBUG
    int height_ = 0;
#endif
    // consider adding a constructor that sets parent set
    AVLnode(KeyType K, ValueType V) : K_(K), V_(V), left_(nullptr), right_(nullptr), parent_(nullptr), heavy_(false) {}
};

// return node or nullptr if not found
AVLnode *find(AVLnode *root, KeyType K) {
    AVLnode *cur = root;
    while (cur != nullptr) {
        if (K < cur->K_) {
            cur = cur->left_;
        } else if (cur->K_ < K) {
            cur = cur->right_;
        } else {
            break;
        }
    }

    return cur;
}

// insert the node in the tree. this correctly inserts the new
// node, except for rebalancing/updating heights.

// does not handle root == nullptr
AVLnode *insert_without_rebalancing(AVLnode *root, KeyType K, ValueType V) {
    AVLnode *cur = root;
    while (1) {
        if (K < cur->K_) {
            if (cur->left_ != nullptr) {
                cur = cur->left_;
                continue;
            }
            
            cur->left_ = new AVLnode(K, V);
            cur->left_->parent_ = cur->left_;
            cur = cur->left_;
            break;
        } else {
            if (cur->right_ != nullptr) {
                cur = cur->right_;
                continue;
            }
            
            cur->right_ = new AVLnode(K, V);
            cur->right_->parent_ = cur->right_;
            cur = cur->right_;
            break;
        }
    }
    return cur;
}

AVLnode *insert(AVLnode *root, KeyType K, ValueType V) {
    // easy case
    if (root == nullptr) {
        return new AVLnode(K, V);
    }

    // rebalance
    AVLnode *cur = insert_without_rebalancing(root, K, V);

    return root;
}

AVLnode *remove(AVLnode *it) {
    return it;
}

/*
  obtain the height via walking the longest path; does not use DEBUG height_ annotation
 */
int height(AVLnode *root) {
    if (root == nullptr) return -1;
    
    AVLnode *cur = root;
    int H = 0;
    while (cur->left_ != nullptr || cur->right_ != nullptr) {
        if (cur->left_ != nullptr && cur->left_->heavy_) {
            H += 2;
            cur = cur->left_;
        } else if (cur->right_ != nullptr && cur->right_->heavy_) {
            H += 2;
            cur = cur->right_;
        } else {
            cur = (cur->left_ != nullptr ? cur->left_ : cur->right_);
            H += 1;
        }
    }

#ifdef DEBUG
    assert(root->height_ == H);
#endif

    return H;
}

/*
  recursively verify that:
  - tree is properly ordered (left <= root <= right)
  - height = 0 for leaves, as computed by decrementing expected
    (longest path) height across the path to each particular node by 1
    (not-heavy)/2 (heavy) as appropriate
  - if DEBUG is set also verify that explicitly stored height is consistent
  - at most one of the subtrees is heavy (this implies tree is balanced)
  - parent pointers are correctly set

  also verifies: no loops.
  does not verify: that indegree of every node <= 1
*/
void verify_inner(AVLnode *root, AVLnode *expected_parent, int expected_height) {
    if (root == nullptr) {
        assert(expected_parent == nullptr);
        assert(expected_height == -1);
        
    }
    assert(expected_height >= 0);
#ifdef DEBUG
    assert(root->height_ == expected_height);
#endif

    assert(root->parent_ == expected_parent);

    // leaf
    if (root->left_ == nullptr && root->right_ == nullptr) {
        assert(expected_height == 0);
        return;
    }

    // has siblings, verify that at most one is heavy, verify order, verify parent pointers, recurse in each
    assert(root->left_ == nullptr || root->right_ == nullptr || !(root->left_->heavy_) || !(root->right_->heavy_));
    if (root->left_ != nullptr) {
        assert(!(root->K_ < root->left_->K_));
        verify_inner(root->left_, root, expected_height - (root->left_->heavy_ ? 2 : 1));
    }
    if (root->right_ != nullptr) {
        assert(!(root->right_->K_ < root->K_));
        verify_inner(root->right_, root, expected_height - (root->right_->heavy_ ? 2 : 1));
    }
}

void verify(AVLnode *root) {
    if (root != nullptr) assert(root->heavy_ == false); // while heavy is only defined for non-root nodes, we set it to false for root by convention
    verify_inner(root, nullptr, height(root));
}

void show(AVLnode *root, std::string prefix="") {
    if (root == nullptr) {
        std::cout << prefix << "nil\n";
        return;
    }
    show(root->right_, prefix + "     ");
    //std::cout << prefix << "(" << root->K_ << ", " << root->V_ << ") h=" << root->height_ << "\n";
    std::cout << prefix << root->K_ << " h=" << root->height_ << "\n";
    show(root->left_, prefix + "     ");
}

int main() {
    AVLnode *root = nullptr;
    for (auto i : {2,1,3,4,0}) {
        std::cout << "insert " << i << "\n";
        root = insert(root, i, "abc");
        show(root);
        std::cout << "============\n";
    }
/*    std::random_device rd;
    std::mt19937 g(rd());

    for (int i = 0; i < 1000; ++i) {
        AVLnode *root = nullptr;
        const int N = 10;

        std::vector<int> insert_order(N), remove_order(N);
        std::iota(insert_order.begin(), insert_order.end(), 0);
        std::iota(remove_order.begin(), remove_order.end(), 0);

        std::shuffle(insert_order.begin(), insert_order.end(), g);
        std::shuffle(remove_order.begin(), remove_order.end(), g);

        for (auto i : insert_order) {
            char cc[2] = {(char)('A' + i), 0};
            root = insert(root, i, std::string(cc));
            
            verify(root);
        }

        for (auto i : remove_order) {
            AVLnode *el = find(root, i);
            root = remove(el);
            
            verify(root);
        }

        assert(root == nullptr);
    }
*/
}
