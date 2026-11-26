moss 1000;

pick head {
  moss: @output as show;
}

fn int area(int w, int h) { return w * h; }

fn int main() {
  const int size = 4;
  const int total = area(size, 3);
  show @size;        // 4
  show @total;       // 12

  int x = size + 1;
  show @x;           // 5
  return 0;
}
