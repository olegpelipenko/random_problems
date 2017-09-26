#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <set>

#include "C:\\dev\\bigint\\BigIntegerLibrary.hh"

using namespace std;

/*
Задача 1

Если мы возьмем 47, перевернем его и сложим, получится 47 + 74 = 121 — число-палиндром.

Если взять 349 и проделать над ним эту операцию три раза, то тоже получится палиндром:
349 + 943 = 1292
1292 + 2921 = 4213
4213 + 3124 = 7337

Найдите количество положительных натуральных чисел меньших 12185 таких, 
что из них нельзя получить палиндром за 50 или менее применений описанной операции 
(операция должна быть применена хотя бы один раз).
*/


struct BigNumber
{
	BigNumber(uint32_t num)
	{
		stringstream str;
		str << num;
		m_number = str.str();
	}

	BigNumber operator+(BigNumber& other)
	{
		if (this->m_number.length() < other.m_number.length())
		{
			string temp(other.m_number.length(), '0');
			temp.replace(temp.length() - this->m_number.length(), this->m_number.length(), &this->m_number[0]);
			this->m_number = temp;
		}

		const auto& lhv = this->m_number;
		const auto& rhv = other.m_number;
		
		bool plusOne = false;
		int t = lhv.size() - 1, o = rhv.size() - 1;
		for (; t >= 0, o >= 0; --t, --o)
		{
			int lhvI = lhv[t] - '0';
			int rhvI = rhv[o] - '0';
			int sum = (lhvI + rhvI);
			if (plusOne)
				sum++;

			if (sum >= 10)
				plusOne = true;
			else
				plusOne = false;

			m_number[t] = (sum % 10) + '0';
		}

		if (plusOne)
			this->m_number.insert(0, 1, '1');
		return *this;
	}

	BigNumber& operator=(BigNumber& other)
	{
		m_number = other.m_number;
		return *this;
	}

	BigNumber& reverse()
	{
		std::reverse(m_number.begin(), m_number.end());
		return *this;
	}

	bool IsPalindrome()
	{
		auto beg = 0;
		auto end = m_number.length() - 1;
		if (m_number[beg] == '0')
		{
			while (m_number[beg] != '0')
				beg++;
		}
		while (beg < end)
		{
			if (m_number[beg] != m_number[end])
				return false;
			beg++;
			end--;
		}
		return true;
	}


	string m_number;
};

ostream& operator<<(ostream& stream, const BigNumber& bn)
{
	stream << bn.m_number.c_str();
	return stream;
}

void CountPalindromes()
{
	uint32_t numberOfNotPalindromes = 0;
	uint32_t counter = 1; 
	while (counter < 12185)
	{
		BigNumber bn1(counter), bn2(counter);
		
		uint32_t palindromeCounter = 0;
		while (palindromeCounter <= 50)
		{
			BigNumber sum = (bn1 + bn2.reverse());
			if (sum.IsPalindrome())
			{
				cout << counter << " is palindrome " << bn1 << " " << palindromeCounter << endl;
				break;
			}

			bn1 = sum;
			bn2 = sum.reverse();
			palindromeCounter++;
		}

		if (palindromeCounter == 50)
		{
			numberOfNotPalindromes++;
			cout << counter << " is not palindrome" << endl;
		}

		counter++;
	}

	cout << "Total: " << numberOfNotPalindromes;
}

int64_t reverse(int64_t num)
{
	std::string s = std::to_string(num);
	std::reverse(s.begin(), s.end());
	return stoll(s);
}

bool IsPalindrome(int64_t num)
{
	int64_t n = num;
	int64_t rev = 0;
	while (num > 0)
	{
		int64_t dig = num % 10;
		rev = rev * 10 + dig;
		num = num / 10;
	}
	return n == rev;
}

