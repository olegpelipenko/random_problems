#ifndef UTILS_H
#define UTILS_H

#include <random>

bool IsFileExist(const std::string& name) 
{
	struct stat buffer;
	if (stat(name.c_str(), &buffer) == 0)
		return buffer.st_mode & S_IFREG;
	else
		return false;
}

long GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

class FileNamesList
{
	typedef std::vector<std::string> FileNamesArrayType;
public:

	void Add(const std::string& value)
	{
		UniqueLock lock(m_mutex);
		m_array.push_back(value);
	}

	size_t Size()
	{
		UniqueLock lock(m_mutex);
		return m_array.size();
	}

	std::string PopBack()
	{
		UniqueLock lock(m_mutex);
		std::string file = m_array.back();
		m_array.pop_back();
		return file;
	}

	std::string operator[](size_t index)
	{
		UniqueLock lock(m_mutex);
		return m_array[index];
	}

private:
	FileNamesArrayType m_array;
	typedef std::unique_lock<std::mutex> UniqueLock;
	std::mutex m_mutex;
};

std::string GetRandomFileName(const std::string& tmpDir)
{
	std::string tmpName(tmpDir);
	tmpName += "\\";
	std::string name = std::tmpnam(nullptr);
	tmpName += name.erase(0, name.find_last_of('\\') + 1);
	return tmpName;
}

class Semaphore
{
public:
	Semaphore() : m_count(0)	{}

	void Increment() 
	{
		UniqueLock lock(m_mutex);
		++m_count;
	}

	void Decrement() {
		UniqueLock lock(m_mutex);
		if (--m_count == 0)
			m_condition.notify_all();
	}

	void Wait() {
		UniqueLock lock(m_mutex);
		while (m_count)
			m_condition.wait(lock);
	}

private:
	std::mutex m_mutex;
	std::condition_variable m_condition;
	uint32_t m_count;
	typedef std::unique_lock<decltype(m_mutex)> UniqueLock;

};

#endif // UTILS_H