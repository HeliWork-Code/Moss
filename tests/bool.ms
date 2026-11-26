moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  bool t, f;
  t = true;
  f = false;
  show(t && !f);
  show(t || f);
  show(3 < 5);
  show(3 >= 5);
  show("ab" == "ab");
  show("ab" != "cd");
  if (t) {
    show("yes");
  } else {
    show("no");
  }
  return 0;
}