void CountPalindromesSimple()
{
	uint32_t numberOfNotPalindromes = 0;
	uint32_t counter = 1;
	while (counter < 12185)
	{
		int64_t bn1(counter), bn2(counter);

		uint32_t palindromeCounter = 0;
		while (palindromeCounter <= 50)
		{
			int64_t sum = (bn1 + reverse(bn2));
			if (IsPalindrome(sum))
			{
				cout << counter << " is palindrome " << bn1 << " " << palindromeCounter << endl;
				break;
			}

			bn1 = sum;
			bn2 = reverse(sum);
			palindromeCounter++;
		}

		if (palindromeCounter == 50)
		{
			numberOfNotPalindromes++;
			cout << counter << " is not palindrome" << endl;
		}

		counter++;
	}

	cout << "Total: " << numberOfNotPalindromes;
}

/*

Рассмотрим спираль, в которой, начиная с 1 в центре, последовательно расставим числа по часовой стрелке,
пока не получится спираль 5 на 5

21 22 23 24 25
20  7  8  9 10
19  6  1  2 11
18  5  4  3 12
17 16 15 14 13
Можно проверить, что сумма всех чисел на диагоналях равна 101.

Чему будет равна сумма чисел на диагоналях, для спирали размером 1169 на 1169?

*/

uint64_t SpiralingArray(uint32_t n)
{
	uint64_t result = 0;
	uint32_t **array = new uint32_t* [n] {0};
	for (uint32_t i = 0; i < n; ++i)
		array[i] = new uint32_t[n]{ 0 };

	uint32_t r = n / 2, c = n / 2;

	//x += 1

	//array[n/2][n/2] = 1;

	for (uint32_t i = 0; i < n; ++i)
	{

	}


	for (uint32_t i = 0; i < n; ++i)
	{
		for (uint32_t j = 0; j < n; ++j)
		{
			cout << array[i][j] << " ";
		}
		cout << endl;
	}

	return result;
}

string IntToString(uint32_t num)
{
	stringstream str;
	str << num;
	return str.str();
}

/*
Число 125874 и результат умножения его на 2 — 251748 можно получить друг из друга перестановкой цифр.
Найдите наименьшее положительное натуральное x такое, что числа 3*x, 4*x можно получить друг из друга перестановкой цифр.
*/

uint32_t Permutations()
{
	// 3, 4
	// x

	uint32_t x = 1;

	while (true)
	{
		if (x % 100 == 0)
			cout << x << endl;

		uint32_t x3(x * 3), x4(x * 4);
		string x3str = IntToString(x3), x4str = IntToString(x4);
		sort(x3str.begin(), x3str.end());
		sort(x4str.begin(), x4str.end());
		if (x3str == x4str)
			break;
		x++;
	}

	return x;
}

int ToInt(char ch)
{
	return ch - '0';
}

uint32_t MarvelousNumbers(uint32_t lo, uint32_t hi)
{
	uint32_t number = lo;
	uint32_t counter = 0;
	vector<string> notSorted;


	while (number != hi)
	{
		vector<string> sequences;
		string numberStr = IntToString(number);

		// Найти все последовательности
		for (auto ch = numberStr.begin(); ch != numberStr.end(); ++ch)
		{
			string current;
			current.push_back(*ch);
			int sum = ToInt(*ch);
			for (auto next = (ch + 1); next != numberStr.end(); ++next)
			{
				current.push_back(*next);
				sum += ToInt(*next);
				if (sum > 10)
					break;
				else if (sum == 10)
					sequences.push_back(current);
			}
		}

		// Пройтись по ним и выявить какие в них есть числа уникальные
		string uniqueNumbers;
		for (auto seq : sequences)
		{
			for (auto ch : seq)
			{
				if (string::npos == uniqueNumbers.find(ch))
					uniqueNumbers.push_back(ch);
			}
		}

		// просортировать число и уникальные числа последовательностей и сравнить все ли на месте
		sort(uniqueNumbers.begin(), uniqueNumbers.end());
		auto ns = numberStr;
		sort(numberStr.begin(), numberStr.end());
		if (uniqueNumbers == numberStr)
		{
			notSorted.push_back(ns);
			counter++;
		}

		number++;
	}

	for (auto num : notSorted)
		cout << num << endl;
	return counter;
}

/*
Рассмотрим все возможные числа ab для 1<a<6 и 1<b<6:
22=4, 23=8, 24=16, 25=32 32=9, 33=27, 34=81, 35=243 42=16, 43=64, 44=256, 45=1024, 52=25, 53=125, 54=625, 55=3125
Если убрать повторения, то получим 15 различных чисел.

Сколько различных чисел ab для 2<a<107 и 2<b<146?
*/

