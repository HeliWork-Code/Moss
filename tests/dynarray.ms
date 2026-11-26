moss 1000;

pick head {
  core: @len, @grow, @shrink;
  moss: @output as show;
}

fn int main() {
  // initial length 2, at most 5 elements
  array int a[2~5];
  a[0] = 10;
  a[1] = 20;
  show @len(a);         // 2

  grow @a 2;            // now 4 elements
  a[2] = 30;
  a[3] = 40;
  show @len(a);         // 4
  show @(a[2] + a[3]);  // 70

  shrink @a 1;          // now 3 elements
  show @len(a);         // 3

  // empty and unbounded
  array int b[~];
  grow @b 3;
  b[0] = 7;
  show @(len(b) + b[0]);  // 3 + 7 = 10

  // started at 1, unbounded
  array string names[1~];
  names[0] = "first";
  grow @names 1;
  names[1] = "second";
  show @names[1];       // second
  return 0;
}
