moss 1000;

pick head {
  core: @read_int as input;
  moss: @output as show;
}

fn int main() {
  int a, b;
  input @a;
  input @b;
  show(a + b);
  return 0;
}
