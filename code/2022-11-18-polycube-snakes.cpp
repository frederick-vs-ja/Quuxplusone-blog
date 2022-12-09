#include <bit>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#if USE_COROUTINES
#include "generator.h"
#endif

// A polycube snake is given by looking down one of its ends
// and recording which way you "turn" each time you move
// to the next cube; so a polysnake of n cubes is represented
// by a string of (n-1) letters.
//
// To canonicalize a directed polysnake, turn it so that the string
// begins with "S" and the first non-"S" letter (if any) is "R".
//
// A polysnake should be non-self-intersecting and
// non-self-adjoining.

struct Pt {
    union {
        struct { signed char x, y, z, pad=0; };
        unsigned int i;
    };
    explicit Pt() {}
    Pt(int x, int y, int z) : x(x), y(y), z(z) {}
    bool operator==(const Pt& p) const { return i == p.i; }
    bool adjacentTo(const Pt& p) const {
        return std::has_single_bit(i - p.i) || std::has_single_bit(p.i - i);
    }
};
template<> struct std::tuple_size<Pt> : std::integral_constant<size_t, 3> {};
template<size_t I> struct std::tuple_element<I, Pt> : std::type_identity<int> {};
template<size_t I> int get(const Pt& p) requires (I == 0) { return p.x; }
template<size_t I> int get(const Pt& p) requires (I == 1) { return p.y; }
template<size_t I> int get(const Pt& p) requires (I == 2) { return p.z; }

struct Facing {
    int value_ = 0;
    explicit Facing(int i) : value_(i) {}
    friend bool operator==(Facing, Facing) = default;
    Facing left() const {
        int a[] = {
            4, 17, 14, 23,  8, 18,  2, 22,  12, 19,  6, 21,
            0, 16, 10, 20,  7, 11, 15,  3,   5,  1, 13,  9,
        };
        return Facing(a[value_]);
    }
    Facing right() const {
        return left().left().left();
    }
    Facing down() const {
        int a[] = {
            20, 5, 18, 15,  23, 9, 19,  3,  22, 13, 16,  7,
            21, 1, 17, 11,   0, 4,  8, 12,  10,  6,  2, 14,
        };
        return Facing(a[value_]);
    }
    Facing up() const {
        return down().down().down();
    }
    Facing twistCW() const {
        return Facing((value_ & ~3) | ((value_ + 1) & 3));
    }
    Facing twistCCW() const {
        return twistCW().twistCW().twistCW();
    }
    Facing undoLeft() const {
        // If we're facing `f` before a left turn, then how should we face
        // in the other direction to make that turn into an initial right?
        return right();
    }
    Facing undoRight() const {
        return left().twistCW().twistCW();
    }
    Facing undoUp() const {
        return down().twistCW();
    }
    Facing undoDown() const {
        return up().twistCCW();
    }
    Pt step(Pt t) const {
        auto [x, y, z] = t;
        x += (12 <= value_ && value_ < 16) - (4 <= value_ && value_ < 8);
        y += (0 <= value_ && value_ < 4) - (8 <= value_ && value_ < 12);
        z += (16 <= value_ && value_ < 20) - (20 <= value_ && value_ < 24);
        return {x, y, z};
    }
};

void unit_test_facings()
{
    for (int i=0; i < 24; ++i) {
        Facing f = Facing(i);
        (void)f;
        assert(f == f.right().right().right().right());
        assert(f.left() == f.right().right().right());
        assert(f.left().left() == f.right().right());
        assert(f.left().left().left() == f.right());
        assert(f.left().left().left().left() == f);
        assert(f == f.up().up().up().up());
        assert(f.down() == f.up().up().up());
        assert(f.down().down() == f.up().up());
        assert(f.down().down().down() == f.up());
        assert(f.down().down().down().down() == f);
        assert(f == f.twistCW().twistCW().twistCW().twistCW());
        assert(f.twistCCW() == f.twistCW().twistCW().twistCW());
        assert(f.twistCCW().twistCCW() == f.twistCW().twistCW());
        assert(f.twistCCW().twistCCW().twistCCW() == f.twistCW());
        assert(f.twistCCW().twistCCW().twistCCW().twistCCW() == f);
        assert(f.down().right().right().down().right().right() == f);
    }
}

