// A mention list: every '@' starts one argument, so this is
// pair_up(@x, @2) -- the shape of your original `ms.out@a@b`.
moss 1000;

pick head {
  moss: @pair as pair_up;
}

fn int main() {
  int x;
  x = 1;
  pair_up @x @2;
  return 0;
}
