a = 0;

b = 1;
c = 0;

i = 0;
while (a > -1) {
	print(a);
	if(a == 0) {
		a = b;
	} else {
		a = c;
	}

	if(i == 100) {
		tmp = b;
		b = c;
		c = tmp;
		i = 0;
	}

	i = i + 1;
}

a;