bool is_valid_snake(std::string_view sv)
{
    Facing f = Facing(0);
    Pt pos = {60, 60, 60};
    std::vector<Pt> visited;
    for (char ch : sv) {
        switch (ch) {
            case 'S': break;
            case 'L': f = f.left(); break;
            case 'R': f = f.right(); break;
            case 'U': f = f.up(); break;
            case 'D': f = f.down(); break;
        }
        Pt nextpos = f.step(pos);
        // nextpos must have no neighbors besides pos.
        for (const auto& pt : visited) {
            if (nextpos.adjacentTo(pt)) return false;
        }
        visited.push_back(pos);
        pos = nextpos;
    }
    return true;
}

bool is_canonical_form(std::string_view sv)
{
    if (sv[0] != 'S') return false;
    size_t udlr = sv.find_first_of("UDLR");
    return (udlr == sv.npos || sv[udlr] == 'R');
}

std::string trace_snake(std::span<const Pt> visited, Facing rf)
{
    std::string rs = std::string(visited.size() - 1, 'S');
    Pt pos = visited[0];
    assert(rf.step(pos) == visited[1]);
    for (size_t i=1; i < visited.size(); ++i) {
        const Pt& nextpos = visited[i];
        // How do we get from pos to nextpos?
        if (rf.step(pos) == nextpos) {
            rs[i-1] = 'S';
        } else if (rf.left().step(pos) == nextpos) {
            rs[i-1] = 'L';
            rf = rf.left();
        } else if (rf.right().step(pos) == nextpos) {
            rs[i-1] = 'R';
            rf = rf.right();
        } else if (rf.up().step(pos) == nextpos) {
            rs[i-1] = 'U';
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            rs[i-1] = 'D';
            rf = rf.down();
        }
        pos = nextpos;
    }
    return rs;
}

enum SnakeOutcome {
    NOT_A_SNAKE,
    OUROBOROS,
    DIRECTED,
    UNDIRECTED,
};

