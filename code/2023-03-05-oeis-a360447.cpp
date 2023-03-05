#include <chrono>
#include <cstdio>
#include <type_traits>

int elapsed_sec() {
    static auto start = std::chrono::steady_clock::now();
    auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(finish - start).count();
}

#ifndef MAX
 #error "Set -DMAX=100'000'000 on the command line!"
#endif

// An Int must be big enough to hold the sum MAX+MAX.

#if USE_40_BIT_INT
struct Int {
    Int() = default;
    Int(size_t x) {
        data_[0] = x;
        data_[1] = x >> 8;
        data_[2] = x >> 16;
        data_[3] = x >> 24;
        data_[4] = x >> 32;
    }
    operator size_t() const {
        size_t result = 0;
        for (int i=0; i < 5; ++i) {
            result = (result << 8) | data_[4-i];
        }
        return result;
    }
    void operator++() {
        *this = *this + 1;
    }
private:
    unsigned char data_[5];
};
#else
using Int = unsigned;
#endif

Int absdiff(Int a, Int b) { return (a < b) ? (b - a) : (a - b); }

// A List::Pos must keep pointing to the same element, even when you insert an element anywhere to the left of it.
// In the STL, this property is satisfied only by the linked lists. If you come up with another data structure
// that has this property, please tell me about it!

#if USE_STD_LIST || USE_PLF_LIST

#if USE_PLF_LIST
#include <plf_list.h>
template<class T> using mylist = plf::list<T>;
#else
#include <list>
template<class T> using mylist = std::list<T>;
#endif

