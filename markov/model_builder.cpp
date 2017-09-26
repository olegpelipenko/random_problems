
#include <cwctype>
#include <list>
#include <boost\format.hpp>
#include <boost\program_options.hpp>
#include <unordered_map>
#include "model.h"
#include "common.h"

template<class T, class T2>
void PreprocessString(vector<T>& buffer, vector<T2>& result)
{
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		if (std::iswupper(buffer[i]))
			result.push_back(towlower(buffer[i]));
		else if (iswlower(buffer[i]))
			result.push_back(buffer[i]);
		else if (buffer[i] == '\'' || iswspace(buffer[i]))
			result.push_back(buffer[i]);
	}
}

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

vector<wchar_t> DownloadUrl(const wstring& url)
{
	deleted_unique_ptr<FILE> file(_wpopen((boost::wformat(L"\"%1%\" -s --url %2%") % CurlPath % url).str().c_str(), L"rt"), 
		[] (FILE* fp) { _pclose(fp); });
	
	Check(file.get() == NULL, "Can't download url");

	vector<char> buffer(BufferSize, 0);
	vector<wchar_t> result;
	result.reserve(BufferSize);
	size_t readed = 0;

	while (true)
	{
		readed = fread(&buffer[0], sizeof buffer[0], buffer.size(), file.get());
		if (readed < BufferSize)
			buffer.resize(readed);

		PreprocessString(buffer, result);

		if (readed != BufferSize)
			break;
	}
	
	return result;
}

void ReadLinks(const string& filePath, list<wstring>& links)
{
	if (filePath.empty())
		return;

	wifstream stream(filePath);
	Check(!stream.is_open(), (boost::format("Can't open file: %1%") % filePath).str());
	
	wstring line;
	while (std::getline(stream, line))
		links.push_back(line);
}

int main(int argc, char** argv)
{
	try
	{
		namespace po = boost::program_options;
		po::options_description description("Allowed options");
		description.add_options()("order", po::value<uint32_t>(), "order of model")
								 ("urls", po::value<string>(), "path to txt file with links to download")
								 ("out", po::value<string>(), "path to file of result model");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, description), vm);
		po::notify(vm);

		if (vm.count("help") || vm["order"].empty() || vm["urls"].empty() || vm["out"].empty()) 
		{
			cout << description << endl;
			return 1;
		}

		uint32_t order = vm["order"].as<uint32_t>();
		string pathToResultModel(vm["out"].as<string>());

		list<wstring> links;
		ReadLinks(vm["urls"].as<string>(), links);

		Check(links.empty(), "No links to download");
		Check(!IsFileExists(CurlPath), "curl.exe not exists, place it near bin file");

		MarkovChainModel completeModel(order);
		
		while (!links.empty())
		{
			auto link = links.front();
			links.pop_front();

			try
			{
				MarkovChainModel tempModel(order);
				if (tempModel.CreateModel(DownloadUrl(link)))
				{
					wcout << "Model for: " << link << " created, size: " << tempModel.GetSize() << endl;
					completeModel.Merge(tempModel);
					wcout << "After merge: " << completeModel.GetSize() << endl;
				}
			}
			catch (exception& e)
			{
				wcout << "Download error: " << link <<", error: " << e.what() << endl;
			}
		}
		cout << "All urls downloaded, saving model to file" << endl;
		completeModel.Save(pathToResultModel);
		cout << "Model was saved to: " << pathToResultModel;
	}
	catch (std::exception& e)
	{
		cout << "Error: " << e.what();
		return 1;
	}

	return 0;
}