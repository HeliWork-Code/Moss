moss 1000;

pick head {
  moss: @output as show;
}

fn void greet(string name) {
  show("Hi, " + name);
}

fn int main() {
  greet("Moss");
  return 0;
}
