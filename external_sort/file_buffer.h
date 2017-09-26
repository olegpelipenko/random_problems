#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <assert.h>
#include <memory>
#include <vector>
#include <functional>
#include <condition_variable>
#include <mutex>

template<typename T1>
using deleted_unique_ptr = std::unique_ptr<T1, std::function<void(T1*)>>;

size_t GetFileSize(const std::string& fileName)
{
	auto file = std::unique_ptr<FILE, std::function<void(FILE*)>>(std::fopen(fileName.c_str(), "rb"), [](FILE* fp) { std::fclose(fp); });
	std::fseek(file.get(), 0, SEEK_END);
	const size_t fileSize = std::ftell(file.get());
	std::fseek(file.get(), 0, SEEK_SET);
	return fileSize;
}

template<class T>
class FileBuffer;

template<class T>
class FileBufferIterator;

template <class T>
void MergeRoutine( FileBufferIterator<T>& lhv,  FileBufferIterator<T>& rhv, FileBufferIterator<T>& result)
{
	while (true)
	{
		if (lhv.Current() < rhv.Current())
		{
			result.PushBack(lhv.Current());
			if (!lhv.Next())
				break;
		}
		else
		{
			result.PushBack(rhv.Current());
			if (!rhv.Next())
				break;
		}
	}
	
	if (lhv.IsValid())
	{
		do
		{
			result.PushBack(lhv.Current());
		} while (lhv.Next());
	}

	if (rhv.IsValid())
	{
		do
		{
			result.PushBack(rhv.Current());
		} while (rhv.Next());
		
	}
}

template<class T>
class FileBufferIterator
{
public:
	FileBufferIterator(FileBuffer<T>& buffer) : m_fileBuffer(buffer), m_currentPos(0) {}

	bool Next()
	{
		if (++m_currentPos >= m_fileBuffer.Size())
		{
			if (m_fileBuffer.Read() == 0)
				return false;
			m_currentPos = 0;
		}
		return true;
	}

	bool IsValid()
	{
		return m_currentPos < m_fileBuffer.Size();
	}

	T Current()
	{
		return m_fileBuffer[m_currentPos];
	}

	void PushBack(T value)
	{
		m_fileBuffer[m_currentPos++] = value;
		if (m_currentPos >= m_fileBuffer.Size())
		{
			m_fileBuffer.Save();
			m_currentPos = 0;
		}
	}

	void Flush()
	{
		m_fileBuffer.Resize(m_currentPos);
		m_fileBuffer.Save();
	}

private:
	size_t m_currentPos;
	FileBuffer<T>& m_fileBuffer;
};

template<class T>
class FileBuffer
{
	typedef std::vector<T> Buffer;
	typedef std::vector<int32_t>::iterator BufferIterator;

public:
	FileBuffer(uint32_t size, bool verbose = false) : m_verbose(verbose), m_bufferSize(size)
	{
		m_buffer.resize(size);
	}
	
	void Open(const std::string& fileName, const std::string& mode = "rb")
	{
		if (m_file.get())
			m_file.release();

		m_file = std::unique_ptr<FILE, std::function<void(FILE*)>>(std::fopen(fileName.c_str(), mode.c_str()),
			[](FILE* fp) { std::fclose(fp); });

		if (!m_file.get())
			throw std::runtime_error("Can't open file");

		m_fileName = fileName;
	}

	size_t GetFileSize()
	{
		assert(m_file.get());
		std::fseek(m_file.get(), 0, SEEK_END);
		const size_t fileSize = std::ftell(m_file.get()) / sizeof(T);
		std::fseek(m_file.get(), 0, SEEK_SET);
		return fileSize;
	}

	void Save()
	{
		if (Size() == 0)
			return;

		if (!m_file.get())
			throw std::runtime_error("Can't open file");

		if (m_verbose)
		{
			std::cout << std::endl << "Saving to file " << m_fileName << ": " << Size() << " digits" << std::endl;
			std::for_each(Begin(), End(), [](int32_t value) { std::cout << value << " "; });
			std::cout << std::endl << "Number of digits: " << Size() << " Bytes: " << Size() * sizeof(T) << std::endl;
		}

		size_t rc = std::fwrite(&m_buffer[0], sizeof(m_buffer[0]), m_buffer.size(), m_file.get());
		m_buffer.clear();
		m_buffer.resize(m_bufferSize);
		
	}

	size_t Read()
	{
		size_t digitsRead = std::fread(&m_buffer[0], sizeof(m_buffer[0]), m_buffer.size(), m_file.get());
		Resize(digitsRead);

		if (m_verbose)
		{
			std::cout << "Read " << m_fileName.c_str() << ":" << std::endl;
			for_each(m_buffer.begin(), m_buffer.end(), [](T value) { std::cout << value << " "; });
			std::cout << std::endl << "Number of digits: " << Size() << " Bytes: " << digitsRead * sizeof(T) << std::endl;
		}

		return digitsRead;
	}

	bool Seek(const int32_t offset)
	{
		return std::fseek(m_file.get(), offset, SEEK_CUR) == 0;
	}

	BufferIterator Begin()
	{
		return m_buffer.begin();
	}

	BufferIterator End()
	{
		return m_buffer.end();
	}

	Buffer& Get()
	{
		return m_buffer;
	}

	void Resize(uint32_t newSize)
	{
		if (newSize != m_buffer.size())
		{
			m_buffer.resize(newSize);
			m_bufferSize = newSize;
		}
	}
	
	size_t Size()
	{
		return m_bufferSize;
	}

	T& operator[](size_t index)
	{
		return m_buffer[index];
	}

	
private:
	Buffer						  m_buffer;
	std::string					  m_fileName;
	deleted_unique_ptr<std::FILE> m_file;
	bool						  m_verbose;
	size_t						  m_bufferSize;
};


#endif