// a simple program to get the nth fibonnaci number;

funk fib(n) {
	if(n < 0) { 
		return 0; 
	}
	elif(n == 1  || n == 2) { 
		return 1; 
	}
	else { 
		return fib(n-1) + fib(n-2); 
	}
}
c = fib(9);
c;
