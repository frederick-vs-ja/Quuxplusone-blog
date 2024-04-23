// Compile with e.g. `g++ -std=c++17 -DN=10 -DK=5 solver.cpp`

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <map>
#include <numeric>
#include <vector>
#include "combinations.h" // https://github.com/HowardHinnant/combinations

using Int = signed char;

struct Position {
  Int data_[N];
  int size_;
  struct Less {
    bool operator()(const Position& lhs, const Position& rhs) const {
      if (lhs.size_ != rhs.size_) return lhs.size_ < rhs.size_;
      return std::lexicographical_compare(lhs.data_, lhs.data_ + lhs.size_, rhs.data_, rhs.data_ + rhs.size_);
    }
  };
  friend bool operator==(const Position& lhs, const Position& rhs) {
    return std::equal(lhs.data_, lhs.data_ + lhs.size_, rhs.data_, rhs.data_ + rhs.size_);
  }
  friend bool operator!=(const Position& lhs, const Position& rhs) {
    return !std::equal(lhs.data_, lhs.data_ + lhs.size_, rhs.data_, rhs.data_ + rhs.size_);
  }
  bool contains(Int k) const {
    return std::find(data_, data_ + size_, k) != data_ + size_;
  }
  bool is_reversed_of(const Position& p) const {
    return size_ == p.size_ && std::equal(
      std::make_reverse_iterator(data_ + size_), std::make_reverse_iterator(data_),
      p.data_, p.data_ + size_
    );
  }
  void print() const {
    for (int i=0; i < size_; ++i) {
      printf("%s%d", (i != 0 ? " " : ""), int(data_[i]));
    }
  }
};

template<class Function>
bool for_each_neighbor_position(int constrain_length, const Position& p, const Function& f) {
  if (p.size_ < constrain_length) {
    // Try splitting
    for (int i = 0; i < p.size_; ++i) {
      Position n;
      n.size_ = p.size_ + 1;
      std::copy(p.data_, p.data_ + i, n.data_);
      std::copy(p.data_ + i + 1, p.data_ + p.size_, n.data_ + i + 2);
      for (Int lo = 1; true; ++lo) {
        Int hi = p.data_[i] - lo;
        if (hi <= lo) break;
        if (p.contains(lo) || p.contains(hi)) {
          continue;
        }
        n.data_[i] = lo;
        n.data_[i+1] = hi;
        bool success = f(n);
        if (success) return true;
        n.data_[i] = hi;
        n.data_[i+1] = lo;
        success = f(n);
        if (success) return true;
      }
    }
  }
  // Try merging
  for (int i = 1; i < p.size_; ++i) {
    Int sum = p.data_[i-1] + p.data_[i];
    if (sum > N || p.contains(sum)) {
      continue;
    }
    Position n;
    n.size_ = p.size_ - 1;
    std::copy(p.data_, p.data_ + i - 1, n.data_);
    n.data_[i-1] = sum;
    std::copy(p.data_ + i + 1, p.data_ + p.size_, n.data_ + i);
    bool success = f(n);
    if (success) return true;
  }
  return false;
}

int moves_to_reverse(const Position& originalp, int constrain_length, bool printit) {
  if (originalp.size_ == 1) return 0;
  std::map<Position, Position, Position::Less> interior;
  std::map<Position, Position, Position::Less> frontier = { {originalp, originalp} };
  int moves = 1;
  while (true) {
    std::map<Position, Position, Position::Less> oldfrontier = std::move(frontier);
    for (const auto& kv : oldfrontier) {
      const Position& p = kv.first;
      bool success = for_each_neighbor_position(constrain_length, p, [&](const Position& n) {
        // Each move changes the parity of the list, so n will never be in oldfrontier.
        if ((moves & 1) == 0 && n.is_reversed_of(originalp)) {
          if (printit) {
            // Print the path that got us here.
            std::vector<Position> moves;
            moves.push_back(n);
            moves.push_back(p);
            Position prev = p;
            while (true) {
              Position prev2 = interior.at(prev);
              if (prev2 == prev) break;
              moves.push_back(prev2);
              prev = prev2;
            }
            assert(prev == originalp);
            while (!moves.empty()) {
              moves.back().print(); printf("\n");
              moves.pop_back();
            }
          }
          return true;
        }
        if (interior.find(n) == interior.end()) {
          frontier.emplace(n, p);
        }
        return false;
      });
      if (success) return moves;
    }
    if (frontier.empty()) {
      return -1;
    }
    moves += 1;
    interior.merge(std::move(oldfrontier));
  }
}

template<class Function>
void for_each_list(int k, const Function& f) {
  int nums[N];
  std::iota(nums, nums + N, 1);
  if (N >= 3) {
    if (k == N) {
      // No moves are possible.
      return;
    } else if (k == N-1) {
      // We won't be able to split N, so the missing number must be N-1
      // so that we can split N-2. Also, the sum to the left of N must
      // equal the sum to the right of N.
      int totalsum = N*(N+1)/2 - (N-1);
      int halfsum = (totalsum - N) / 2;
      for_each_permutation(nums, nums + N - 2, nums + N - 2, [&](auto first, auto last) {
        // We only need to check half of all the positions, since the other half are just the first half reversed.
        if (*first > last[-1]) return false;
        // Find the one place it's legal to insert N into this list.
        int leftsum = 0;
        for (int i=0; first+i != last; ++i) {
          leftsum += first[i];
          if (leftsum == halfsum) {
            Position p;
            std::copy(first, first+i+1, p.data_);
            p.data_[i+1] = N;
            std::copy(first+i+1, last, p.data_+i+2);
            p.size_ = N-1;
            f(p);
            break;
          }
        }
        return false;
      });
      return;
    }
  }
  int size = k;
  for_each_permutation(nums, nums + size - 1, nums + N - 1, [&](auto first, auto last) {
    // We only need to check half of all the positions, since the other half are just the first half reversed.
    if (*first > last[-1]) return false;
    // In any solvable list, the sum of the missing elements must exceed n-1.
    if (N*(N+1)/2 - std::accumulate(first, last, N) < N-1) return false;
    Position p;
    std::copy(first, last, p.data_);
    p.size_ = size;
    p.data_[size - 1] = N;
    f(p);
    for (int i = size - 1; i > 0; --i) {
      std::swap(p.data_[i], p.data_[i - 1]);
      f(p);
    }
    return false;
  });
}

int main() {
  int C = 0;
  int M = 0;
  int start = time(NULL);
  std::vector<Position> solvable;
  for_each_list(K, [&](Position p) {
    int moves = moves_to_reverse(p, N-1, false);
    if (moves == -1) return;
    // The list is solvable.
    solvable.push_back(p);
    C += 2;
    if (moves > M) {
      M = moves;
    }
  });
  // Now C, M, and solvable are accurate.
  int L = K;
  for (const Position& p : solvable) {
    // Can we solve this list with a smaller L?
    while (true) {
      int m2 = moves_to_reverse(p, L, false);
      if (m2 == -1 || m2 > M) {
        ++L;
      } else {
        break;
      }
    }
  }
  // Now L is accurate.
  // Finally, double-check everything.
  for (const Position& p : solvable) {
    int m2 = moves_to_reverse(p, L, false);
    assert(m2 != -1 && m2 <= M);
  }
  printf("N=%d, K=%d: length %d, count %d, max %d, in %d seconds\n", N, K, L, C, M, int(time(NULL) - start));
}
