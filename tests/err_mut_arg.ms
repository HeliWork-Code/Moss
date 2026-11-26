moss 1000;

pick head {
  core: @read_int as input;
  moss: @output as show;
}

fn int main() {
  int x;
  input(5);
  show(x);
  return 0;
}
