#!/usr/bin/env python

import itertools

BigPrime = 18361375334787046697

def modmult(x, y):
  return (x * y) % BigPrime

def modpow(x, y):
  result = x
  while (y != 0):
    result = modmult(result, result)
    if (y % 2) == 1:
      result = modmult(result, x)
    y = y // 2
  return result

def modp(w, x, y, z):
  return modmult(modpow(w, x), modpow(y, z))

def p(w, x, y, z):
  return w**x * y**z

def check(msg, w1, x1, y1, z1, w2, x2, y2, z2):
  if modp(w1, x1, y1, z1) == modp(w2, x2, y2, z2):
    result = p(w1, x1, y1, z1)
    if p(w2, x2, y2, z2) == result:
      print("{:-20d}   ".format(result) + msg.format(w1, x1, y1, z1, w2, x2, y2, z2))

def check_wood(msg, w1, x1, y1, w2, x2, y2, w3, x3, y3):
  result = w3**x3 * y3
  if (w1**x1 * y1 == result) and (w2**x2 * y2 == result):
    print("{:-20d}   ".format(result) + msg.format(w1, x1, y1, w2, x2, y2, w3, x3, y3))

if __name__ == '__main__':
  for [a,b,c,d,e,g,h,i,j] in itertools.permutations([1,2,3,4,5,6,7,8,9]):
    ab = 10*a + b
    abc = 10*ab + c
    bc = 10*b + c
    de = 10*d + e
    gh = 10*g + h
    ij = 10*i + j
    if True:
      # H.E. Dudeney's single equality problem
      if (g < i): check("{0} * {2} = {4} * {6}", abc, 1, de, 1, gh, 1, ij, 1)
      check("{0} * {2} = {4} * {6}^{7}", abc, 1, de, 1, gh, 1, i, j)
      if (g < i): check("{0} * {2} = {4}^{5} * {6}^{7}", abc, 1, de, 1, g, h, i, j)
      if (g < i): check("{0} * {2}^{3} = {4} * {6}", abc, 1, d, e, gh, 1, ij, 1)
      check("{0} * {2}^{3} = {4} * {6}^{7}", abc, 1, d, e, gh, 1, i, j)
      if (g < i): check("{0} * {2}^{3} = {4}^{5} * {6}^{7}", abc, 1, d, e, g, h, i, j)

      if (g < i): check("{0}^{1} * {2} = {4} * {6}", ab, c, de, 1, gh, 1, ij, 1)
      check("{0}^{1} * {2} = {4} * {6}^{7}", ab, c, de, 1, gh, 1, i, j)
      if (g < i): check("{0}^{1} * {2} = {4}^{5} * {6}^{7}", ab, c, de, 1, g, h, i, j)
      if (g < i): check("{0}^{1} * {2}^{3} = {4} * {6}", ab, c, d, e, gh, 1, ij, 1)
      check("{0}^{1} * {2}^{3} = {4} * {6}^{7}", ab, c, d, e, gh, 1, i, j)
      if (g < i): check("{0}^{1} * {2}^{3} = {4}^{5} * {6}^{7}", ab, c, d, e, g, h, i, j)

      if (g < i): check("{0}^{1} * {2} = {4} * {6}", a, bc, de, 1, gh, 1, ij, 1)
      check("{0}^{1} * {2} = {4} * {6}^{7}", a, bc, de, 1, gh, 1, i, j)
      if (g < i): check("{0}^{1} * {2} = {4}^{5} * {6}^{7}", a, bc, de, 1, g, h, i, j)
      if (g < i): check("{0}^{1} * {2}^{3} = {4} * {6}", a, bc, d, e, gh, 1, ij, 1)
      check("{0}^{1} * {2}^{3} = {4} * {6}^{7}", a, bc, d, e, gh, 1, i, j)
      if (g < i): check("{0}^{1} * {2}^{3} = {4}^{5} * {6}^{7}", a, bc, d, e, g, h, i, j)
    else:
      # Clement Wood's double equality problem
      hi = 10*h + i
      hij = 10*hi + j
      if (a < d): check_wood("{0} * {2} = {3} * {5} = {6}", ab, 1, c, de, 1, g, hij, 1, 1)
      if (a < d): check_wood("{0} * {2} = {3} * {5} = {6}^{7}", ab, 1, c, de, 1, g, hi, j, 1)
      if (a < d): check_wood("{0} * {2} = {3} * {5} = {6}^{7}", ab, 1, c, de, 1, g, h, ij, 1)
      if (a < d): check_wood("{0} * {2} = {3} * {5} = {6}^{7} * {8}", ab, 1, c, de, 1, g, h, i, j)
      check_wood("{0} * {2} = {3}^{4} * {5} = {6}", ab, 1, c, d, e, g, hij, 1, 1)
      check_wood("{0} * {2} = {3}^{4} * {5} = {6}^{7}", ab, 1, c, d, e, g, hi, j, 1)
      check_wood("{0} * {2} = {3}^{4} * {5} = {6}^{7}", ab, 1, c, d, e, g, h, ij, 1)
      check_wood("{0} * {2} = {3}^{4} * {5} = {6}^{7} * {8}", ab, 1, c, d, e, g, h, i, j)
      if (a < d): check_wood("{0}^{1} * {2} = {3}^{4} * {5} = {6}", a, b, c, d, e, g, hij, 1, 1)
      if (a < d): check_wood("{0}^{1} * {2} = {3}^{4} * {5} = {6}^{7}", a, b, c, d, e, g, hi, j, 1)
      if (a < d): check_wood("{0}^{1} * {2} = {3}^{4} * {5} = {6}^{7}", a, b, c, d, e, g, h, ij, 1)
      if (a < d): check_wood("{0}^{1} * {2} = {3}^{4} * {5} = {6}^{7} * {8}", a, b, c, d, e, g, h, i, j)
