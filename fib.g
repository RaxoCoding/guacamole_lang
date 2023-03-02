// a simple program to get the nth fibonnaci number;

funk fib(n) {
	if(n < 0) { 0; }
	elif(n == 1  || n == 2) { 1; }
	else { fib(n-1) + fib(n-2); }
}
c = fib(9);
print(c);
