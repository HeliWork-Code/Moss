// `moss` has no capability named @nope -- picking it must fail.
moss 1000;

pick head {
  moss: @nope as show;
}

fn int main() {
  return 0;
}