uint32_t Powers(uint32_t alo, uint32_t ahi, uint32_t blo, uint32_t bhi)
{
	set<uint32_t> result;

	uint32_t aC = alo + 1, bC = blo + 1;
	for (uint32_t aC = (alo + 1); aC < ahi; aC++)
	{
		for (uint32_t bC = (blo + 1); bC < bhi; bC++) 
		{
			result.insert(pow(aC, bC));
		}
	}

	return result.size();
}

BigInteger factorial2(BigInteger n)
{
	BigInteger ret = 1;
	for (BigInteger i = 1; i <= n; ++i)
		ret *= i;
	return ret;
}

void factors(uint64_t n)
{
	uint64_t z = 2;
	while (z * z <= n)
	{
		if (n % z == 0)
		{
			cout << z << endl;
			n /= z;
		}
		else
		{
			z++;
		}
	}
	if (n > 1)
		cout << n << endl;
}


// Maximum number of digits in output
#define MAX 500

int multiply(int x, int res[], int res_size);
int res[MAX];

// This function finds factorial of large numbers and prints them
void factorial3(int n)
{

	// Initialize result
	res[0] = 1;
	int res_size = 1;

	// Apply simple factorial formula n! = 1 * 2 * 3 * 4...*n
	for (int x = 2; x <= n; x++)
		res_size = multiply(x, res, res_size);

	/*
	cout << "Factorial of given number is \n";
	for (int i = res_size - 1; i >= 0; i--)
		cout << res[i];
		*/
	//return res;
}

// This function multiplies x with the number represented by res[].
// res_size is size of res[] or number of digits in the number represented
// by res[]. This function uses simple school mathematics for multiplication.
// This function may value of res_size and returns the new value of res_size
int multiply(int x, int res[], int res_size)
{
	int carry = 0;  // Initialize carry

					// One by one multiply n with individual digits of res[]
	for (int i = 0; i<res_size; i++)
	{
		int prod = res[i] * x + carry;
		res[i] = prod % 10;  // Store last digit of 'prod' in res[]
		carry = prod / 10;    // Put rest in carry
	}

	// Put carry in res and increase result size
	while (carry)
	{
		res[res_size] = carry % 10;
		carry = carry / 10;
		res_size++;
	}
	return res_size;
}

BigInteger Factorial(BigInteger M, BigInteger N)
{
	BigInteger sum = 0;
	
	for (BigInteger counter = M; counter <= N; ++counter)
	{
		BigInteger fac = 1;
		BigInteger currentRes = 1;
		while (true)
		{
			if (currentRes % counter == 0)
			{
				sum += fac;
				cout << currentRes << " " << counter << " " << fac << endl;
				break;
			}
			fac++;
			currentRes = factorial2(fac);
		}
	}

	return sum;
}

void HunterTest()
{
	/*
	BigNumber bn1(123);
	cout << bn1.IsPalindrome();

	BigNumber bn2(121);
	cout << bn2.IsPalindrome();

	BigNumber bn3(1219);
	cout << bn3.IsPalindrome();

	BigNumber bn4(283382);
	cout << bn4.IsPalindrome();

	BigNumber bn5(28382);
	cout << bn5.IsPalindrome();
	*/
	//BigNumber bn1(10), bn2(10);
	//cout << bn1 + bn2;
	//BigNumber bn1(8888), bn2(999999);
	//BigNumber sum = (bn1 + bn2.reverse());
	//cout << sum << endl;
	
	//cout << Permutations();
	//cout << MarvelousNumbers(1, 9500000) << endl;
	//cout << Powers(2, 107, 2, 146);
	//factors(10000);

	//cout << Factorial(75000000, 75000003) << endl;
	factorial3(1000000);
	for (int i = MAX - 1; i >= 0; i--)
		cout << res[i];

	//BigNumber bn1(244), bn2(244);
	//BigNumber sum = (bn1 + bn2.reverse());
	//cout << sum << " " << sum.IsPalindrome() << endl;
	//SpiralingArray(10);
	//CountPalindromes();
	//CountPalindromesSimple();


}