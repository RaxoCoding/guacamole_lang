// a simple program to get the nth fibonnaci number;

funk fib(int n) {
	if(n < 0) { 0; }
	elif(n == 1  || n == 2) { 1; }
	else { fib(n-1) + fib(n-2); }
}
int c = fib(9);
print(c);
