#include <iostream>
#include "file_buffer.h"
#include <boost\program_options.hpp>
#include "sort.h"

template <typename T>
void GenerateFile(const std::string& fileName, const uint32_t bufferSize, const uint32_t bytesToGenerate, bool verbose)
{
	FileBuffer<T> buffer(bufferSize / sizeof(T), verbose);
	buffer.Open(fileName, "wb");
	FileBufferIterator<T> it(buffer);

	std::random_device r;
	std::default_random_engine e1(r());
	std::uniform_int_distribution<int> uniform_dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
	//std::uniform_int_distribution<int> uniform_dist(0, 100);

	const int32_t numberOfValues = bytesToGenerate / sizeof(T);
	for (int n = 0; n < numberOfValues; ++n)
		it.PushBack(uniform_dist(e1));

	it.Flush();
}

template<typename T>
void CheckSorted(const std::string& fileName, const uint32_t bufferSizeInBytes, bool verbose = false)
{
	if (!IsFileExist(fileName))
		throw std::runtime_error("File not exists");
	
	const size_t MaxNumberOfDigitsInBuffer = bufferSizeInBytes / sizeof(T);
	
	FileBuffer<T> buffer(MaxNumberOfDigitsInBuffer, verbose);
	buffer.Open(fileName);
	
	size_t totalNumberOfDigitsRead = 0;
	size_t digitsRead = buffer.Read();
	totalNumberOfDigitsRead += digitsRead;

	bool isSorted = true;
	while (digitsRead != 0)
	{
		if (digitsRead < MaxNumberOfDigitsInBuffer)
			buffer.Resize(digitsRead);

		isSorted = std::is_sorted(buffer.Begin(), buffer.End());

		if (!isSorted)
			break;
		
		if (digitsRead > 1)
		{
			buffer.Seek(-sizeof(T));
			totalNumberOfDigitsRead -= 1; // 
		}

		digitsRead = buffer.Read();
		totalNumberOfDigitsRead += digitsRead;
	}

	std::cout << "Total number of digits read: " << totalNumberOfDigitsRead << std::endl;
	std::cout << "File is " << (isSorted ? "sorted" : "not sorted") << std::endl;
}

bool ToBool(const std::string& s)
{
	if (s == "on" || s == "yes" || s == "1" || s == "true")
		return true;
	else if (s == "off" || s == "no" || s == "0" || s == "false")
		return false;
	return false;
}

int main(int argc, char** argv)
{
	try
	{
		namespace po = boost::program_options;
		po::options_description description("You can use only one of these allowed options at a time");
		description.add_options()("sort-file", po::value<std::string>(), "sort file, specify file path")
			("generate-file", po::value<std::string>(), "generate file with random ints, specify file path")
			("bytes-to-generate", po::value<uint32_t>(), "number of bytes to generate")
			("check-sorted", po::value<std::string>(), "check that file in sorted order, specify file path")
			("buffer-size", po::value<uint32_t>(), "Buffer size to work with (in bytes)")
			("verbose", po::value<std::string>(), "Set verbosity")
			("temp-dir", po::value<std::string>(), "Temporary dir path");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, description), vm);
		po::notify(vm);

		std::vector<bool> options = { vm["sort-file"].empty(), vm["generate-file"].empty(), vm["check-sorted"].empty() };
		if (vm.count("help") || /*count(options.begin(), options.end(), false) != (options.size() - 1) ||*/ vm["buffer-size"].empty())
		{
			std::cout << description << std::endl;
			return 1;
		}

		clock_t begin = clock();

		bool verbose = (!vm["verbose"].empty() ? ToBool(vm["verbose"].as<std::string>()) : false);
		if (!vm["generate-file"].empty())
		{
			if (vm["bytes-to-generate"].empty())
				throw std::runtime_error("Specify bytes-to-generate param");
			GenerateFile<int32_t>(vm["generate-file"].as<std::string>(), vm["buffer-size"].as<uint32_t>(), vm["bytes-to-generate"].as<uint32_t>(), verbose);
		}
		else if (!vm["sort-file"].empty())
		{
			if (vm["temp-dir"].empty())
				throw std::runtime_error("Specify temp-dir param");

			Sort<int32_t>(vm["sort-file"].as<std::string>(), vm["buffer-size"].as<uint32_t>(), vm["temp-dir"].as<std::string>(), verbose);
		}
		else
		{
			CheckSorted<int32_t>(vm["check-sorted"].as<std::string>(), vm["buffer-size"].as<uint32_t>(), verbose);
		}

		clock_t end = clock();
		std::cout << double(end - begin) / CLOCKS_PER_SEC << " millisecond" << std::endl;

	}
	catch (const std::exception& e)
	{
		std::cerr << "Error:" << e.what() << std::endl;
	}
}
