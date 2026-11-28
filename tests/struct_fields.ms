moss 1000;

pick head {
  core: @len, @grow;
  moss: @output as show;
}

struct Point {
  int x;
  int y;
}

struct Buffer {
  array int data;
  Point origin;
  int count;
}

fn int main() {
  Buffer b;

  // An array field starts empty and grows in place.
  grow @b.data 3;
  b.data[0] = 10;
  b.data[1] = 20;
  show @len(b.data);
  show @b.data[0];
  show @b.data[1];
  show @b.data[2];

  // A struct field starts as a fresh zeroed object.
  show @b.origin.x;
  b.origin.x = 7;
  b.origin.y = 8;
  show @b.origin.x;
  show @b.origin.y;

  // Plain fields are unaffected.
  show @b.count;

  // Two instances have their own arrays.
  Buffer c;
  show @len(c.data);
  grow @c.data 1;
  show @len(c.data);
  show @len(b.data);
  return 0;
}
