#!/usr/bin/env python

import random
import subprocess
import sys
import time

def generate_headers():
    return """
#include <algorithm>
#include <type_traits>
#include <utility>
    """

def generate_nouns(nouns):
    if nouns == 0:
        return """
template<class T> using TypeIdentity = std::type_identity<T>;
template<int I> using IndexConstant = std::integral_constant<int, I>;
template<int... Is> using Vector = std::integer_sequence<int, Is...>;
template<int N> using Iota = std::make_integer_sequence<int, N>;
        """
    elif nouns == 1:
        return """
template<class T> struct TypeIdentity { using type = T; };
template<int I> struct IndexConstant { static constexpr int value = I; };
template<int...> struct Vector {};

template<int, class> struct Append {};
template<int I, int... Is> struct Append<I, Vector<Is...>> : TypeIdentity<Vector<Is..., I>> {};

template<int N> struct IotaImpl : Append<N-1, typename IotaImpl<N-1>::type> {};
template<> struct IotaImpl<0> { using type = Vector<>; };
template<int N> using Iota = IotaImpl<N>::type;
        """

def generate_verbs(x):
    if x == 0:
        return """
template<int, class> struct Prepend;
template<int I, int... Is> struct Prepend<I, Vector<Is...>> : TypeIdentity<Vector<I, Is...>> {};

template<int, class> struct RemoveFirst;
template<int N> struct RemoveFirst<N, Vector<>> : TypeIdentity<Vector<>> {};
template<int N, int... Hs> struct RemoveFirst<N, Vector<N, Hs...>> : TypeIdentity<Vector<Hs...>> {};
template<int N, int H, int... Hs> struct RemoveFirst<N, Vector<H, Hs...>> {
  using type = Prepend<H, typename RemoveFirst<N, Vector<Hs...>>::type>::type;
};

template<class> struct Min;
template<int I> struct Min<Vector<I>> : IndexConstant<I> {};
template<int I, int... Is> struct Min<Vector<I, Is...>> {
  using TailMin = Min<Vector<Is...>>;
  static constexpr int value = TailMin::value < I ? TailMin::value : I;
};

template<class> struct Sort;
template<> struct Sort<Vector<>> : TypeIdentity<Vector<>> {};
template<int... Is> struct Sort<Vector<Is...>> {
  using CurrMin = Min<Vector<Is...>>;
  using Tail = RemoveFirst<CurrMin::value, Vector<Is...>>::type;
  using type = Prepend<CurrMin::value, typename Sort<Tail>::type>::type;
};
        """
    elif x in [1,2,3,4]:
        step_code = {
            1: "std::sort(arr, arr + sizeof...(Is));",
            2: "std::ranges::sort(arr);",
            3: "std::nth_element(arr, arr + i, arr + sizeof...(Is));",
            4: "std::ranges::nth_element(arr, arr + i);",
        }[x]
        return """
template<class> struct Sort;
template<int... Is> struct Sort<Vector<Is...>> {
  static constexpr int GetNthElement(int i) {
    int arr[] = {Is...};
    %s
    return arr[i];
  }
  template<class> struct Helper;
  template<int... Indices> struct Helper<Vector<Indices...>> {
    using type = Vector<GetNthElement(Indices)...>;
  };
  using type = Helper<Iota<sizeof...(Is)>>::type;
};
        """ % step_code


def generate_one_test(i, n, r):
    vec = [r.randrange(0, (n*n) // 2) for i in range(n)]
    return "\n".join([
        "void test%d() {" % i,
        "  using In = Vector<%s>;" % ",".join(map(str, vec)),
        "  using Out = Vector<%s>;" % ",".join(map(str, sorted(vec))),
        "  static_assert(__is_same(typename Sort<In>::type, Out));",
        "}"
    ])

def generate_file(nouns, verbs, n):
    r = random.Random()
    r.seed(42)
    return generate_headers() + generate_nouns(nouns) + generate_verbs(verbs) + '\n'.join([
        generate_one_test(i, n, r) for i in range(10)
    ])

def benchmark_one_compile(nouns, verbs, n):
    compiler = 'g++'
    compiler_args = ['-std=c++20', '-O2']
    iterations = 3
    with open('x.cpp', 'w') as f:
        f.write(generate_file(nouns, verbs, n))
    elapsed = 0.0
    for i in range(iterations):
        start = time.time()
        subprocess.call([compiler] + compiler_args + ['-c', 'x.cpp'])
        elapsed += (time.time() - start)
    t = (elapsed / iterations)
    print('%d %d %d %.3f' % (nouns, verbs, n, t))
    sys.stdout.flush()
    return t


def benchmark_all_compiles():
    for nouns in [0, 1]:
        for verbs in [0, 1, 2, 3, 4]:
            for n in range(10, 160, 10):
                t = benchmark_one_compile(nouns, verbs, n)
                if (t > 20):
                    break


def scatter_one_plot(plt, nouns, verbs, data):
    import numpy as np
    import scipy
    ns = [d[2] for d in data if d[0] == nouns and d[1] == verbs]
    timings = [d[3] for d in data if d[0] == nouns and d[1] == verbs]
    marker, color, label = {
        (0, 0): ('^', '#000000', 'Vector, selection sort'),
        (1, 0): ('^', '#d62728', 'Vector, std::sort'),
        (2, 0): ('<', '#ff7f0e', 'Vector, std::ranges::sort'),
        (3, 0): ('^', '#1f77b4', 'Vector, std::nth_element'),
        (4, 0): ('<', '#2ca02c', 'Vector, std::ranges::nth_element'),
        (0, 1): ('v', '#000000', 'std::integer_sequence, selection sort'),
        (1, 1): ('v', '#d62728', 'std::integer_sequence, std::sort'),
        (2, 1): ('>', '#ff7f0e', 'std::integer_sequence, std::ranges::sort'),
        (3, 1): ('v', '#1f77b4', 'std::integer_sequence, std::nth_element'),
        (4, 1): ('>', '#2ca02c', 'std::integer_sequence, std::ranges::nth_element'),
    }[(verbs, nouns)]
    plt.scatter(ns, timings, marker=marker, color=color, label=label, s=10)

    def func(x, a, b, c, d, e):
        a = a if a > 0 else 0
        b = b if b > 0 else 0
        c = c if c > 0 else 0
        d = d if d > 0 else 0
        e = e if e > 0 else 0
        return a*x*x*x + b*x*x + c*x*np.log2(x) + d*x + e

    popt, _ = scipy.optimize.curve_fit(func, ns, timings)
    a, b, c, d, e = popt
    xs = list(range(ns[0], ns[-1]))
    plt.plot(xs, [func(x, a, b, c, d, e) for x in xs], color=color, linewidth=1)


if sys.argv[1] == '--graph':
    import matplotlib.pyplot as plt
    fields = [line.split(' ') for line in sys.stdin.readlines()]
    data = [(int(f[0]), int(f[1]), int(f[2]), float(f[3])) for f in fields]
    for nouns in [0, 1]:
        for verbs in [0, 1, 2, 3, 4]:
            scatter_one_plot(plt, nouns, verbs, data)
    plt.legend()
    plt.show()
elif sys.argv[1] == '--time':
    benchmark_all_compiles()
else:
    print('Usage: ./x.py --time | ./x.py --graph')
