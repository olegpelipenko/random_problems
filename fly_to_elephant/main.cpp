#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <list>
#include <sys/stat.h>

#include "words_graph.h"
#include "helpers.h"


void ShowHelp(char** argv)
{
	std::cout << "Usage: " << argv[0] << " path_to_input_file path_to_dictionary" << std::endl;
}

bool IsFileExists(const std::string& name)
{
	struct stat buffer;
	if (stat(name.c_str(), &buffer) == 0)
		return buffer.st_mode & S_IFREG;
	else
		return false;
}

int main(int argc, char** argv)
{
	if (argc != 3 || !IsFileExists(argv[1]) || !IsFileExists(argv[2]))
	{
		ShowHelp(argv);
		return 0;
	}
	
	std::locale::global(std::locale(""));

	try
	{
		std::list<Word> firstTwoWords;
		{
			
			std::string filePath = argv[1];
			std::wifstream file(filePath);

			Word word;
			if (!std::getline(file, word) || word.empty())
				throw std::runtime_error("Empty input word");

			auto& facet = std::use_facet<std::ctype<typename Word::value_type>>(std::locale());

			facet.tolower(&word[0], &word[0] + word.size());
			firstTwoWords.push_back(word);
			word.clear();

			if (!std::getline(file, word) || word.empty())
				throw std::runtime_error("Empty input word");
			
			facet.tolower(&word[0], &word[0] + word.size());
			firstTwoWords.push_back(word);
		}


		std::string dictionaryPath = argv[2];
		Word fromWord = firstTwoWords.front(), toWord = firstTwoWords.back();

		if (fromWord.length() != toWord.length())
			throw std::runtime_error("Words are of different length");

		std::vector<Word> wl = FindPath(dictionaryPath, fromWord, toWord);
		if (!wl.empty())
		{
			for (const auto& p : wl)
				std::wcout << p << std::endl;
		}
		else
		{
			std::cout << "No path from '" << fromWord.c_str() << "' to '" << toWord.c_str() << "' " << std::endl;
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}
