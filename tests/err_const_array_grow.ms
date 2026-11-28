moss 1000;

pick head {
  core: @grow;
  moss: @output as show;
}

fn int main() {
  const array int digits[3] = {1, 2, 3};
  grow @digits 1;
  show @digits[0];
  return 0;
}
