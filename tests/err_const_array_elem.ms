moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  const array int digits[3] = {1, 2, 3};
  digits[0] = 9;
  show @digits[0];
  return 0;
}
