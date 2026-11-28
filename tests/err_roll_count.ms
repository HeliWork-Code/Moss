moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  array int a[4] = {1, 2, 3, 4};
  __roll a[5] = {1, 2};
  show @a[0];
  return 0;
}
