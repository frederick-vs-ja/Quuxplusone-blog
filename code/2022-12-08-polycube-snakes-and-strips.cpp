#include <bit>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

std::string trace_mirrored_snake(std::span<const Pt> visited, Facing rf)
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
            rs[i-1] = 'D';
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            rs[i-1] = 'U';
            rf = rf.down();
        }
        pos = nextpos;
    }
    return rs;
}

struct Floodfiller {
    explicit Floodfiller() = default;

    void reset_things(Pt *visited, int n) {
        visited_first = visited;
        visited_last = visited + n;
        minx = miny = minz = INT_MAX;
        maxx = maxy = maxz = INT_MIN;
        for (int i=0; i < n; ++i) {
//printf("visited[%d] = Pt(%d,%d,%d)\n", i, get<0>(visited[i]), get<1>(visited[i]), get<2>(visited[i]));
            minx = std::min(minx, get<0>(visited[i]));
            miny = std::min(miny, get<1>(visited[i]));
            minz = std::min(minz, get<2>(visited[i]));
            maxx = std::max(maxx, get<0>(visited[i]));
            maxy = std::max(maxy, get<1>(visited[i]));
            maxz = std::max(maxz, get<2>(visited[i]));
        }
        assert((maxx+1 - minx) <= n);
        assert((maxy+1 - miny) <= n);
        assert((maxz+1 - minz) <= n);
        minx -= 1;
        miny -= 1;
        minz -= 1;
        maxx += 1;
        maxy += 1;
        maxz += 1;
        flooded_last = flooded;
    }

    bool too_thin_for_cavities() const {
        return (maxx+1 - minx) <= 4 || (maxy+1 - miny) <= 4 || (maxz+1 - minz) <= 4;
    }

    void flood() {
        flood(Pt(minx, miny, minz));
    }

    size_t flooded_volume() const {
        return flooded_last - flooded;
    }
    size_t volume() const {
        return (maxx+1 - minx) * (maxy+1 - miny) * (maxz+1 - minz);
    }

private:
    void flood(Pt p) {
        assert(flooded_last < std::end(flooded));
        assert(minx <= p.x && p.x <= maxx);
        assert(miny <= p.y && p.y <= maxy);
        assert(minz <= p.z && p.z <= maxz);
        if (std::find(visited_first, visited_last, p) != visited_last) {
            // This cell is part of the snake itself.
        } else if (std::find(flooded, flooded_last, p) != flooded_last) {
            // This cell is previously flooded.
        } else {
            // Flood it and visit its neighbors.
            *flooded_last++ = p;
            if (p.x + 1 <= maxx) flood(Pt(p.x + 1, p.y, p.z));
            if (minx <= p.x - 1) flood(Pt(p.x - 1, p.y, p.z));
            if (p.y + 1 <= maxy) flood(Pt(p.x, p.y + 1, p.z));
            if (miny <= p.y - 1) flood(Pt(p.x, p.y - 1, p.z));
            if (p.z + 1 <= maxz) flood(Pt(p.x, p.y, p.z + 1));
            if (minz <= p.z - 1) flood(Pt(p.x, p.y, p.z - 1));
        }
    }

    Pt *visited_first = nullptr;
    Pt *visited_last = nullptr;
    int minx = INT_MAX;
    int miny = INT_MAX;
    int minz = INT_MAX;
    int maxx = INT_MIN;
    int maxy = INT_MIN;
    int maxz = INT_MIN;
    Pt *flooded_last = nullptr;
    Pt flooded[60*60*60];
};
static Floodfiller g_floodfiller;

bool has_cavities(Pt *visited, int n) {
    g_floodfiller.reset_things(visited, n);
    if (g_floodfiller.too_thin_for_cavities()) {
        return false;
    }
    g_floodfiller.flood();
    return (g_floodfiller.flooded_volume() != g_floodfiller.volume() - n);
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

    for (int t=0; t < n; ++t) {
        for (int i=0; i < 24; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_mirrored_snake(visited, rf);
                assert(rs.size() == sv.size());
                if (is_canonical_form(rs) && rs < sv) {
                    return has_cavities(pv, n) ? ONESIDED_CAVITOUS_OUROBOROS : ONESIDED_STRIP_OUROBOROS;
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
                auto rs = trace_mirrored_snake(visited, rf);
                assert(rs.size() == sv.size());
                if (is_canonical_form(rs) && rs < sv) {
                    return has_cavities(pv, n) ? ONESIDED_CAVITOUS_OUROBOROS : ONESIDED_STRIP_OUROBOROS;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    return has_cavities(pv, n) ? FREE_CAVITOUS_OUROBOROS : FREE_STRIP_OUROBOROS;
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
        for (int j = i-2; j >= 0; j -= 2) {
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
    visited[n] = pos;

    // If this string is less than the reverse-trace, it's at least a one-sided snake/strip.
    // If this string is less than either its mirror-image or the mirror-image of its reverse-trace,
    // then it's a free snake/strip.
    char reverse[64] = "S";
    for (size_t i=0; i < n; ++i) {
        const Pt& nextpos = visited[n - i - 1];
        // How do we get from pos to nextpos?
        if (rf.step(pos) == nextpos) {
            reverse[i] = 'S';
        } else if (rf.left().step(pos) == nextpos) {
            reverse[i] = 'L';
            rf = rf.left();
        } else if (rf.right().step(pos) == nextpos) {
            reverse[i] = 'R';
            rf = rf.right();
        } else if (rf.up().step(pos) == nextpos) {
            reverse[i] = 'U';
            rf = rf.up();
        } else {
            assert(rf.down().step(pos) == nextpos);
            reverse[i] = 'D';
            rf = rf.down();
        }
        pos = nextpos;
    }
    assert(std::string_view(reverse).size() == sv.size());
    bool reverseIsLess = (std::string_view(reverse) < sv);
    bool mirrorIsLess = (!reverseIsLess) && [&]() {
        for (size_t i=0; i < n; ++i) {
            if (sv[i] == 'U' || sv[i] == 'D') return (sv[i] == 'U');
        }
        return false;
    }();
    bool reverseMirrorIsLess = (!reverseIsLess) && (!mirrorIsLess) && [&]() {
        assert(reverse[0] == 'S');
        for (size_t i=1; i < n; ++i) {
            reverse[i] = (reverse[i] == 'U') ? 'D' : (reverse[i] == 'D') ? 'U' : reverse[i];
        }
        assert(std::string_view(reverse).size() == sv.size());
        return (std::string_view(reverse) < sv);
    }();

    if (reverseIsLess) {
        return NOT_A_SNAKE;
    } else if (mirrorIsLess || reverseMirrorIsLess) {
        return has_cavities(visited, n+1) ? ONESIDED_CAVITOUS_SNAKE : ONESIDED_STRIP;
    } else {
        return has_cavities(visited, n+1) ? FREE_CAVITOUS_SNAKE : FREE_STRIP;
    }
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
        do {
            assert(is_canonical_form(s));
            nStrings += 1;
            switch (testSnake(s)) {
                default:
                    assert(false);
                    break;
                case NOT_A_SNAKE:
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
            if (++tick == 100000) {
                tick = 0;
                print_stats('\r');
            }
        } while (odometer(s));
        print_stats('\n');
    }
}
