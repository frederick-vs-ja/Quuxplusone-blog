#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// This "should be enough for anyone." Don't assume n=MAXN will actually work.
// If you want to test larger n, increase MAXN.
#define MAXN 32

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
        static_assert(2*MAXN < CHAR_MAX, "All coords must fit into the positive 'signed char' values");
        struct { signed char x, y, z, pad=0; };
        unsigned int i;
    };
    explicit Pt() {}
    Pt(int x, int y, int z) : x(x), y(y), z(z) {}
    bool operator==(const Pt& p) const { return i == p.i; }
    bool adjacentTo(const Pt& p) const {
        return std::has_single_bit(i - p.i) || std::has_single_bit(p.i - i);
    }
    struct Less {
        bool operator()(const Pt& a, const Pt& b) const { return a.i < b.i; }
    };
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

bool is_canonical_form(std::string_view sv)
{
    if (sv[0] != 'S') return false;
    size_t udlr = sv.find_first_of("UDLR");
    return (udlr == sv.npos || sv[udlr] == 'R');
}

std::string_view trace_snake(std::span<const Pt> visited, Facing rf)
{
    const int n = visited.size() - 1;
    static char rs[MAXN] = "S";
    assert(rf.step(visited[0]) == visited[1]);
    Pt pos = visited[1];
    for (int i = 1; i < n; ++i) {
        const Pt& nextpos = visited[i+1];
        // How do we get from pos to nextpos?
        if (rf.step(pos) == nextpos) {
            rs[i] = 'S';
        } else if (rf.left().step(pos) == nextpos) {
            rs[i] = 'L';
            rf = rf.left();
        } else if (rf.right().step(pos) == nextpos) {
            rs[i] = 'R';
            rf = rf.right();
        } else if (rf.up().step(pos) == nextpos) {
            rs[i] = 'U';
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            rs[i] = 'D';
            rf = rf.down();
        }
        pos = nextpos;
    }
    return std::string_view(rs, rs + n);
}

std::string_view trace_snake_backwards(std::span<const Pt> visited, Facing rf)
{
    const int n = visited.size() - 1;
    static char rs[MAXN] = "S";
    assert(rf.step(visited[n]) == visited[n-1]);
    Pt pos = visited[n-1];
    for (int i = 1; i < n; ++i) {
        const Pt& nextpos = visited[n-i-1];
        // How do we get from pos to nextpos?
        if (rf.step(pos) == nextpos) {
            rs[i] = 'S';
        } else if (rf.left().step(pos) == nextpos) {
            rs[i] = 'L';
            rf = rf.left();
        } else if (rf.right().step(pos) == nextpos) {
            rs[i] = 'R';
            rf = rf.right();
        } else if (rf.up().step(pos) == nextpos) {
            rs[i] = 'U';
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            rs[i] = 'D';
            rf = rf.down();
        }
        pos = nextpos;
    }
    return std::string_view(rs, rs + n);
}

struct Floodfiller {
    explicit Floodfiller() = default;

    void generate_corner_neighbors(std::span<const Pt> visited, int dx, int dy, int dz) {
        for (const auto& [x, y, z] : visited) {
            Pt p1 = { x + dx, y, z };
            Pt p2 = { x, y + dy, z };
            Pt p3 = { x, y, z + dz };
            if (std::find(visited.begin(), visited.end(), p1) == visited.end()) *expected_last++ = p1;
            if (std::find(visited.begin(), visited.end(), p2) == visited.end()) *expected_last++ = p2;
            if (std::find(visited.begin(), visited.end(), p3) == visited.end()) *expected_last++ = p3;
        }
    }

