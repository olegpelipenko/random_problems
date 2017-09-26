#pragma once

#include <list>
#include <string>
#include <algorithm>
#include <mutex>
#include <set>
#include <vector>
#include <iostream>
#include <sstream>
#include <stack>
#include <fstream>

namespace
{

const char Digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const std::vector<char> Operators = { '+', '-', '*', '/' };

// Number of digits in expression
const int N = 10;
const uint32_t ExpectedResult = 2017;

// For threads
const uint32_t MaxNumberOfThreads = 8;
const uint32_t WorkingSetSize = 30;

}

bool CheckRPNCorrectness(const std::string& rpn)
{
	int counter = 0;
	for (int i = 0; i < rpn.size(); ++i)
	{
		if (rpn[i] == '0')
			counter++;
		else if (rpn[i] == '.')
			counter--;
		if (counter == 0)
			return false;
	}

	return true;
}

std::vector<std::string> GenerateRPNPatterns(int n)
{
	std::string pattern;
	pattern.resize(2 * n - 3, '0');
	pattern.replace(pattern.begin() + n - 2, pattern.end(), n - 1, '.');
	std::reverse(pattern.begin(), pattern.end()); // for permutations generator

	std::vector<std::string> patterns;

	do
	{
		std::string tempPattern = pattern;
		tempPattern.insert(0, "00");
		if (CheckRPNCorrectness(tempPattern))
			patterns.push_back(tempPattern);
	} while (std::next_permutation(pattern.begin(), pattern.end()));

	return patterns;
}

void ReplaceWithDigits(std::string& str, int counter)
{
	for (int i = 0; i < str.size(); ++i)
	{
		if (str[i] == '0')
		{
			if (counter < 10)
				str[i] = Digits[counter];
			counter--;
		}
	}
}

int EvaluateRPN(const std::string& str)
{
	std::stack<float> operands;
	for (const auto& token : str)
	{
		if (isdigit(token))
		{
			int value = (token - '0');
			if (value == 0)
				value = 10;
			operands.push(value);
		}
		else
		{
			float right = operands.top();
			operands.pop();
			float left = operands.top();
			operands.pop();

			float result;

			switch (token)
			{
			case '+':
				operands.push(left + right);
				break;

			case '-':
				operands.push(left - right);
				break;

			case '*':
				operands.push(left * right);
				break;

			case '/':
				if (right == 0 || left == 0)
					return 0;
				result = left / right;
				operands.push(result);
				break;

			default:
				throw std::runtime_error("Operator not found");
			}
		}
	}

	// Check that mantissa is equals to 0
	float result = operands.top();
	return (result - (long)result == 0) ? result : 0;
}

// Vector with synchronization
class ResultsArray
{
public:

	void Insert(const std::string& r)
	{
		UniqueLock l(m_mutex);
		m_results.push_back(r);
	}

	std::vector<std::string>& GetArray()
	{
		return m_results;
	}

private:
	std::vector<std::string> m_results;
	typedef std::unique_lock<std::mutex> UniqueLock;
	std::mutex m_mutex;
};

void ReplaceDots(std::string& pattern, int current, ResultsArray& results)
{
	if (current == pattern.size())
	{
		if (ExpectedResult == EvaluateRPN(pattern))
			results.Insert(pattern);

		return;
	}

	int i = current;
	while (pattern[i] != '.' && i < pattern.size())
		i++;

	if (pattern[i] == '.')
	{
		for (const auto& op : Operators)
		{
			pattern[i] = op;
			ReplaceDots(pattern, i + 1, results);
		}
	}

	pattern[i] = '.';
}

bool NeedToWrap(const std::string& expression, char token, bool divider)
{
	if (expression.size() / 2 == count(expression.begin(), expression.end(), '*'))
	{
		if (expression.size() == 1)
			return false;
		else
			return (token == '/' && divider) ? true : false;
	}

	if (expression.size() == 3)
		return expression[1] != '*';

	return expression.size() > 2;
}

std::string ConvertToInfix(const std::string& str)
{
	std::stack<std::string> operands;
	for (const auto& token : str)
	{
		if (isdigit(token))
		{
			std::string s;
			if (token == '0')
				s += "10";
			else
				s += token;
			operands.push(s);
		}
		else
		{
			auto second = operands.top();
			operands.pop();

			auto first = operands.top();
			operands.pop();

			std::ostringstream expression;

			if (NeedToWrap(first, token, false))
				expression << "(" << first << ")";
			else
				expression << first;

			expression << token;

			if (NeedToWrap(second, token, true))
				expression << "(" << second <<  ")";
			else
				expression << second;

			operands.push(expression.str());
		}
	}
	
	return operands.top();
}

void main(int argc, char** argv)
{
	bool verbose = false;
	if (argc > 1 && std::string(argv[1]) == std::string("-v"))
		verbose = true;

	clock_t begin = clock();
	
	// Generate and prepare patterns
	auto patterns = GenerateRPNPatterns(N);
	for (auto it = patterns.begin(); it != patterns.end(); ++it)
		ReplaceWithDigits(*it, N);

	ResultsArray results;
	std::list<std::thread> workers;
	uint32_t currentPos = 0;
	std::mutex patternsMutex;

	// Replace patterns with operators
	for (size_t t = 0; t < MaxNumberOfThreads; ++t)
	{
		workers.push_back(std::thread([&results, &patterns, &patternsMutex, begin, &currentPos, verbose]() {
			while (true)
			{
				std::vector<std::string> workingSet;
				{
					std::unique_lock<std::mutex> lock(patternsMutex);
					if (currentPos != patterns.size())
					{
						if (verbose && currentPos % WorkingSetSize == 0)
						{
							clock_t end = clock();
							std::cout << double(end - begin) / CLOCKS_PER_SEC << " " << currentPos << std::endl;
						}

						if (currentPos + WorkingSetSize < patterns.size())
						{
							workingSet.assign(patterns.begin() + currentPos, patterns.begin() + currentPos + WorkingSetSize);
							currentPos += WorkingSetSize;
						}
						else
						{
							workingSet.assign(patterns.begin() + currentPos, patterns.begin() + patterns.size() - 1);
							currentPos = patterns.size();
						}
					}
					else
					{
						break;
					}
				}

				for (auto& pattern : workingSet)
					ReplaceDots(pattern, 0, results);

			}
		}));
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.join(); });

	std::set<std::string> uniqueArray;
	const auto& resultsArray = results.GetArray();

	for (int i = 0; i < resultsArray.size(); i++)
		uniqueArray.insert(ConvertToInfix(resultsArray[i]));

	for (auto result = uniqueArray.begin(); result != uniqueArray.end(); ++result)
		std::printf("%s\n", result->c_str());
	
	if (verbose)
	{
		clock_t end = clock();
		std::cout << double(end - begin) / CLOCKS_PER_SEC << " millisecond" << std::endl;
	}
}