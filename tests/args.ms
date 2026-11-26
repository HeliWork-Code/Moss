moss 1000;

pick head {
  moss: @output as show;
}

// `args` marks that the runtime supplies the command line as one string.
fn int main(args string argv) {
  show @argv;
  return 0;
}