    void reset_things(Pt *visited, int n) {
        this->n = n;
        Pt *visited_last = visited + n;
        expected_last = expected;

        struct Deltas { int dx, dy, dz; };
        if (true) {
            // Each cavity must contain at least one empty cell bordered by snake on these three faces,
            // which will show up here as a threepeat in the sorted list of rookwise neighbors.
            // If no such cell exists, this snake can't have a cavity.
            // (This also starts building the "expected" volume we're going to flood-fill.)
            generate_corner_neighbors(std::span(visited, n), -1, -1, -1);
            std::sort(expected, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(expected, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), +1, +1, +1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), -1, -1, +1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), -1, +1, -1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), -1, +1, +1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), +1, -1, -1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), +1, -1, +1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            Pt *first = expected_last;
            generate_corner_neighbors(std::span(visited, n), +1, +1, -1);
            std::sort(first, expected_last, Pt::Less());
            cannot_have_cavities_ = !has_threepeat(first, expected_last);
            if (cannot_have_cavities_) {
                return;
            }
            expected_last = first;
        }
        if (true) {
            // Now build the rest of the "expected" list.
            static constexpr Deltas other_neighbors[20] = {
                {-1, -1, -1},
                {-1, -1,  0},
                {-1, -1, +1},
                {-1,  0, -1},
                {-1,  0, +1},
                {-1, +1, -1},
                {-1, +1,  0},
                {-1, +1, +1},
                { 0, -1, -1},
                { 0, -1, +1},
                { 0, +1, -1},
                { 0, +1, +1},
                {+1, -1, -1},
                {+1, -1,  0},
                {+1, -1, +1},
                {+1,  0, -1},
                {+1,  0, +1},
                {+1, +1, -1},
                {+1, +1,  0},
                {+1, +1, +1},
            };
            for (const auto& [x, y, z] : std::span(visited, n)) {
                for (const auto& [dx, dy, dz] : other_neighbors) {
                    Pt p = { x + dx, y + dy, z + dz };
                    if (std::find(visited, visited_last, p) != visited_last) continue;
                    assert(expected_last < std::end(expected));
                    *expected_last++ = p;
                }
            }
            std::sort(expected, expected_last, Pt::Less());
            expected_last = std::unique(expected, expected_last);
        }
    }

    void flood() {
        assert(!cannot_have_cavities_);
        std::fill(flooded, flooded + sheath_volume(), false);
        flood(expected[0]);
    }

    size_t flooded_volume() const {
        return std::count(flooded, flooded + sheath_volume(), true);
    }
    size_t sheath_volume() const {
        return expected_last - expected;
    }

    bool cannot_have_cavities() const { return cannot_have_cavities_; }

private:
    template<class It>
    bool has_threepeat(It first, It last) {
        assert(last - first >= 3);
        for (; first+2 != last; ++first) {
            if (first[0] == first[1] && first[1] == first[2]) {
                return true;
            }
        }
        return false;
    }

    void flood(Pt p) {
        auto it = std::find(expected, expected_last, p);
        if (it != expected_last && !flooded[it - expected]) {
            // Flood it and visit its neighbors.
            flooded[it - expected] = true;
            flood(Pt(p.x + 1, p.y, p.z));
            flood(Pt(p.x - 1, p.y, p.z));
            flood(Pt(p.x, p.y + 1, p.z));
            flood(Pt(p.x, p.y - 1, p.z));
            flood(Pt(p.x, p.y, p.z + 1));
            flood(Pt(p.x, p.y, p.z - 1));
        }
    }

    int n = 0;
    bool cannot_have_cavities_ = false;
    Pt *expected_last = nullptr;
    Pt expected[25*MAXN+2];
    bool flooded[25*MAXN+2];
};
static Floodfiller g_floodfiller;

bool cannot_have_cavities(std::string_view sv, Pt *visited, int n) {
    auto is_turning = [](char ch) { return ch != 'S'; };
    if (std::count_if(sv.begin(), sv.end(), is_turning) < 6) return true;
    int minx = INT_MAX;
    int miny = INT_MAX;
    int minz = INT_MAX;
    int maxx = INT_MIN;
    int maxy = INT_MIN;
    int maxz = INT_MIN;
    for (int i=0; i < n; ++i) {
        minx = std::min(minx, get<0>(visited[i]));
        miny = std::min(miny, get<1>(visited[i]));
        minz = std::min(minz, get<2>(visited[i]));
        maxx = std::max(maxx, get<0>(visited[i]));
        maxy = std::max(maxy, get<1>(visited[i]));
        maxz = std::max(maxz, get<2>(visited[i]));
    }
    int dx = (maxx - minx) + 1;
    int dy = (maxy - miny) + 1;
    int dz = (maxz - minz) + 1;
    // There must be space for a cavity.
    if (dx < 3 || dy < 3 || dz < 3) return true;
    // There must be cubes enough to surround a cavity.
    // The shortest cavitous snake fits 11 cubes into a 3x3x3 box.
    if ((n-11 < dx-3) || (n-11 < dy-3) || (n-11 < dz-3)) return true;
    return false;
}

