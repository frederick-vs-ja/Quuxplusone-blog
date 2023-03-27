#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

bool overshoot = true;

struct MinScore {
    int score;
    int driver;
    int putter;
};

int gcd(int x, int y) {
    if (x > y) {
        return gcd(y, x);
    } else if (y % x == 0) {
        return x;
    } else {
        return gcd(y % x, x);
    }
}

int score_for(bool allow_overshoot, int hole, int driver, int putter) {
    assert(driver > putter);
    if (!allow_overshoot && hole % gcd(driver, putter) != 0) {
        return INT_MAX;
    }
    int candidate = 100;
    if (hole % driver == 0) {
        return (hole / driver);  // Drive forward
    }
    if (allow_overshoot && (hole+1) % driver == 0) {
        candidate = std::min(candidate, (hole+1) / driver);  // Drive forward, overshooting
    }
    if (hole % putter == 0) {
        candidate = std::min(candidate, hole / putter);  // Putt forward
    }
    if (allow_overshoot && (hole+1) % putter == 0) {
        candidate = std::min(candidate, (hole+1) / putter);  // Putt forward, overshooting
    }
    if (allow_overshoot && (hole-1) % putter == 0) {
        candidate = std::min(candidate, ((hole-1) / putter) + 2);  // Putt forward, then putt backward
    }
    if (allow_overshoot && (hole-1) % driver == 0) {
        candidate = std::min(candidate, ((hole-1) / driver) + 2);  // Drive forward, then drive backward
    }
    for (int d = 0; d <= hole/driver; ++d) {  // Drive forward, then putt forward
        int left = (hole - d*driver);
        if (left % putter == 0) {
            candidate = std::min(candidate, d + (left / putter));
        }
        if (allow_overshoot && (left+1) % putter == 0) {
            candidate = std::min(candidate, d + ((left+1) / putter));
        }
    }
    for (int d = hole/driver + 1; d < candidate; ++d) {  // Drive forward, then putt backward
        int left = (d*driver - hole);
        if (left % putter == 0) {
            candidate = std::min(candidate, d + (left / putter));
        }
        if (allow_overshoot && (left+1) % putter == 0) {
            candidate = std::min(candidate, d + ((left+1) / putter));
        }
    }
    for (int p = hole/putter + 1; p < candidate; ++p) { // Putt forward, then drive backward
        int left = (p*putter - hole);
        if (left % driver == 0) {
            candidate = std::min(candidate, p + (left / driver));
        }
        if (allow_overshoot && (left+1) % driver == 0) {
            candidate = std::min(candidate, p + ((left+1) / driver));
        }
    }
    if (candidate < 100) {
        return candidate;
    } else {
        return INT_MAX;
    }
}

MinScore solve(bool allow_overshoot, const std::vector<int>& holes) {
    int max = *std::max_element(holes.begin(), holes.end());
    MinScore minscore = { INT_MAX, 0, 0 };
    for (int driver = 2; driver < 28*max; ++driver) {
        for (int putter = 1; putter < driver; ++putter) {
            int score = 0;
            for (int hole : holes) {
                int s = score_for(allow_overshoot, hole, driver, putter);
                if (s == INT_MAX) {
                    score = INT_MAX;
                    break;
                }
                score += s;
            }
            if (score < minscore.score) {
                minscore = { score, driver, putter };
            }
        }
    }
    return minscore;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        puts("./golf basic 10 15 16 29  -- find the best pair of clubs");
        puts("./golf advanced 10 15 16 29 -- as above, but permit overshoot by 1");
        puts("./golf (basic|advanced) clubs 5 4 10 15 16 29 -- print a hole-by-hole breakdown of (10,15,16,29) using clubs (5,4)");
        puts("./golf (basic|advanced) -- find the hardest 4-hole course less than (20,20,20,20)");
        return 0;
    }
    bool allow_overshoot = false;
    if (argc >= 2 && argv[1] == std::string("basic")) {
        allow_overshoot = false;
        argv += 1;
        argc -= 1;
    }
    if (argc >= 2 && argv[1] == std::string("advanced")) {
        allow_overshoot = true;
        argv += 1;
        argc -= 1;
    }
    if (argc >= 3 && (argv[1] == std::string("clubs"))) {
        int driver = atoi(argv[2]);
        int putter = atoi(argv[3]);
        if (putter > driver) std::swap(putter, driver);
        printf("Driver %d, putter %d\n", driver, putter);
        for (int i = 4; i < argc; ++i) {
            int hole = atoi(argv[i]);
            int s = score_for(allow_overshoot, hole, driver, putter);
            printf("- %d in %d strokes\n", hole, s);
        }
    } else if (argc >= 2) {
        // Find the best possible score for this specific course of holes.
        std::vector<int> holes;
        for (int i = 1; i < argc; ++i) {
            holes.push_back(atoi(argv[i]));
        }
        MinScore minscore = solve(allow_overshoot, holes);
        printf("Best: %d strokes with driver %d and putter %d\n", minscore.score, minscore.driver, minscore.putter);
    } else {
        // Find the hardest possible course.
        int ms = INT_MIN;
        for (int a = 2; a < 20; ++a) {
            for (int b = 2; b < a; ++b) {
                for (int c = 2; c < b; ++c) {
                    for (int d = 2; d < c; ++d) {
                        MinScore minscore = solve(allow_overshoot, {a,b,c,d});
                        if (minscore.score > ms) {
                            printf("%d %d %d %d -- Best: %d strokes with driver %d and putter %d\n",
                                a,b,c,d, minscore.score, minscore.driver, minscore.putter);
                            ms = minscore.score;
                        }
                    }
                }
            }
        }
    }
}
