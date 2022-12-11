#include <bit>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// A polyomino snake is given by looking down one of its ends
// and recording which way you "turn" each time you move
// to the next cube; so a polysnake of n squares is represented
// by a string of (n-1) letters.
//
// To canonicalize a directed polysnake, turn it so that the string
// begins with "S".
//
// A polysnake should be non-self-intersecting and
// non-self-adjoining.

struct Pt {
    union {
        struct { signed char x, y, pad1=0, pad2=0; };
        unsigned int i;
    };
    explicit Pt() {}
    Pt(int x, int y) : x(x), y(y) {}
    bool operator==(const Pt& p) const { return i == p.i; }
    bool adjacentTo(const Pt& p) const {
        return std::has_single_bit(i - p.i) || std::has_single_bit(p.i - i);
    }
};
template<> struct std::tuple_size<Pt> : std::integral_constant<size_t, 2> {};
template<size_t I> struct std::tuple_element<I, Pt> : std::type_identity<int> {};
template<size_t I> int get(const Pt& p) requires (I == 0) { return p.x; }
template<size_t I> int get(const Pt& p) requires (I == 1) { return p.y; }

struct Facing {
    int value_ = 0;
    explicit Facing(int i) : value_(i) {}
    friend bool operator==(Facing, Facing) = default;
    Facing left() const {
        return Facing((value_ + 3) % 4);
    }
    Facing right() const {
        return Facing((value_ + 1) % 4);
    }
    Pt step(Pt t) const {
        auto [x, y] = t;
        x += (value_ == 1) - (value_ == 3);
        y += (value_ == 0) - (value_ == 2);
        return {x, y};
    }
};

void unit_test_facings()
{
    for (int i=0; i < 4; ++i) {
        Facing f = Facing(i);
        (void)f;
        assert(f == f.right().right().right().right());
        assert(f.left() == f.right().right().right());
        assert(f.left().left() == f.right().right());
        assert(f.left().left().left() == f.right());
        assert(f.left().left().left().left() == f);
    }
}

bool is_canonical_form(std::string_view sv)
{
    if (sv[0] != 'S') return false;
    return true;
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
        } else {
            assert(rf.right().step(pos) == nextpos);
            rs[i-1] = 'R';
            rf = rf.right();
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
            rs[i-1] = 'R';
            rf = rf.left();
        } else {
            assert(rf.right().step(pos) == nextpos);
            rs[i-1] = 'L';
            rf = rf.right();
        }
        pos = nextpos;
    }
    return rs;
}

enum SnakeOutcome {
    NOT_A_SNAKE,
    FREE_OUROBOROS,
    ONESIDED_OUROBOROS,
    FREE_STRIP,
    ONESIDED_STRIP,
    FREE_SNAKE,
    ONESIDED_SNAKE,
};

