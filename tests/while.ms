moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  int i, sum;
  i = 0;
  sum = 0;
  while (i < 5) {
    sum = sum + i;
    i = i + 1;
  }
  show(sum);
  return 0;
}
