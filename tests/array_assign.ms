moss 1000;

pick head {
  core: @len;
  moss: @output as show;
}

fn int main() {
  // Whole-array assignment keeps the length and zero-fills the tail.
  array int a[5] = {1, 2, 3, 4, 5};
  a = {9, 8};
  show @len(a);
  for i in a {
    show @i;
  }

  // Prefix: rewrite the first 3 elements only.
  a[3] = {7, 6, 5};
  for i in a {
    show @i;
  }

  // A whole-array assignment grows an unbounded array.
  array int g[~];
  g = {4, 5, 6};
  show @len(g);
  for v in g {
    show @v;
  }

  // `\null` leaves its slot untouched, the rest of the prefix is zeroed.
  array int h[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  h[3] = {10, \null, 30};
  for x in h {
    show @x;
  }

  // A short literal zeroes the rest of the prefix, not the whole array.
  array int z[4] = {1, 2, 3, 4};
  z[3] = {1};
  for y in z {
    show @y;
  }

  // Strings work the same way.
  array string names[2] = {"a", "b"};
  names = {"x"};
  show @names[0];
  show @names[1];
  return 0;
}
