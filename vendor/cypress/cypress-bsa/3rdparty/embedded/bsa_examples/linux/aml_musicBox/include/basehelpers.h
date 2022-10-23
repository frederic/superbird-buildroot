/****************************************************************************
 *
 *		Helper Classes
 *		--------------
 *
 ****************************************************************************
 *
 *	Description:	Helper classes
 *
 *	Copyright:		Copyright DEMA Consulting 2008-2009, All rights reserved.
 *	Owner:			DEMA Consulting.
 *					93 Richardson Road
 *					Dublin, NH 03444
 *
 ***************************************************************************/

/**
 * @addtogroup Helpers
 * @{
 */

/**
 * @file
 * @brief Helper classes
 */

#ifndef BASEHELPERS_H_INCLUDED
#define BASEHELPERS_H_INCLUDED



#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "paths.h"

using namespace std;



/** Device operation succeeded. */
#define ERR_Success						(0)							// 0

/** Caller must wait for later notification. */
#define ERR_WaitNotify					(-1)						// -1

/** Command threw an exception. */
#define ERR_Exception					(-2)						// -2

/** The action is not supported. */
#define ERR_NotSupported				(-3)						// -3

/** The named object was not found in the registry. */
#define ERR_NoSuchObject				(-4)						// -4

/** There is no registry. */
#define ERR_NoRegistry					(-5)						// -5

/** Command was called with the wrong number of arguments. */
#define ERR_CMD_WrongNumberOfArgs		(-6)						// -6

/** Error getting argv[2] as array. */
#define ERR_CMD_CantGetArray			(-7)						// -7

/** Error getting length of array argv[2]. */
#define ERR_CMD_CantGetArrayLength		(-8)						// -8

/** Error getting element of argv[2]. */
#define ERR_CMD_CantGetArrayElement		(-9)						// -9

/** Command argument count. */
#define ERR_ArgumentCount				(-10)						// -10

/** Command argument error. */
#define ERR_ArgumentError				(-11)						// -11

/** There was an issue with the JavaScript. */
#define ERR_JscriptError				(-12)						// -12

/** Unable to write a file. */
#define ERR_FileWriteError				(-13)						// -13

/** Can't find specified file. */
#define ERR_NoSuchFile					(-14)						// -14

/** Error reading file. */
#define ERR_FileReadError				(-15)						// -15

/** Format error in read data. */
#define ERR_BadData						(-16)						// -16

/** Syntax error in CSV string. */
#define ERR_CSVSyntax					(-17)						// -17

/** Error creating object. */
#define ERR_CantCreate					(-18)						// -18


/**
 * @brief Map of string to string.
 */
class string_map : public map<string, string>
{
public:
	/**
	 * @brief Populate a string_map.
	 *
	 * This function populates the string-map from a string of the form:
	 * name=value, name2 = value2, name3 = value3
	 *
	 * @param [in] src				Source string to parse.
	 * @param [in] quote			Quote character.
	 * @return						Number of items processed.
	 */
	size_t Split(const char *src, char quote = 0);
};


/**
 * @brief Safe version of strncpy.
 * @param [out] dest				destination of copy
 * @param [in] source				source of copy
 * @param [in] count				max number of bytes including NULL to copy
 * @return							dest
 *
 * This function guarantees that dest is NULL terminated by truncating source
 * if needed. It does not zero-fill unused parts of dest.
 */
char *strNcpy(char *dest, const char *source, size_t count);

/**
 * @brief Template function to perform deep-comparison of vectors.
 *
 * This function compares two vectors for equality by performing
 * a member-wise comparison of all items in the vector.
 *
 * @param [in] v1		First vector to compare.
 * @param [in] v2		Second vector to compare.
 * @return				true if the vectors are equal.
 */
template <typename T>
inline bool compare_vectors(const vector<T> &v1, const vector<T> &v2)
{
	/* Compare the count. */
	if (v1.size() != v2.size())
	{
		return false;
	}

	/* Compare each member. */
	for (size_t i = 0; i < v1.size(); ++i)
	{
		if (v1[i] != v2[i])
		{
			return false;
		}
	}

	/* Vectors are identical. */
	return true;
}

