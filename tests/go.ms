moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  // Filtered iteration: `ex` is the implicit loop variable.
  __go (1 ~ 5) when (ex % 2 == 0) {
    show @ex;        // 2 4
  }

  int sum;
  sum = 0;
  __go (1 ~ 4) {
    sum = sum + ex;
  }
  show @sum;         // 10

  __go (1 ~ 10) when (ex % 3 == 0) {
    sum = sum + ex;
  }
  show @sum;         // 10 + 3 + 6 + 9 = 28
  return 0;
}
