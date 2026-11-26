moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  int x;
  x = 42;
  show @x;    // mention a variable
  show(@x);   // mention inside a normal call
  show 7;     // plain expression argument, no '@'
  return 0;
}