bool has_cavities(std::string_view sv, Pt *visited, int n) {
    if (cannot_have_cavities(sv, visited, n)) {
        return false;
    }
    g_floodfiller.reset_things(visited, n);
    if (g_floodfiller.cannot_have_cavities()) {
        return false;
    }
    g_floodfiller.flood();
    return (g_floodfiller.flooded_volume() != g_floodfiller.sheath_volume());
}

enum SnakeOutcome {
    NOT_A_SNAKE,
    FREE_STRIP,
    FREE_STRIP_OUROBOROS,
    ONESIDED_STRIP,
    ONESIDED_STRIP_OUROBOROS,
    FREE_CAVITOUS_SNAKE,
    FREE_CAVITOUS_OUROBOROS,
    ONESIDED_CAVITOUS_SNAKE,
    ONESIDED_CAVITOUS_OUROBOROS,
};

static bool vertically_mirrored_is_less_than(std::string_view a, std::string_view b)
{
    assert(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        char mirrored = (a[i] == 'U') ? 'D' : (a[i] == 'D') ? 'U' : a[i];
        if (mirrored != b[i]) return mirrored < b[i];
    }
    return false;
}

SnakeOutcome testOuroboros(std::string_view sv, Pt *pv)
{
    if (sv[1] == 'S') {
        // SSSRXYZ is always less than SRXYZVW.
        return NOT_A_SNAKE;
    }
    const int n = sv.size() + 1;
    auto visited = std::span<Pt>(pv, pv + n);
    bool mirroredIsLess = vertically_mirrored_is_less_than(sv, sv);

    for (int t=1; t < n; ++t) {
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
        for (int i=0; i < 24; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0]) && visited[2] == rf.right().step(visited[1])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return NOT_A_SNAKE;
                }
                if (vertically_mirrored_is_less_than(rs, sv)) {
                    mirroredIsLess = true;
                }
            }
        }
    }
    std::reverse(visited.begin(), visited.end());
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 24; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0]) && visited[2] == rf.right().step(visited[1])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return NOT_A_SNAKE;
                }
                if (vertically_mirrored_is_less_than(rs, sv)) {
                    mirroredIsLess = true;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }

    if (mirroredIsLess) {
        return has_cavities(sv, pv, n) ? ONESIDED_CAVITOUS_OUROBOROS : ONESIDED_STRIP_OUROBOROS;
    } else {
        return has_cavities(sv, pv, n) ? FREE_CAVITOUS_OUROBOROS : FREE_STRIP_OUROBOROS;
    }
}

SnakeOutcome testSnake(std::string_view sv, int *self_intersection)
{
    const size_t n = sv.size();
    Facing f = Facing(0);
    Facing rf = f.right().right();
    Pt pos = {MAXN, MAXN, MAXN};
    static Pt visited[MAXN];
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
                *self_intersection = i;
                return NOT_A_SNAKE;
            }
        }
        visited[i] = pos;
        pos = nextpos;
    }
    visited[n] = pos;

    std::string_view rs = trace_snake_backwards(std::span<Pt>(visited, n+1), rf);
    if (rs < sv) {
        return NOT_A_SNAKE;
    } else if (vertically_mirrored_is_less_than(sv, sv) || vertically_mirrored_is_less_than(rs, sv)) {
        return has_cavities(sv, visited, n+1) ? ONESIDED_CAVITOUS_SNAKE : ONESIDED_STRIP;
    } else {
        return has_cavities(sv, visited, n+1) ? FREE_CAVITOUS_SNAKE : FREE_STRIP;
    }
}

struct Odometer {
    static void increment(char& ch) {
        if (ch == 'D') ch = 'S';
        else if (ch == 'S') ch = 'R';
        else if (ch == 'R') ch = 'L';
        else if (ch == 'L') ch = 'U';
        else ch = 'D';
    }
    static void fast_forward(std::string& s, int i) {
        const int n = s.size();
        for (int j = i+1; j < n; ++j) s[j] = 'D';
    }
    static bool advance(std::string& s) {
        const int n = s.size();
        int i = n - 1;
        do {
            increment(s[i--]);
        } while (s[i+1] == 'S');
        if (s[i+1] == 'L' && s[i] == 'S' && s.find_first_not_of('S') == size_t(i+1)) {
            // Have we just made SSSSLSSSSS?
            // Replace it with   SSSRSSSSSS.
            if (i == 0) {
                return false;  // Done!
            }
            s[i] = 'R';
            s[i+1] = 'S';
        }
        if (s[i] == s[i+1] && s[i] != 'S') {
            // We just created LL, RR, DD, or UU:
            // substrings that can't appear in a valid snake.
            // (Except for the trivial 4-cube ouroboros SRR.)
            fast_forward(s, i+1);
            return advance(s);
        }
        return true;
    }
};

