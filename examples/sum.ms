// The program from your original draft, written in the final syntax.
moss 1000;

pick head {
  core: @read_int as input;
  moss: @output as show;
}

fn int add(int a, int b, int c) {
  return a + b + c;
}

fn int main() {
  int a, b, c;
  input @a;
  input @b;
  input @c;
  show @add(a, b, c);
  return 0;
}
