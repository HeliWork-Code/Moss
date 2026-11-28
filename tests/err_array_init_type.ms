moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  array int a[3] = {1, "two", 3};
  show @a[0];
  return 0;
}
