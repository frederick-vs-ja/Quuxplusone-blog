#!/usr/bin/env python

# python 2024-12-31-cumul.py be 2024-12-31-media-roundup.html
# python 2024-12-31-cumul.py f 2024-12-31-media-roundup.html

import matplotlib.pyplot as plt
import re
import sys

with open(sys.argv[2]) as f:
  lines = f.readlines()

values = []
for tag in sys.argv[1]:
  for line in lines:
    if re.search(' [%s] ' % tag, line):
      m = re.search(r'[(](\d\d\d\d)[)]', line)
      if m is not None:
        values += [(int(m.group(1)), tag)]

print(values)
values.sort()

colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
for (ti, tag) in enumerate(sys.argv[1]):
  xs = []
  ys = []
  for (i, (year, t)) in enumerate(values):
    if (t == tag) and (year >= 1600):
      xs += [year]
      ys += [i]
  plt.scatter(xs, ys, label=tag, marker='o', color=colors[ti], s=2)

plt.legend()
plt.show()