SnakeOutcome testOuroboros(std::string_view sv, Pt *pv)
{
    const int n = sv.size() + 1;
    auto visited = std::span<Pt>(pv, pv + n);
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 4; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return NOT_A_SNAKE;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    std::reverse(visited.begin(), visited.end());
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 4; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return NOT_A_SNAKE;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }

    for (int t=0; t < n; ++t) {
        for (int i=0; i < 4; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_mirrored_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return ONESIDED_OUROBOROS;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    std::reverse(visited.begin(), visited.end());
    for (int t=0; t < n; ++t) {
        for (int i=0; i < 4; ++i) {
            Facing rf = Facing(i);
            if (visited[1] == rf.step(visited[0])) {
                auto rs = trace_mirrored_snake(visited, rf);
                assert(rs.size() == sv.size());
                assert(is_canonical_form(rs));
                if (rs < sv) {
                    return ONESIDED_OUROBOROS;
                }
            }
        }
        std::rotate(visited.begin(), visited.begin()+1, visited.end());
    }
    return FREE_OUROBOROS;
}

bool is_strip(Pt *visited, int n) {
    int minx = INT_MAX;
    int miny = INT_MAX;
    int maxx = INT_MIN;
    int maxy = INT_MIN;
    for (int i=0; i < n; ++i) {
        minx = std::min(minx, get<0>(visited[i]));
        miny = std::min(miny, get<1>(visited[i]));
        maxx = std::max(maxx, get<0>(visited[i]));
        maxy = std::max(maxy, get<1>(visited[i]));
    }
    minx -= 1;
    miny -= 1;
    maxx += 1;
    maxy += 1;
    const size_t rectSize = (maxx+1 - minx) * (maxy+1 - miny);
    std::vector<Pt> flooded;
    flooded.reserve(rectSize - n);
    std::function<void(Pt)> flood = [&](Pt p) {
        if (std::find(visited, visited + n, p) != visited + n) {
            // This cell is part of the snake itself.
        } else if (std::find(flooded.begin(), flooded.end(), p) != flooded.end()) {
            // This cell is previously flooded.
        } else {
            // Flood it and visit its neighbors.
            flooded.push_back(p);
            if (p.x + 1 <= maxx) flood(Pt(p.x + 1, p.y));
            if (minx <= p.x - 1) flood(Pt(p.x - 1, p.y));
            if (p.y + 1 <= maxy) flood(Pt(p.x, p.y + 1));
            if (miny <= p.y - 1) flood(Pt(p.x, p.y - 1));
        }
    };
    flood(Pt(minx, miny));
    assert(flooded.size() <= rectSize - n);
    assert(flooded.size() >= 2u*(maxx+1 - minx));
    assert(flooded.size() >= 2u*(maxy+1 - miny));
    return (flooded.size() == rectSize - n);
}

SnakeOutcome testSnake(std::string_view sv)
{
    const size_t n = sv.size();
    Facing f = Facing(0);
    Facing rf = f.right().right();
    Pt pos = {60, 60};
    static Pt visited[64];
    for (size_t i = 0; i < n; ++i) {
        switch (sv[i]) {
            case 'S': break;
            case 'L': rf = f.right(); f = f.left();  break;
            case 'R': rf = f.left();  f = f.right(); break;
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
    for (size_t i=1; i < n; ++i) {
        reverse[i] = sv[n-i];
    }
    assert(std::string_view(reverse).size() == sv.size());
    bool reverseMirrorIsLess = (std::string_view(reverse) < sv);
    for (size_t i=1; i < n; ++i) {
        reverse[i] = (reverse[i] == 'R') ? 'L' : (reverse[i] == 'L') ? 'R' : 'S';
    }
    assert(std::string_view(reverse).size() == sv.size());
    bool reverseIsLess = (std::string_view(reverse) < sv);
    bool mirrorIsLess = (!reverseIsLess) && [&]() {
        for (size_t i=0; i < n; ++i) {
            if (sv[i] != 'S') return (sv[i] == 'R');
        }
        return false;
    }();

    if (reverseIsLess) {
        return NOT_A_SNAKE;
    } else if (mirrorIsLess || reverseMirrorIsLess) {
        return is_strip(visited, n+1) ? ONESIDED_STRIP : ONESIDED_SNAKE;
    } else {
        return is_strip(visited, n+1) ? FREE_STRIP : FREE_SNAKE;
    }
}

bool odometer(std::string& s)
{
    int n = s.size();
    auto increment = [](char& ch) {
        if (ch == 'L') ch = 'S';
        else if (ch == 'S') ch = 'R';
        else ch = 'L';
    };
    int i = n - 1;
    do {
        if (i == 0) return false;
again:
        increment(s[i--]);
    } while (s[i+1] == 'S');
    if (s[i] == s[i+1] && s[i] != 'S') {
        // We just created LL or RR:
        // substrings that can't appear in a valid snake.
        // (Except for the trivial 4-cube ouroboros SRR.)
        // If we just made XYZRRSSS, change it to XYZRLSSS.
        // If we just made XYZLLSSS, change it to XYZLSSSS.
        ++i;
        goto again;
    }
    return true;
}

int main(int argc, char **argv)
{
    int n = (argc < 2 || atoi(argv[1]) < 3) ? 3 : atoi(argv[1]);
    unit_test_facings();
    printf("| n | Strings | Free strip polyominoes (A333313) | Free non-ouroboros snakes (A002013) | Free ouroboroi | One-sided strips | One-sided non-ouroboros snakes (A151514) | One-sided ouroboroi |\n");

    for (; true; ++n) {
        size_t nStrings = 0;
        size_t nFreeStrips = 0;
        size_t nFreeSnakes = 0;
        size_t nFreeOuroboroi = 0;
        size_t nOneSidedStrips = 0;
        size_t nOneSidedSnakes = 0;
        size_t nOneSidedOuroboroi = 0;
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
            printf("| %d | %zu | %zu | %zu | %zu | %zu | %zu | %zu | (%zu sec, %zu sec asleep)%c",
                n, nStrings,
                nFreeStrips, nFreeSnakes, nFreeOuroboroi,
                nOneSidedStrips, nOneSidedSnakes, nOneSidedOuroboroi,
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
                case FREE_OUROBOROS:
                    nFreeOuroboroi += 1;
                    nOneSidedOuroboroi += 1;
                    break;
                case ONESIDED_OUROBOROS:
                    nOneSidedOuroboroi += 1;
                    break;
                case FREE_STRIP:
                    nFreeStrips += 1;
                    nOneSidedStrips += 1;
                    nFreeSnakes += 1;
                    nOneSidedSnakes += 1;
                    break;
                case ONESIDED_STRIP:
                    nOneSidedStrips += 1;
                    nOneSidedSnakes += 1;
                    break;
                case FREE_SNAKE:
                    nFreeSnakes += 1;
                    nOneSidedSnakes += 1;
                    break;
                case ONESIDED_SNAKE:
                    nOneSidedSnakes += 1;
                    break;
            }
            if (++tick == 1000000) {
                tick = 0;
                print_stats('\r');
            }
        } while (odometer(s));
        print_stats('\n');
    }
}
