moss 1000;

pick head {
  core: @len;
  moss: @output as show;
}

fn int main() {
  // Fixed 8 slots: growth is impossible, so the overflow wraps around.
  array int a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  __roll a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
  show @len(a);
  for i in a {
    show @i;
  }

  // Unbounded: it can grow, so it grows and never wraps.
  array int b[~];
  __roll b[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
  show @len(b);
  show @b[0];
  show @b[9];

  // Bounded with room left: grows up to the declared maximum.
  array int c[2~5] = {1, 2};
  __roll c[4] = {10, 20, 30, 40};
  show @len(c);
  show @c[3];

  // Bounded and full: wraps over the five slots.
  array int d[2~5] = {1, 2, 3, 4, 5};
  __roll d[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  show @len(d);
  for v in d {
    show @v;
  }

  // A hole still advances the slot but leaves the value alone.
  array int e[4] = {1, 2, 3, 4};
  __roll e[5] = {9, \null, 7, 8, 8};
  for w in e {
    show @w;
  }
  return 0;
}