class List {
public:
    // Pos points to the *second* element of the pair.
    using Pos = mylist<Int>::iterator;
    explicit List() = default;
    static Pos nosuchpos() { return Pos(); }
    Pos initialize() {
        // Initialize to {1, 2}, and return the resulting Pos.
        data_ = {1, 2};
        return std::next(data_.begin());
    }
    Int sum_at(Pos p) const {
        return *std::prev(p) + *p;
    }
    Int difference_at(Pos p) const {
        return absdiff(*std::prev(p), *p);
    }
    std::pair<Pos, Pos> insert_at(Pos p, Int i) {
        // Insert between, and return the two resulting Poses.
        p = data_.emplace(p, i);
        return {p, std::next(p)};
    }
    Pos insert_at_end(Int i) {
        // Insert at end, and return the one resulting Pos.
        data_.push_back(i);
        return std::prev(data_.end());
    }
    Int back() const { return data_.back(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
private:
    mylist<Int> data_;
};

#elif USE_STD_FORWARD_LIST
// The fastest alternative, by my measurements.
#include <forward_list>

class List {
public:
    // Pos points to the *first* element of the pair.
    using Pos = std::forward_list<Int>::iterator;
    explicit List() = default;
    static Pos nosuchpos() { return Pos(); }
    Pos initialize() {
        // Initialize to {1, 2}, and return the resulting Pos.
        data_ = {1, 2};
        back_ = std::next(data_.begin());
        return data_.begin();
    }
    Int sum_at(Pos p) const {
        return *p + *std::next(p);
    }
    Int difference_at(Pos p) const {
        return absdiff(*std::next(p), *p);
    }
    std::pair<Pos, Pos> insert_at(Pos p, Int i) {
        // Insert between, and return the two resulting Poses.
        Pos q = data_.emplace_after(p, i);
        return {p, q};
    }
    Pos insert_at_end(Int i) {
        // Insert at end, and return the one resulting Pos.
        Pos q = back_;
        back_ = data_.insert_after(q, i);
        return q;
    }
    Int back() const { return *back_; }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
private:
    std::forward_list<Int> data_;
    std::forward_list<Int>::iterator back_; // points to the last element
};

#elif USE_CUSTOM_LIST
#include <memory>

class List {
public:
    // Pos points to the *first* element of the pair.
    using SizeT = Int;
    using Pos = SizeT;
    explicit List() = default;
    static Pos nosuchpos() { return SizeT(-1); }
    Pos initialize() {
        // Initialize to {1, 2}, and return the resulting Pos.
        data_ = std::make_unique<Node[]>(MAX+1);
        data_[0] = Node(1, 1);
        data_[1] = Node(2, nosuchpos());
        end_ = &data_[2];
        back_ = 1;
        return 0;
    }
    Int sum_at(Pos p) const {
        return data_[p].value_ + data_[data_[p].next_].value_;
    }
    Int difference_at(Pos p) const {
        return absdiff(data_[p].value_, data_[data_[p].next_].value_);
    }
    std::pair<Pos, Pos> insert_at(Pos p, Int i) {
        // Insert between, and return the two resulting Poses.
        Pos q = data_[p].next_;
        data_[p].next_ = (end_ - data_.get());
        *end_++ = Node(i, q);
        return {p, data_[p].next_};
    }
    Pos insert_at_end(Int i) {
        // Insert at end, and return the one resulting Pos.
        Pos q = back_;
        data_[q].next_ = back_ = (end_ - data_.get());
        *end_++ = Node(i, nosuchpos());
        return q;
    }
    Int back() const { return data_[back_].value_; }
    auto begin() const { return Iterator{this, 0}; }
    auto end() const { return Iterator{this, nosuchpos()}; }
private:
    struct Node {
        explicit Node() = default;
        explicit Node(Int v, SizeT n) : value_(v), next_(n) {}
        Int value_;
        SizeT next_;
    };
    struct Iterator {
        const List *self_;
        SizeT pos_;
        Int operator*() const { return self_->data_[pos_].value_; }
        void operator++() { pos_ = self_->data_[pos_].next_; }
        bool operator==(const Iterator& rhs) const { return pos_ == rhs.pos_; }
    };
    std::unique_ptr<Node[]> data_;
    Node *end_;  // points one past the last element in contiguous order
    SizeT back_;  // points at the last element in linked-list order
};
#elif USE_ARRAY_LIST
// The most memory-efficient, since virtual address space is the bottleneck.
#include <memory>

class List {
public:
    // Pos points to the *first* element of the pair.
    using Pos = Int;
    explicit List() = default;
    static Pos nosuchpos() { return Int(-1); }
    Pos initialize() {
        // Initialize to {1, 2}, and return the resulting Pos.
        next_ = std::make_unique<Pos[]>(MAX+1);
        next_[0] = 1;
        next_[1] = 2;
        next_[2] = nosuchpos();
        back_ = 2;
        return 1;
    }
    Int sum_at(Pos p) const {
        return p + next_[p];
    }
    Int difference_at(Pos p) const {
        return absdiff(p, next_[p]);
    }
    std::pair<Pos, Pos> insert_at(Pos p, Int i) {
        // Insert between, and return the two resulting Poses.
        Int q = next_[p];
        next_[p] = i;
        next_[i] = q;
        return {p, i};
    }
    Pos insert_at_end(Int i) {
        // Insert at end, and return the one resulting Pos.
        Int q = std::exchange(back_, i);
        next_[q] = i;
        next_[i] = nosuchpos();
        return q;
    }
    Int back() const { return back_; }
    auto begin() const { return Iterator{this, 1}; }
    auto end() const { return Iterator{this, nosuchpos()}; }
private:
    struct Iterator {
        const List *self_;
        Int pos_;
        Int operator*() const { return pos_; }
        void operator++() { pos_ = self_->next_[pos_]; }
        bool operator==(const Iterator& rhs) const { return pos_ == rhs.pos_; }
    };
    std::unique_ptr<Int[]> next_;
    Int back_;  // holds the last element in linked-list order
};
#else
 #error "Specify a LIST implementation with -DUSE_STD_FORWARD_LIST!"
#endif

// Map just needs to support the usual STL insert/find operations.

#if USE_STD_MAP || USE_STD_UNORDERED_MAP

#if USE_STD_MAP
#include <map>
template<class K, class V> using mymap = std::map<K, V>;
#else
#include <unordered_map>
template<class K, class V> using mymap = std::unordered_map<K, V>;
#endif

class Map {
    using Iterator = mymap<Int, List::Pos>::iterator;
public:
    explicit Map(Int k, List::Pos v) {
#if USE_STD_UNORDERED_MAP
        data_.reserve(MAX / 2);
#endif
        data_.emplace(k, v);
    }
    Iterator find(Int k) { return data_.find(k); }
    bool wasnt_found(Iterator it) const { return it == data_.end(); }
    void erase(Iterator it) { data_.erase(it); }
    void emplace(Int k, List::Pos v) { data_.emplace(k, v); }
    static List::Pos *value_part(Iterator it) { return &it->second; }
private:
    mymap<Int, List::Pos> data_;
};

#elif USE_CUSTOM_MAP
#include <bit>
#include <memory>
#include <vector>

class Map {
    struct Node {
        explicit Node() = default;
        explicit Node(Int k, List::Pos v, Node *n) : key_(k), value_(v), next_(n) {}
        Int key_;
        List::Pos value_;
        Node *next_;
    };
    using Iterator = Node*;
    static constexpr size_t NBUCKETS = std::bit_ceil(size_t(MAX / 2));
public:
    explicit Map(Int k, List::Pos v) {
        data_ = std::make_unique<Node[]>(MAX / 2);
        data_[0] = Node(k, v, &data_[0]);
        bucket_[k % NBUCKETS] = &data_[0];
        end_ = &data_[1];
    }
    Iterator find(Int k) {
        Node *start = bucket_[k % NBUCKETS];
        if (start == nullptr) return nullptr;
        Node *it = start;
        while (it->key_ != k) {
            it = it->next_;
            if (it == start) return nullptr;
        }
        return it;
    }
    bool wasnt_found(Iterator it) const { return (it == nullptr); }
    void erase(Iterator it) {
        Node *&start = bucket_[it->key_ % NBUCKETS];
        if (it->next_ == it) {
            // The bucket is now empty.
            start = nullptr;
            it->next_ = std::exchange(spare_, it);
        } else {
            Node *after = it->next_;
            if (start == after) {
                start = it;
            }
            *it = *after;
            after->next_ = std::exchange(spare_, after);
        }
    }
    void emplace(Int k, List::Pos v) {
        Node *newnode = spare_ ? std::exchange(spare_, spare_->next_) : end_++;
        Node *it = std::exchange(bucket_[k % NBUCKETS], newnode);
        if (it == nullptr) {
            *newnode = Node(k, v, newnode);
        } else {
            *newnode = Node(k, v, std::exchange(it->next_, newnode));
        }
    }
    static List::Pos *value_part(Iterator it) { return &it->value_; }
private:
    std::unique_ptr<Node[]> data_;
    std::vector<Node*> bucket_ = std::vector<Node*>(NBUCKETS, nullptr);
    Node *spare_ = nullptr;
    Node *end_;
};

#elif USE_ARRAY_MAP
#include <bit>
#include <vector>

class Map {
    using Iterator = List::Pos*;
    static constexpr size_t N = std::bit_ceil(size_t(MAX/2));
public:
    explicit Map(Int k, List::Pos v) {
        data_ = std::vector<List::Pos>(N, List::nosuchpos());
        data_[k] = v;
    }
    Iterator find(Int k) {
        if (data_[k % N] == List::nosuchpos()) {
            return nullptr;
        } else {
            return &data_[k % N];
        }
    }
    bool wasnt_found(Iterator it) const { return (it == nullptr); }
    void erase(Iterator it) { *it = List::nosuchpos(); }
    void emplace(Int k, List::Pos v) { data_[k % N] = v; }
    static List::Pos *value_part(Iterator it) { return it; }
private:
    std::vector<List::Pos> data_;
};

#else
 #error "Specify a MAP implementation with -DUSE_STD_UNORDERED_MAP!"
#endif

int main() {
    List v;
    Map sums = Map(3, v.initialize());

    auto maybe_insert = [&](List::Pos p) {
        Int newsum = v.sum_at(p);
        if (newsum > MAX) {
            // We'll never reach this value of `i`, so we don't care
            return;
        }
        auto it = sums.find(newsum);
        if (sums.wasnt_found(it)) {
            sums.emplace(newsum, p);
        } else {
            List::Pos *q = sums.value_part(it);
            if (v.difference_at(p) < v.difference_at(*q)) {
                *q = p;
            }
        }
    };

    Int nextupdate = 4;
    for (Int i=3; nextupdate < MAX; ++i) {
        auto it = sums.find(i);
        if (sums.wasnt_found(it)) {
            if (i + v.back() > MAX) {
                // We'll never reach this value of `i`, so we don't care
            } else {
                List::Pos p = v.insert_at_end(i);
                maybe_insert(p);
            }
        } else {
            // Otherwise, we found the place to insert this sum. Insert it.
            List::Pos p = *sums.value_part(it);
            sums.erase(it);
            auto [p1, p2] = v.insert_at(p, i);
            maybe_insert(p1);
            maybe_insert(p2);
            if (i >= nextupdate) {
                // Print as long as no pair has a sum greater than i.
                printf("--------------------------------\n0, ");
                Int prevj = 0;
                for (Int j : v) {
                    if (prevj + j > i) {
                        nextupdate = prevj + j;
                        break;
                    }
                    printf("%zu, ", size_t(j));
                    prevj = j;
                }
                printf(" (i=%zu, t=%ds, next update at i=%zu)\n", size_t(i), elapsed_sec(), size_t(nextupdate));
            }
        }
    }
}