/** This is a wrapper for stdio FILE with additional functionality. */
class File2
{
	/** Copy disabled by private assignment. */
	File2 &operator = (const File2& );

protected:
	/** The stream we wrap. */
	FILE *_stream;

	/** When true, close the stream when the object is destroyed. Only false
	if attached to pre-existing stream. */
	bool _closeStream;

public:
	/** Default constructor. */
	File2();

	/**
	 * @brief Construct using an existing stream.
	 * @param [in] fh				the stream to wrap
	 */
	File2(FILE *fh);

	/**
	 * @brief Construct using a filename and open specification.
	 * @param [in] filename			the file to open
	 * @param [in] mode				how to open it - see fopen
	 * @param [in] bufLen			optional buffer size for I/O buffer
	 */
	File2(const char *filename, const char *mode = "rb", int bufLen = -1);

	/**
	 * @brief File destructor.
	 */
	virtual ~File2();

	/**
	 * @brief Test if the object is valid (has q valid stream).
	 * @return						true of valid
	 */
	inline bool valid() const
	{
		return (_stream) ? true : false;
	}

	/**
	 * @brief Open as though by fopen
	 * @param [in] filename			the file to open
	 * @param [in] mode				how to open it - see fopen
	 * @return						true on success
	 */
	bool open(const char *filename, const char *mode = "rb");

	/** Close the current stream. */
	void close();

	// Positioning.
	/**
	 * @brief Report the current seek position
	 * @return						position, or -1 for error - see errno
	 */
	int tell();

	/**
	 * @brief Seek the current file pointer
	 * @param [in] offs				the offset
	 * @param [in] org				one of SEEK_CUR, SEEK_END, SEEK_SET
	 * @return						0 on success
	 */
	int seek(int offs, int org);

	/**
	 * @brief Set an I/O buffer to use.
	 * @param [in] buf				the buffer to use
	 * @param [in] bufSize			its size
	 * @return						false on success
	 */
	bool setBuffer(void *buf, size_t bufSize);

	/**
	 * @brief Flush the I/O buffer
	 * @return						false on success
	 */
	bool flush();

	/**
	 * @brief Return the length of	a file
	 * @return						the file's length
	 *
	 * This is intended for use on read streams.
	 */
	long getLength();

	// Reading / Writing.
	/**
	 * @brief Read a single character
	 * @return						the character read, or EOF
	 */
	inline int readChar()
	{
		return fgetc(_stream);
	}

	/**
	 * @brief Write a single character
	 * @param [in] ch				the character to write
	 * @return						the character written, or EOF on error
	 */
	int writeChar(int ch);

	/**
	 * @brief Read data
	 * @param [out] p				the data buffer to read into
	 * @param [in] len				its size
	 * @return						number of characters read
	 */
	size_t read(void *p, size_t len);

	/**
	 * @brief Write data
	 * @param [in] p				the data to write
	 * @param [in] len				its size
	 * @return						number of characters written
	 */
	inline size_t write(const void *p, size_t len)
	{
		return fwrite(p, 1, len, _stream);
	}

	// Printing.
	/**
	 * @brief Write a string
	 * @param [in] buf				the string to write
	 * @return						number of characters written
	 */
	inline size_t puts(const char *buf)
	{
		return write((void *)buf, strlen(buf));
	}

	/**
	 * @brief Write formatted output
	 * @param [in] fmt				the format string
	 * @return						number of characters written
	 */
	size_t printf(const char *fmt, ...);

	/**
	 * @brief Read a text line
	 * @param [out] out				the line read
	 * @param [in] strip			when true, remove terminating line ends
	 * @param [in] unix				when true, line ends are LF as well as CRLF
	 * @return						true on success
	 *
	 * This function expands on the traditional fgets() by supporting stripping
	 * line ends, and supporting both Windows and Unix line ends.
	 */
	bool fgets(std::string &out,
			   bool strip = false,
			   bool unixf = false);
};

