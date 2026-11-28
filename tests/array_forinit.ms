moss 1000;

pick head {
  core: @len;
  moss: @output as show;
}

fn int main() {
  // An array declared in a for-init still needs its declared length: both
  // backends must build it the same way.
  for (array int a[3]; true; ) {
    show @len(a);
    a[0] = 7;
    show @a[0];
    return 0;
  }
  return 0;
}
