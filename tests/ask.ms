moss 1000;

pick head {
  core: @ask;
  moss: @output as show;
}

fn int main() {
  int age, extra;
  ask "Age? " @age;   // prints the prompt, then reads
  ask @extra;         // no prompt, just reads
  show @(age + extra);
  return 0;
}
