// expect-error: switch quantity not an integer
int main() { float f = 1; switch (f) { case 1: return 1; } return 0; }
