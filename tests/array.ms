moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  // fixed length
  array int squares[5];
  for (int i = 0; i < 5; i = i + 1) {
    squares[i] = i * i;
  }

  int sum;
  sum = 0;
  for (int i = 0; i < 5; i = i + 1) {
    sum = sum + squares[i];
  }
  show @sum;        // 0+1+4+9+16 = 30
  show @squares[3]; // 9

  // length from a variable; elements default to zero
  int n;
  n = 3;
  array string names[n];
  names[0] = "zero";
  names[1] = "one";
  names[2] = "two";
  show @names[1];   // one

  array bool flags[2];
  flags[0] = true;
  show @flags[0];   // true
  show @flags[1];   // false
  return 0;
}
