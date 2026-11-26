moss 1000;

pick head {
  moss: @output as show;
}

fn int fib(int n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

fn int main() {
  show(fib(10));
  return 0;
}
