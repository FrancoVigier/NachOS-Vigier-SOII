// Simple standard library

//Undefined if s doesn't contain '\0'
unsigned
strlen(const char *s)
{
	unsigned size = 0;
	for (unsigned int i = 0; s[i] != '\0'; i++) {
		size++;
	}
	return size;
}

void
puts2(const char *s)
{
	unsigned int length = strlen(s);
	Write(s, length, CONSOLE_OUTPUT);
}

void
itoa(int n, char *str)
{
	unsigned base_10_strlen = 0;
	for(int i = n; i > 0; i /= 10) {
		base_10_strlen++;
	}

	for(int i = base_10_strlen-1, value = n; i < base_10_strlen; i--, value /= 10) {
		str[i] = '0' + (value%10);
	}

	str[base_10_strlen] = '\0';
}

char
strcmpp(const char *s1, const char *s2)
{
    unsigned iter = 0;
    for(;   s1[iter] != '\0'&& s2[iter] != '\0'&& s1[iter] == s2[iter]; ++iter);

    if(s1[iter] == '\0' && s2[iter] == '\0')
        return 1;
    return 0;
}
