moss 1000;

pick head {
  core: @len;
  moss: @output as show;
}

fn void dump(array int a) {
  for v in a {
    show @v;
  }
}

fn int main() {
  const array int digits[5] = {1, 2, 3};
  for d in digits {
    show @d;
  }
  show @len(digits);

  const array string names[2] = {"ann", "bob"};
  for n in names {
    show @n;
  }

  // A constant array can still be read, here through a parameter.
  dump(digits);
  return 0;
}
