moss 1000;

pick head {
  moss: @output as show;
}

struct Point {
  int x;
  int y;

  // Methods see the fields directly; `self` is the receiver.
  fn int sum() { return x + y; }

  fn void move(int dx, int dy) {
    x = x + dx;
    y = y + dy;
  }

  fn Point add(Point o) {
    Point r;
    r.x = x + o.x;
    r.y = y + o.y;
    return r;
  }
}

fn int main() {
  Point p;
  p.x = 3;
  p.y = 4;
  show @p.sum();        // 7

  p.move(1, 1);
  show @p.sum();        // 9

  Point q;
  q.x = 10;
  q.y = 20;
  Point r = p.add(q);
  show @r.sum();        // 9 + 30 = 39

  // for-in over a range and over an array
  array int a[4];
  for i in 0~3 {
    a[i] = i * i;
  }
  int total;
  total = 0;
  for v in a {
    total = total + v;
  }
  show @total;          // 0 + 1 + 4 + 9 = 14

  array string names[2];
  names[0] = "hi";
  names[1] = "there";
  for s in names {
    show @s;
  }
  return 0;
}
