moss 1000;

pick head {
  core: @len;
  moss: @output as show;
}

fn int main() {
  // Fixed length: the literal may be shorter, the rest is zero.
  array int a[5] = {1, 2, 3};
  for i in a {
    show @i;
  }

  // Exact length.
  array string names[3] = {"ann", "bob", "cid"};
  for n in names {
    show @n;
  }

  // bools zero-fill as false.
  array bool flags[3] = {true};
  for f in flags {
    show @f;
  }

  // A growable range grows to the literal when it is longer than the minimum.
  array int g[2~5] = {7, 8, 9};
  show @len(g);
  for v in g {
    show @v;
  }

  // A growable range fills up to its minimum when the literal is shorter.
  array int h[3~] = {1};
  show @len(h);
  for w in h {
    show @w;
  }

  // Empty literal, bounded: stays empty.
  array int e[~10] = {};
  show @len(e);

  // Several declarators share the element type.
  array int p[2] = {1, 2}, q[3] = {4, 5};
  show @p[1];
  show @q[2];
  return 0;
}
