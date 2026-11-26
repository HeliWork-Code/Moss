moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  int a, b;
  a = 7;
  b = 3;
  show(a + b);
  show(a - b);
  show(a * b);
  show(a / b);
  show(a % b);
  show(-a);
  return 0;
}
