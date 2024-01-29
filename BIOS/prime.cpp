#include <string>

std::string primeCalc(int digits)
{
	bool isPrime = false;
	std::string ret;
	ret.clear();

	int x = 0;
	while (ret.length() < digits)
	{
		printf("Checking %d\n", x);
		if (x == 0 || x == 1)
		{
			x++;
			continue;
		}
		
		isPrime = true;

		for (int y = 2; y <= x / 2; y++)
		{
			if (x % y == 0)
			{
				isPrime = false;
				break;
			}
		}

		if (isPrime)
			ret += std::to_string(x);
		x++;
	}

	return ret;
}

int main()
{
	std::string s = primeCalc(10005).c_str();
	printf("%s\n", s.substr(0, 5).c_str());
	return s.size();
}