SnakeOutcome testOuroboros(std::string_view sv, Pt *pv)
{
    if (sv[1] == 'S') {
        // SSSRXYZ is always less than SRXYZVW.
        return NOT_A_SNAKE;
    }
    const int n = sv.size() + 1;
    auto visited = std::span<Pt>(pv, pv + n);
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 24; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                if (is_canonical_form(rs) && rs < sv) {
                    return NOT_A_SNAKE;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    std::reverse(visited.begin(), visited.end());
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 24; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                if (is_canonical_form(rs) && rs < sv) {
                    return NOT_A_SNAKE;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    return OUROBOROS;
}

SnakeOutcome testSnake(std::string_view sv)
{
    const size_t n = sv.size();
    Facing f = Facing(0);
    Facing rf = f.right().right();
    Pt pos = {60, 60, 60};
    static Pt visited[32];
    for (size_t i = 0; i < n; ++i) {
        switch (sv[i]) {
            case 'S': break;
            case 'L': rf = f.undoLeft();  f = f.left();  break;
            case 'R': rf = f.undoRight(); f = f.right(); break;
            case 'U': rf = f.undoUp();    f = f.up();    break;
            case 'D': rf = f.undoDown();  f = f.down();  break;
        }
        Pt nextpos = f.step(pos);
        // nextpos must have no neighbors besides pos.
        for (int j = i-4; j >= 0; j -= 2) {
            if (nextpos.adjacentTo(visited[j])) {
                if (j == 0 && i == n-1) {
                    visited[n-1] = pos;
                    visited[n] = nextpos;
                    return testOuroboros(sv, visited);
                }
                return NOT_A_SNAKE;
            }
        }
        visited[i] = pos;
        pos = nextpos;
    }
    for (size_t i=0; i < n; ++i) {
        char c = sv[i];
#define COMPARE(rc) do { if (c != rc) return (c < rc) ? UNDIRECTED : DIRECTED; } while (0)
        const Pt& nextpos = visited[n - i - 1];
        // How do we get from pos to nextpos?
        if (rf.step(pos) == nextpos) {
            COMPARE('S');
        } else if (rf.left().step(pos) == nextpos) {
            COMPARE('L');
            rf = rf.left();
        } else if (rf.right().step(pos) == nextpos) {
            COMPARE('R');
            rf = rf.right();
        } else if (rf.up().step(pos) == nextpos) {
            COMPARE('U');
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            COMPARE('D');
            rf = rf.down();
        }
        pos = nextpos;
    }
    return UNDIRECTED;
}

bool odometer(std::string& s)
{
    int n = s.size();
    auto increment = [](char& ch) {
        if (ch == 'D') ch = 'S';
        else if (ch == 'S') ch = 'R';
        else if (ch == 'R') ch = 'L';
        else if (ch == 'L') ch = 'U';
        else ch = 'D';
    };
again:
    int i = n - 1;
    do {
        increment(s[i--]);
    } while (s[i+1] == 'S');
    if (s[i+1] == 'L' && s[i] == 'S') {
        // Have we just made SSSSLSSSSS?
        // Replace it with   SSSRSSSSSS.
        if (s.find_first_not_of('S') == size_t(i+1)) {
            if (i == 0) {
                return false;  // Done!
            }
            s[i] = 'R';
            s[i+1] = 'S';
        }
    }
    if (s[i] == s[i+1] && s[i] != 'S') {
        // We just created LL, RR, DD, or UU:
        // substrings that can't appear in a valid snake.
        // (Except for the trivial 4-cube ouroboros SRR.)
        // If we just made XYZRRSSS, change it to XYZRLSSS.
        // If we just made XYZLLSSS, change it to XYZLUSSS.
        // If we just made XYZUUSSS, change it to XYZUDSSS.
        // If we just made XYZDDSSS, change it to XYZDDDDD and increment.
        increment(s[i+1]);
        if (s[i+1] == 'S') {
            for (int j=i; j < n; ++j) s[j] = 'D';
            goto again;
        }
    }
    return true;
}

#if USE_COROUTINES
generator<std::string_view> strings_of_length(int n)
{
    // Odometer algorithm, with digits "SRLUD" in that order.
    std::string s(n, 'S');
    co_yield s;
    while (odometer(s)) {
        co_yield s;
    }
    co_return;
}
#endif

int main(int argc, char **argv)
{
    int n = (argc < 2 || atoi(argv[1]) < 3) ? 3 : atoi(argv[1]);
    unit_test_facings();
    printf("| n | Strings | Directed | Undirected | Ouroboroi |\n");
    for (; true; ++n) {
        size_t sc = 0;
        size_t uc = 0;
        size_t dc = 0;
        size_t oc = 0;
        size_t tick = 0;
        auto elapsed_while_asleep = std::chrono::seconds(0);
        auto last_elapsed = std::chrono::seconds(0);
        auto start = std::chrono::system_clock::now();
#if USE_COROUTINES
        for (std::string_view s : strings_of_length(n-1)) {
#else
        std::string s(n-1, 'S');
        do {
#endif
            assert(is_canonical_form(s));
            sc += 1;
            SnakeOutcome outcome = testSnake(s);
            if (outcome == OUROBOROS) {
                oc += 1;
            } else if (outcome == DIRECTED) {
                dc += 1;
            } else if (outcome == UNDIRECTED) {
                dc += 1;
                uc += 1;
            }
            if (++tick == 1000000) {
                tick = 0;
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);
                if (elapsed - last_elapsed > std::chrono::seconds(30)) {
                    // The computer probably went to sleep. Don't count this interval.
                    elapsed_while_asleep += (elapsed - last_elapsed);
                    elapsed = last_elapsed;
                    start = std::chrono::system_clock::now() - elapsed;
                } else {
                    last_elapsed = elapsed;
                }
                if (elapsed_while_asleep.count() > 0) {
                    printf("| %d | %zu | %zu | %zu | %zu | (%zu sec, %zu sec asleep)\r", n, sc, dc, uc, oc, size_t(elapsed.count()), size_t(elapsed_while_asleep.count()));
                } else {
                    printf("| %d | %zu | %zu | %zu | %zu | (%zu sec)\r", n, sc, dc, uc, oc, size_t(elapsed.count()));
                }
                fflush(stdout);
            }
#if USE_COROUTINES
        }
#else
        } while (odometer(s));
#endif
        printf("| %d | %zu | %zu | %zu | %zu |\n", n, sc, dc, uc, oc);
    }
}
