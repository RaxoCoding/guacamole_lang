a = 0;

funk add(a, b) {
	a + b;
}

funk checkBigger(a, b) {
	if(a > b) {
		return 1;
	}
	return 0;
}

while (1) {
	a = add(a, 1);
	if(checkBigger(a, 5)) {
		break;
	}
}
a;
