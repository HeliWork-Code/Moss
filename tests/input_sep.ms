moss 1000;

pick head {
  core: @read_int_sep as input_sep;
  moss: @output as show;
}

fn int main() {
  int a, b;
  input_sep @a @"," @b;   // read "4,6" -> a = 4, b = 6
  show(a + b);
  return 0;
}
