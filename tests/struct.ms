moss 1000;

pick head {
  moss: @output as show;
}

struct Point {
  int x;
  int y;
  string label;
}

fn int sum(Point p) {
  return p.x + p.y;
}

fn void bump(Point p) {
  p.x = p.x + 1;   // structs are references: callers see this change
}

fn int main() {
  Point p;
  p.x = 3;
  p.y = 4;
  p.label = "orig";
  show(p.x);
  show(p.label);
  bump(p);
  show(p.x);
  Point q;
  q = p;           // q and p name the same object
  q.y = 100;
  show(p.y);
  show(sum(p));
  return 0;
}
