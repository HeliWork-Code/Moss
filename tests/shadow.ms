moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  int x;
  x = 1;
  {
    int x;
    x = 2;
    show(x);
  }
  show(x);
  return 0;
}
