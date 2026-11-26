moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  int sum;
  sum = 0;
  for (int i = 1; i <= 5; i = i + 1) {
    sum = sum + i;
  }
  show @sum;          // 15

  for (int j = 3; j > 0; j = j - 1) {
    show @j;          // 3 2 1
  }

  for (int k = 1; k <= 16; k = k * 2) {
    show @k;          // 1 2 4 8 16
  }

  // All three clauses may be omitted; loop variable stays inside the loop.
  int n;
  n = 0;
  for (;;) {
    n = n + 1;
    if (n == 3) {
      return 0;
    }
  }
  return 0;
}