/** This class is a wrapper for vector<string> to allow reading and
writing of files. */
class string_vector: public vector<string>
{
public:
	/**
	 * @brief Append a string to the vector
	 * @param [in] s				the string to append
	 */
	inline void append(const char *s)
	{
		if (s == 0)
		{
			s = "";
		}
		push_back(s);
	}

	/**
	 * @brief Append a string to the vector
	 * @param [in] s				the string to append
	 */
	inline void append(const string &s)
	{
		push_back(s);
	}

	/**
	 * @brief Append an argc/argv to the vector.
	 * @param [in] argc				Number of strings.
	 * @param [in] argv				Array of strings.
	 */
	inline void append(size_t argc, const char **argv)
	{
		/* Append all items. */
		for (size_t i = 0; i < argc; ++i)
		{
			append(argv[i]);
		}
	}

	/**
	 * @brief Make an argc/argv array.
	 *
	 * The caller is responsible for deleting the array once finished. The
	 * array will be corrupted if the corresponding string_vector object is
	 * modified.
	 *
	 * @param [out] count			Number of strings.
	 * @return						Array of string pointers.
	 */
	inline const char **MakeArgv(size_t &count)
	{
		/* Get the number of elements. */
		count = size();

		/* Allocate an array of string. */
		const char** ret = new const char*[count];
		for (size_t i = 0; i < count; ++i)
		{
			ret[i] = at(i).c_str();
		}

		/* Return the allocated string. Caller deletes. */
		return ret;
	}

	/**
	 * @brief Populate this string_vector by splitting the supplied string.
	 * @param [in] src				Source string to split.
	 * @param [in] delim			Delimiter character(s).
	 * @param [in] quote			Quote character.
	 */
	void Split(const char *src, const char *delim, char quote = 0);

	/**
	 * @brief read a text file into the vector
	 * @param [in] filename			the file to read
	 * @return						the number of lines read, -1 if error
	 *
	 * The vector is emptied before reading the file. The strings
	 * in the vector have line ends stripped. Both Windows and Unix
	 * line ends are handled.
	 */
	int LoadFromFile(const char *filename);

	/**
	 * @brief write a text file from the vector
	 * @param [in] filename			the file to read
	 * @return						true if succesful
	 *
	 * Line ends of CRLF are added to every line written, so the lines
	 * in the vector are expected to be unterminated.
	 */
	bool SaveToFile(const char *filename);

#if 0
	/**
	 * @brief Read a text file into the vector
	 * @param [in] filename			the file to read
	 * @param [in] fileStore		the file store containing the file
	 * @return						the number of lines read, -1 if error
	 *
	 * The vector is emptied before reading the file. The strings
	 * in the vector have line ends stripped. Both Windows and Unix
	 * line ends are handled.
	 */
	int LoadFromFileStoreFile(const char *filename, IFileStore *fileStore);

	/**
	 * @brief Read an IFile into the vector from the current position
	 * @param [in] fileRef			file to read from
	 * @return						the number of lines read, -1 if error
	 */
	int LoadFromIFile(IFileRef fileRef);

	/**
	 * @brief Write a text file from the vector
	 * @param [in] filename			the file to read
	 * @param [in] fileStore		the file store containing the file
	 * @return						true if succesful
	 *
	 * Line ends of CRLF are added to every line written, so the lines
	 * in the vector are expected to be unterminated.
	 */
	bool SaveToFileStoreFile(const char *filename, IFileStore *fileStore);

	/**
	 * @brief Write an IFile file from the vector at the current position
	 * @param [in] fileRef			file to write to
	 * @return						true if succesful
	 */
	bool SaveToIFile(IFileRef fileRef);
#endif

	/**
	 * @brief read a string into the vector
	 * @param [in] s				the string to read
	 * @return						the number of lines read, -1 if error
	 *
	 * The vector is emptied before reading the string. The strings
	 * in the vector have line ends stripped. Both Windows and Unix
	 * line ends are handled.
	 */
	int LoadFromString(const char *s);

