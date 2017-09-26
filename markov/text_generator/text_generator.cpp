#include <iostream>
#include <vector>
#include "../model.h"
#include "../common.h"

#include <boost\program_options.hpp>
#include <boost\lexical_cast.hpp>

using namespace std;

wstring Convert(const string& str)
{
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

int main(int argc, char** argv)
{
	try
	{
		namespace po = boost::program_options;
		po::options_description description("Allowed options");
		description.add_options()("words", po::value<uint32_t>(), "number of words you want to generate")
			("input", po::value<string>(), "input sequence of words (quantity will be used as order of model)")
			("model", po::value<string>(), "path to file of model");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, description), vm);
		po::notify(vm);

		if (vm["input"].empty() || vm["words"].empty() || vm["model"].empty())
		{
			cout << description << endl;
			return 1;
		}

		uint32_t k = vm["words"].as<uint32_t>(); // number of words to build
		
		wstring beginSentence = Convert(vm["input"].as<string>());
		vector<wstring> words;
		boost::split(words, beginSentence, boost::is_any_of(" \t"));

		uint32_t order = words.size();
		string pathToModel = vm["model"].as<string>();

		wstring currentSentence;
		wstring resultText;
		resultText += beginSentence;

		uint32_t counter = 0;
		MarkovChainView model(pathToModel, order);

		Sentence s(beginSentence, order);
		while (counter < k)
		{
			wstring nextWord;
			if (!model.GetNextWord(s.GetKey(), nextWord))
				break;

			s.InsertWord(nextWord);
			resultText += L" ";
			resultText += nextWord;

			counter++;
		}

		wcout << resultText << endl;
	}
	catch (exception& e)
	{
		cout << "Fail: " << e.what() << endl;
		return 1;
	}

	return 0;
}