int main(int argc, char **argv)
{
    int n = (argc < 2 || atoi(argv[1]) < 3) ? 3 : atoi(argv[1]);
    unit_test_facings();

    printf("| n  | Strings | Free non-ouroboros snakes | Free non-ouroboros snakes with cavities | Free ouroboroi | Free ouroboroi with cavities "
           "| One-sided non-ouroboros snakes | One-sided non-ouroboros snakes with cavities | One-sided ouroboroi | One-sided ouroboroi with cavities |\n");

    for (; true; ++n) {
        size_t nStrings = 0;
        size_t nFreeSnakesWithCavities = 0;
        size_t nFreeSnakesWithoutCavities = 0;
        size_t nFreeOuroboroiWithCavities = 0;
        size_t nFreeOuroboroiWithoutCavities = 0;
        size_t nChiralSnakesWithCavities = 0;
        size_t nChiralSnakesWithoutCavities = 0;
        size_t nChiralOuroboroiWithCavities = 0;
        size_t nChiralOuroboroiWithoutCavities = 0;
        size_t tick = 0;
        auto elapsed_while_asleep = std::chrono::seconds(0);
        auto last_elapsed = std::chrono::seconds(0);
        auto start = std::chrono::system_clock::now();

        auto print_stats = [&](char newline) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);
            if (elapsed - last_elapsed > std::chrono::seconds(30)) {
                // The computer probably went to sleep. Don't count this interval.
                elapsed_while_asleep += (elapsed - last_elapsed);
                elapsed = last_elapsed;
                start = std::chrono::system_clock::now() - elapsed;
            } else {
                last_elapsed = elapsed;
            }
            printf("| %d | %zu | %zu | %zu | %zu | %zu | %zu | %zu | %zu | %zu | (%zu sec, %zu sec asleep)%c",
                n, nStrings,
                nFreeSnakesWithCavities + nFreeSnakesWithoutCavities, nFreeSnakesWithCavities,
                nFreeOuroboroiWithCavities + nFreeOuroboroiWithoutCavities, nFreeOuroboroiWithCavities,
                nFreeSnakesWithCavities + nFreeSnakesWithoutCavities + nChiralSnakesWithCavities + nChiralSnakesWithoutCavities, nFreeSnakesWithCavities + nChiralSnakesWithCavities,
                nFreeOuroboroiWithCavities + nFreeOuroboroiWithoutCavities + nChiralOuroboroiWithCavities + nChiralOuroboroiWithoutCavities, nFreeOuroboroiWithCavities + nChiralOuroboroiWithCavities,
                size_t(elapsed.count()), size_t(elapsed_while_asleep.count()), newline
            );
            fflush(stdout);
        };

        std::string s(n-1, 'S');
        int self_intersection_idx = -1;
        do {
            assert(is_canonical_form(s));
            nStrings += 1;
            switch (testSnake(s, &self_intersection_idx)) {
                default:
                    assert(false);
                    break;
                case NOT_A_SNAKE:
                    if (self_intersection_idx != -1) {
                        Odometer::fast_forward(s, self_intersection_idx);
                        self_intersection_idx = -1;
                    }
                    break;
                case FREE_CAVITOUS_OUROBOROS:
                    nFreeOuroboroiWithCavities += 1;
                    break;
                case FREE_STRIP_OUROBOROS:
                    nFreeOuroboroiWithoutCavities += 1;
                    break;
                case ONESIDED_CAVITOUS_OUROBOROS:
                    nChiralOuroboroiWithCavities += 1;
                    break;
                case ONESIDED_STRIP_OUROBOROS:
                    nChiralOuroboroiWithoutCavities += 1;
                    break;
                case FREE_STRIP:
                    nFreeSnakesWithoutCavities += 1;
                    break;
                case ONESIDED_STRIP:
                    nChiralSnakesWithoutCavities += 1;
                    break;
                case FREE_CAVITOUS_SNAKE:
                    nFreeSnakesWithCavities += 1;
                    break;
                case ONESIDED_CAVITOUS_SNAKE:
                    nChiralSnakesWithCavities += 1;
                    break;
            }
            if (++tick == 1000000) {
                tick = 0;
                print_stats('\r');
            }
        } while (Odometer::advance(s));
        print_stats('\n');
    }
}