	/**
	 * @brief write a string from the vector
	 * @param [in] s				the string to fill
	 * @param [in] newLine			use new lines if set
	 * @return						true if succesful
	 *
	 * Line ends of CRLF are added to every line written, so the lines
	 * in the vector are expected to be unterminated.
	 */
	bool SaveToString(string &s, bool newLine = true) const;

	/**
	 * @brief Comparison operator.
	 * @param [in] src				string_vector to compare against.
	 * @return						true if the vectors are equal.
	 */
	inline bool operator==(const string_vector &src) const
	{
		/* Perform a deep comparison of the vectors. */
		return compare_vectors(*this, src);
	}

	/**
	 * @brief sort the vector
	 * @param [in] st				optional start item
	 */
	void sort(size_t st = 0);

	/**
	 * @brief sort the vector reversed
	 * @param [in] st				optional start item
	 */
	void sortrev(size_t st = 0);

	/**
	 * @brief Sort the vector ignoring case
	 * @param [in] st				optional start item
	 */
	void sorti(size_t st = 0);

	/**
	 * @brief Sort the vector reversed ignoring case
	 * @param [in] st				optional start item
	 */
	void sortirev(size_t st = 0);

	/**
	 * @brief Test if a string is in the vector
	 * @param [in] s				string to test
	 * @param [in] st				optional start index
	 * @return						item index or -1
	 */
	int find(const char *s, size_t st = 0) const;

	/**
	 * @brief Test case insensitive if a string is in the vector
	 * @param [in] s				string to test
	 * @param [in] st				optional start index
	 * @return						item index or -1
	 */
	int findi(const char *s, size_t st = 0) const;

	/**
	 * @brief Test with wildcard if a string is in the vector
	 * @param [in] s				wildcard pattern to test
	 * @param [in] st				optional start index
	 * @return						item index or -1
	 */
	int findwild(const char *s, size_t st = 0) const;

	/**
	 * @brief Populate the vector from a CSV string
	 * @param [in] str				the CSV string to parse
	 * @param [in] stripWhite		when set remove leading and trailing white space from field value
	 * @param [in] delim			delimiter character to use
	 * @param [in] quote			quote character to use
	 * @return						true if the parse succeeded
	 */
	bool loadFromCSVString(const char *str, bool stripWhite = false, char delim = ',', char quote = '\"');

	/**
	 * @brief Save the vector as a CSV string
	 * @param [in] from				index to start saving from
	 * @param [in] forceQuote		when set, quote the field anyway
	 * @param [in] delim			delimiter character to use
	 * @param [in] quote			quote character to use
	 * @return						the CSV string
	 */
	std::string saveToCSVString(size_t from = 0, bool forceQuote = false, char delim = ',', char quote = '\"');

	/**
	 * @brief Get a string from a vector treated as an INI file
	 * @param [in] section			INI file section
	 * @param [in] key				section key
	 * @param [in] def				default to use if key not present
	 */
	std::string GetProfileStr(const char *section, const char *key, const char *def = 0) const;

	/**
	 * @brief Get an integer from a vector treated as an INI file
	 * @param [in] section			INI file section
	 * @param [in] key				section key
	 * @param [in] def				default to use if key not present
	 */
	int GetProfileIntg(const char *section, const char *key, int def = 0) const;

	/**
	 * @brief Write a string to a vector treated as an INI file
	 * @param [in] section			INI file section
	 * @param [in] key				section key
	 * @param [in] val				value to write
	 */
	void WriteProfileStr(const char *section, const char *key, const char *val);

	/**
	 * @brief Write an integer to a vector treated as an INI file
	 * @param [in] section			INI file section
	 * @param [in] key				section key
	 * @param [in] val				value to write
	 */
	void WriteProfileIntg(const char *section, const char *key, int val);
};

