// `ask` is Moss's "问": print a prompt, then listen.
// Without a prompt it is a plain read.
moss 1000;

pick head {
  core: @ask;
  moss: @output as show;
}

fn int main() {
  int age;
  ask "How old are you? " @age;
  show "Next year you will be:";
  show @age + 1;
  return 0;
}
