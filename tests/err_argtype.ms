moss 1000;

pick head {
  moss: @output as show;
}

fn int add(int a, int b) {
  return a + b;
}

fn int main() {
  show(add(1, "two"));
  return 0;
}