/** Template class for generalized vector of pair<string, T *>. */
template <class T> class Dual
{
	/** True if this container owns the T storage. */
	bool m_owns_storage;

	/** The vector. */
	vector<pair<string, T *> > m_vector;

	/**
	 * @brief Compare item strings for sort
	 * @param [in] s1				first item
	 * @param [in] s2				second item
	 * @return						true if s1 < s2
	 */
	static bool compare_dual_strings(const pair<string, T *>& s1, const pair<string, T *>& s2)
	{
		return s1.first < s2.first;
	}

	/**
	 * @brief Compare item strings case insensitive for sort
	 * @param [in] s1				first item
	 * @param [in] s2				second item
	 * @return						true if s1 < s2
	 */
	static bool compare_duali_strings(const pair<string, T *>& s1, const pair<string, T *>& s2)
	{
#ifdef WIN32
		return _stricmp(s1.first.c_str(), s2.first.c_str()) < 0;
#else
		return strcasecmp(s1.first.c_str(), s2.first.c_str()) < 0;
#endif
	}

public:
	/**
	 * @brief Constructor.
	 * @param [in] ownsStorage		set to own storage, default true
	 */
	Dual(bool ownsStorage = true)
		: m_owns_storage(ownsStorage)
	{
	}

	void SetOwnsStorage(bool owns)
	{
		m_owns_storage = owns;
	}

	/** Destructor. */
	~Dual()
	{
		clear();
	}

	/** Clear the vector. */
	inline void clear()
	{
		if (m_owns_storage)
		{
			for (size_t i = 0; i < m_vector.size(); i++)
			{
				delete m_vector[i].second;
			}
		}
		m_vector.clear();
	}

	/**
	 * @brief Get the size
	 * @return						the size
	 */
	inline size_t size() const
	{
		return m_vector.size();
	}

	/**
	 * @brief append a value to the vector
	 * @param [in] s				the string to append
	 * @param [in] p				the object to append
	 */
	inline void append(const char *s, const T *p = 0)
	{
		if (s == 0)
		{
			s = "";
		}
		m_vector.push_back(pair<string, T *>(s, const_cast<T *>(p)));
	}

	/**
	 * @brief append a value to the vector
	 * @param [in] s				the string to append
	 * @param [in] p				the object to append
	 */
	inline void append(const string &s, const T *p = 0)
	{
		m_vector.push_back(pair<string, T *>(s, const_cast<T *>(p)));
	}

	/**
	 * @brief insert a value at the index
	 * @param [in] s				the string to insert
	 * @param [in] idx				where to insert
	 * @param [in] p				optional value
	 * @return						where inserted
	 */
	inline size_t insert(const char *s, size_t idx, const T *p = 0)
	{
		if (s == 0)
		{
			s = "";
		}
		if (idx == 0 || idx < m_vector.size())
		{
			m_vector.insert(m_vector.begin() + idx, pair<string, T *>(s, const_cast<T *>(p)));
		}
		else
		{
			append(s, p);
		}
		return idx;
	}

	/**
	 * @brief insert a value at the index
	 * @param [in] s				the string to insert
	 * @param [in] idx				where to insert
	 * @param [in] p				optional value
	 * @return						where inserted
	 */
	inline size_t insert(const string &s, size_t idx, const T *p = 0)
	{
		if (idx == 0 || idx < m_vector.size())
		{
			m_vector.insert(m_vector.begin() + idx, pair<string, T *>(s, const_cast<T *>(p)));
		}
		else
		{
			append(s, p);
		}
		return idx;
	}

	/**
	 * @brief Replace a value
	 * @param [in] s				the string to replace
	 * @param [in] idx				where to replace
	 * @param [in] p				optional value
	 * @return						true if replaced
	 */
	inline bool replace(const char *s, size_t idx, const T *p = 0)
	{
		if (s == 0)
		{
			s = "";
		}
		if (idx < m_vector.size())
		{
			m_vector[idx].first = s;
			if (m_owns_storage)
			{
				delete m_vector[idx].second;
			}
			m_vector[idx].second = const_cast<T *>(p);
			return true;
		}
		return false;
	}

	/**
	 * @brief Replace a value
	 * @param [in] s				the string to replace
	 * @param [in] idx				where to replace
	 * @param [in] p				optional value
	 * @return						true if replaced
	 */
	inline bool replace(const string &s, size_t idx, const T *p = 0)
	{
		if (idx < m_vector.size())
		{
			m_vector[idx].first = s;
			if (m_owns_storage)
			{
				delete m_vector[idx].second;
			}
			m_vector[idx].second = p;
			return true;
		}
		return false;
	}

	/**
	 * @brief Remove a value
	 * @param [in] idx				where to remove
	 * @return						true if removed
	 */
	inline bool remove(size_t idx)
	{
		if (idx < m_vector.size())
		{
			if (m_owns_storage)
			{
				delete m_vector[idx].second;
			}
			m_vector.erase(m_vector.begin() + idx);
			return true;
		}
		return false;
	}

	/**
	 * @brief Detach a value
	 * @param [in] idx				where to detach
	 * @return						detached item, or 0 on failure
	 */
	inline T *detach(size_t idx)
	{
		if (idx < m_vector.size())
		{
			T *ptr = GetValue(idx);
			m_vector.erase(m_vector.begin() + idx);
			return ptr;
		}
		return 0;
	}

	/**
	 * @brief Get a string value
	 * @param [in] idx				where to get
	 * @return						string at that index
	 */
	inline const string &operator[](size_t idx) const
	{
		return m_vector[idx].first;
	}

	/**
	 * @brief Get a value
	 * @param [in] idx				where to get
	 * @return						value at that index
	 */
	inline T *GetValue(size_t idx)
	{
		return m_vector[idx].second;
	}

	/**
	 * @brief Get a const value
	 * @param [in] idx				where to get
	 * @return						value at that index
	 */
	inline const T *GetValue(size_t idx) const
	{
		return m_vector[idx].second;
	}

	/**
	 * @brief Get the string value
	 * @param [in] idx				where to get
	 * @return						value at that index
	 */
	inline const char *GetStr(size_t idx) const
	{
		return m_vector[idx].first.c_str();
	}

	/**
	 * @brief Set a string value
	 * @param [in] indx				where to set
	 * @param [in] str				string to set
	 */
	inline void SetStr(size_t idx, const char *str)
	{
		m_vector[idx].first = str;
	}

	/**
	 * @brief Set a string value
	 * @param [in] indx				where to set
	 * @param [in] str				string to set
	 */
	inline void SetStr(size_t idx, const std::string &str)
	{
		m_vector[idx].first = str;
	}

	/**
	 * @brief Sort the vector
	 * @param [in] st				optional start item
	 */
	inline void sort(size_t st = 0)
	{
		if (st == 0 || st < size() - 1)
		{
			stable_sort(m_vector.begin() + st, m_vector.end(), compare_dual_strings);
		}
	}

	/**
	 * @brief Sort the vector ignoring case
	 * @param [in] st				optional start item
	 */
	inline void sorti(size_t st = 0)
	{
		if (st == 0 || st < size() - 1)
		{
			stable_sort(m_vector.begin() + st, m_vector.end(), compare_duali_strings);
		}
	}

	/**
	 * @brief Test if a string is in the vector
	 * @param [in] s				string to test
	 * @param [in] st				optional start item
	 * @return						item index or -1
	 */
	inline int find(const char *s, size_t st = 0) const
	{
		for (size_t i = st; i < size(); i++)
		{
			if (m_vector[i].first == s)
			{
				return (int)i;
			}
		}
		return -1;
	}

	/**
	 * @brief Test case insensitive if a string is in the vector
	 * @param [in] s				string to test
	 * @param [in] st				optional start item
	 * @return						item index or -1
	 */
	inline int findi(const char *s, size_t st = 0) const
	{
		for (size_t i = st; i < size(); i++)
		{
#ifdef WIN32
			if (_stricmp(m_vector[i].first.c_str(), s) == 0)
#else
			if (strcasecmp(m_vector[i].first.c_str(), s) == 0)
#endif
			{
				return (int)i;
			}
		}
		return -1;
	}
};



#endif	// !BASEHELPERS_H_INCLUDED

/**
 * @}
 * End of file
 */
