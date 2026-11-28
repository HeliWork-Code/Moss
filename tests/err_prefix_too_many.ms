moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  array int a[4] = {1, 2, 3, 4};
  a[2] = {1, 2, 3};
  show @a[0];
  return 0;
}
