moss 1000;

pick head {
  moss: @output as show;
}

fn int add(int a, int b) {
  return a + b;
}

fn int twice(int x) {
  return add(x, x);
}

fn int main() {
  show(twice(21));
  return 0;
}
