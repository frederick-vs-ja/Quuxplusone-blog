#!/usr/bin/env python

# python 2024-12-31-slalom.py be 2024-12-31-media-roundup.html
# python 2024-12-31-slalom.py f 2024-12-31-media-roundup.html

import datetime
import matplotlib.pyplot as plt
import re
import sys

with open(sys.argv[2]) as f:
  lines = f.readlines()

values = []
for tag in sys.argv[1]:
  for line in lines:
    if re.search(' [%s] ' % tag, line):
      m = re.search(r'(2024-\d\d-\d\d)', line)
      if m is None:
        continue
      yvalue = datetime.datetime.strptime(m.group(1), '%Y-%m-%d').timetuple().tm_yday
      m = re.search(r'[(](\d\d\d\d)[)]', line)
      if m is None:
        xvalue = 0
      else:
        xvalue = int(m.group(1))
        if xvalue < 1600:
          xvalue = 0
      starred = ('&#x2B50;' in line)
      values += [(xvalue, yvalue, tag, starred)]

print(values)

colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
for (ti, tag) in enumerate(sys.argv[1]):
  xs = []
  ys = []
  for (i, (xvalue, yvalue, t, _)) in enumerate(values):
    if (t == tag) and (xvalue >= 1600):
      xs += [xvalue]
      ys += [yvalue]
  plt.scatter(xs, ys, label=tag, marker='o', color=colors[ti], s=5)

  xs = []
  ys = []
  for (i, (xvalue, yvalue, t, _)) in enumerate(values):
    if (t == tag) and (xvalue < 1600):
      xs += [1590]
      ys += [yvalue]
  plt.scatter(xs, ys, marker='<', color=colors[ti], s=5)

  xs = []
  ys = []
  for (i, (xvalue, yvalue, t, starred)) in enumerate(values):
    if (t == tag) and starred:
      if (xvalue < 1600):
        xvalue = 1590
      xs += [xvalue]
      ys += [yvalue]
  plt.scatter(xs, ys, marker='*', color=colors[ti], s=20)

plt.legend()
plt.show()
