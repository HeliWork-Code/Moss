// The `@`-chain input style from the original draft:
//   ms.in@b@","@c
// is now written as a mention list of three arguments.
moss 1000;

pick head {
  core: @read_int_sep as input_sep;
  moss: @output as show;
}

fn int main() {
  int b, c;
  input_sep @b @"," @c;
  show @(b + c);
  return 0;
}
