moss 1000;

pick head {
  moss: @output as show;
}

fn string classify(int n) {
  if (n < 0) {
    return "negative";
  } else if (n == 0) {
    return "zero";
  } else {
    return "positive";
  }
}

fn int main() {
  show(classify(-5));
  show(classify(0));
  show(classify(7));
  return 0;
}
