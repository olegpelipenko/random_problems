#ifndef SORT_H
#define SORT_H

#include <thread>
#include <list>
#include "utils.h"

template <class T>
void SplitFile(const std::string& filePath,	const std::string& tempDir,	uint32_t bufferSize, FileNamesList& files)
{
	FileBuffer<T> buffer(bufferSize / sizeof(T));
	buffer.Open(filePath);

	uint32_t numberOfChunks = 0;
	size_t bytesReaded = 0;
	const size_t fileSize = buffer.GetFileSize();

	while (bytesReaded != fileSize)
	{
		bytesReaded += buffer.Read();
		std::sort(buffer.Begin(), buffer.End());
		
		std::string chunkFileName = GetRandomFileName(tempDir).c_str();
		std::FILE* chunk = std::fopen(chunkFileName.c_str(), "wb");
		std::fwrite(&buffer.Get()[0], sizeof(buffer.Get()[0]), buffer.Get().size(), chunk);
		std::fclose(chunk);
		files.Add(chunkFileName);

		numberOfChunks++;
	}
}

template<class T>
void Merge(std::pair<std::string, std::string> filesToMerge, const std::string& tempDir, uint32_t bufferSize, FileNamesList& resultFilesList)
{
	{
		const uint32_t inBufferSize = bufferSize / (4 * sizeof(T)), outBufferSize = bufferSize / (2 * sizeof(T));

		FileBuffer<T> lBuffer(inBufferSize), rBuffer(inBufferSize), outBuffer(outBufferSize);
		lBuffer.Open(filesToMerge.first);
		rBuffer.Open(filesToMerge.second);
		auto outFileName = GetRandomFileName(tempDir);
		outBuffer.Open(outFileName, "wb");

		lBuffer.Read();
		rBuffer.Read();
		FileBufferIterator<T> outIt(outBuffer);
		MergeRoutine(FileBufferIterator<T>(lBuffer), FileBufferIterator<T>(rBuffer), outIt);
		outIt.Flush();
		resultFilesList.Add(outFileName);
	}
	std::remove(filesToMerge.first.c_str());
	std::remove(filesToMerge.second.c_str());
}

template<typename T>
void Sort(const std::string& fileName, const uint32_t bufferSize, const std::string& tempDir, bool verbose = false)
{
	if (!IsFileExist(fileName))
		throw std::runtime_error("File not exists");

	const uint32_t MaxNumberOfThreads = 8; // Assume, that max number of cores is 8, so for max efficiency use 8 threads
	const size_t OptimalFileSize = 30 * 1024 * 1024; // 50 Mb
	const size_t BufferForEachThread = bufferSize / MaxNumberOfThreads;

	// Split file to chunks
	std::cout << "Splitting..." << std::endl;
	FileNamesList resultFiles;
	SplitFile<T>(fileName, tempDir, bufferSize <= OptimalFileSize ? bufferSize : OptimalFileSize, resultFiles);
	
	std::cout << "Merging..." << std::endl;
	std::cout << "Files to merge: " << resultFiles.Size() << std::endl;

	// Merge chunks in one file, until one file exists
	while (resultFiles.Size() != 1)
	{
		std::cout << "Starting new threads..." << std::endl;
		
		Semaphore sem;
		std::list<std::thread> workers;

		for (size_t t = 0; t < MaxNumberOfThreads && (resultFiles.Size() >= 2); ++t)
		{
			std::pair<std::string, std::string> filesToMerge;
			filesToMerge.first = resultFiles.PopBack();
			filesToMerge.second = resultFiles.PopBack();
			
			workers.push_back(std::thread([BufferForEachThread, filesToMerge, &tempDir, &sem, &resultFiles]() {
				sem.Increment();
				Merge<T>(filesToMerge, tempDir, BufferForEachThread, resultFiles);
				sem.Decrement();
			}));

			std::cout << "Thread " << workers.back().get_id() << " starts, files: " << filesToMerge.first << ", " << filesToMerge.second << std::endl;
		}

		sem.Wait();
		std::cout << "Threads finished work, remaining number of files to merge: " << resultFiles.Size() << std::endl;
		std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.join(); });
	}
		
	std::cout << "Sorted file: " << resultFiles[0] << std::endl;
}

#endif // SORT_H