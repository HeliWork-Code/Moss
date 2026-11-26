moss 1000;

pick head {
  moss: @output as show;
}

// Arrays are passed by handle: the callee sees the caller's elements.
fn void fill(array int a, int n) {
  for (int i = 0; i < n; i = i + 1) {
    a[i] = i * 2;
  }
}

fn int total(array int a, int n) {
  int s;
  s = 0;
  for (int i = 0; i < n; i = i + 1) {
    s = s + a[i];
  }
  return s;
}

fn int main() {
  array int v[4];
  fill(v, 4);
  show @total(v, 4);   // 0 + 2 + 4 + 6 = 12
  return 0;
}
