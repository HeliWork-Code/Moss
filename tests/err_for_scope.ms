moss 1000;

// The loop variable must not escape the loop.
fn int main() {
  for (int i = 0; i < 3; i = i + 1) {
  }
  return i;
}
