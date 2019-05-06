#include "fs/path.hpp"

#if defined(EA_PLATFORM_WINDOWS)
#	define WIN32_LEAN_AND_MEAN
#	include <ShlObj.h>
#	include <Windows.h>
#else
#	include <unistd.h>
#	include <sys/stat.h>
#endif

#if defined(EA_PLATFORM_LINUX)
#	include <linux/limits.h>
#endif

#include <EASTL/type_traits.h>
#include <cstring>
#include <sstream>
#include <stdexcept>

fs::path fs::path::make_absolute(eastl::polyalloc::allocator_handle handle) const {
#if !defined(_WIN32)
	char temp[PATH_MAX];
	if (realpath(str(path_type::posix_path, handle).c_str(), temp) == nullptr)
		throw std::runtime_error("Internal error in realpath(): " + std::string(strerror(errno)));
	return path(temp, handle);
#else
	internal::wstring value = wstr(handle), out(MAX_PATH_WINDOWS, '\0', handle);
	DWORD length = GetFullPathNameW(value.c_str(), MAX_PATH_WINDOWS, &out[0], NULL);
	if (length == 0)
		throw std::runtime_error("Internal error in realpath(): " + std::to_string(GetLastError()));
	return path(internal::substr(out, 0, length, handle), handle);
#endif
}

bool fs::path::file_exists(eastl::polyalloc::allocator_handle handle) const {
#if defined(_WIN32)
	return GetFileAttributesW(wstr(handle).c_str()) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat sb {};
	return stat(str(path_type::posix_path, handle).c_str(), &sb) == 0;
#endif
}

size_t fs::path::file_size(eastl::polyalloc::allocator_handle handle) const {
#if defined(_WIN32)
	struct _stati64 sb;
	if (_wstati64(wstr(handle).c_str(), &sb) != 0)
		throw std::runtime_error(("path::file_size(): cannot stat file \"" + str(handle) + "\"!").c_str());
#else
	struct stat sb {};
	if (stat(str(path_type::posix_path, handle).c_str(), &sb) != 0)
		throw std::runtime_error(("path::file_size(): cannot stat file \"" + str(path_type::posix_path, handle) + "\"!").c_str());
#endif
	return (size_t) sb.st_size;
}

bool fs::path::is_directory(eastl::polyalloc::allocator_handle handle) const {
#if defined(_WIN32)
	DWORD result = GetFileAttributesW(wstr(handle).c_str());
	if (result == INVALID_FILE_ATTRIBUTES)
		return false;
	return (result & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat sb {};
	if (stat(str(path_type::posix_path, handle).c_str(), &sb))
		return false;
	return S_ISDIR(sb.st_mode);
#endif
}

bool fs::path::is_file(eastl::polyalloc::allocator_handle handle) const {
#if defined(_WIN32)
	DWORD attr = GetFileAttributesW(wstr(handle).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
#else
	struct stat sb {};
	if (stat(str(path_type::posix_path, handle).c_str(), &sb))
		return false;
	return S_ISREG(sb.st_mode);
#endif
}

fs::internal::string fs::path::extension(eastl::polyalloc::allocator_handle handle) const {
	const internal::string& name = filename(handle);
	size_t pos = name.find_last_of('.');
	if (pos == std::string::npos)
		return "";
	return internal::substr(name, pos + 1, handle);
}

fs::internal::string fs::path::filename(eastl::polyalloc::allocator_handle handle) const {
	if (empty())
		return "";
	const internal::string& last = m_path[m_path.size() - 1];
	return internal::string(last, handle);
}

fs::path fs::path::parent_path(eastl::polyalloc::allocator_handle handle) const {
	path result(handle);
	result.m_absolute = m_absolute;

	if (m_path.empty()) {
		if (!m_absolute)
			result.m_path.emplace_back("..");
	}
	else {
		size_t until = m_path.size() - 1;
		for (size_t i = 0; i < until; ++i)
			result.m_path.push_back(m_path[i]);
	}
	return result;
}

fs::path fs::path::operator/(fs::path const& other) const {
	if (other.m_absolute)
		throw std::runtime_error("path::operator/(): expected a relative path!");
	if (m_type != other.m_type)
		throw std::runtime_error("path::operator/(): expected a path of the same type!");

	path result(*this);

	for (const auto& i : other.m_path) {
		result.m_path.push_back(i);
	}

	return result;
}

fs::internal::string fs::path::str(path_type type, eastl::polyalloc::allocator_handle handle) const {
	internal::string out_str(handle);

	if (m_absolute) {
		if (m_type == path_type::posix_path)
			out_str += '/';
		else {
			size_t length = 0;
			for (const auto& i : m_path) {
				// No special case for the last segment to count the NULL character
				length += i.length() + 1;
			}
			// Windows requires a \\?\ prefix to handle paths longer than MAX_PATH
			// (including their null character). NOTE: relative paths >MAX_PATH are
			// not supported at all in Windows.
			if (length > MAX_PATH_WINDOWS_LEGACY) {
				char header[] = R"(\\?\)";
				out_str.append(header, eastl::extent<decltype(header)>::value);
			}
		}
	}

	for (size_t i = 0; i < m_path.size(); ++i) {
		out_str.append(m_path[i]);
		if (i + 1 < m_path.size()) {
			if (type == path_type::posix_path) {
				out_str += '/';
			}
			else {
				out_str += '\\';
			}
		}
	}

	return out_str;
}

void fs::path::set(internal::string const& str, path_type type, eastl::polyalloc::allocator_handle handle) {
	m_type = type;
	if (type == path_type::windows_path) {
		internal::string tmp(str, handle);

		// Long windows paths (sometimes) begin with the prefix \\?\. It should only
		// be used when the path is >MAX_PATH characters long, so we remove it
		// for convenience and add it back (if necessary) in str()/wstr().
		static const internal::string PREFIX(R"(\\?\)", get_global_allocator());
		if (tmp.length() >= PREFIX.length()
		    && std::mismatch(std::begin(PREFIX), std::end(PREFIX), std::begin(tmp)).first == std::end(PREFIX)) {
			tmp.erase(0, 4);
		}
		m_path = tokenize(tmp, internal::string("/\\", handle), m_path.get_allocator());
		m_absolute = tmp.size() >= 2 && std::isalpha(tmp[0]) && tmp[1] == ':';
	}
	else {
		m_path = tokenize(str, internal::string("/", handle), m_path.get_allocator());
		m_absolute = !str.empty() && str[0] == '/';
	}
}

std::ostream& fs::operator<<(std::ostream& os, fs::path const& path) {
	os << path.str(path::path_type::native_path, path.m_path.get_allocator()).c_str();
	return os;
}

bool fs::create_directory(fs::path const& p, eastl::polyalloc::allocator_handle handle) {
#if defined(_WIN32)
	return CreateDirectoryW(p.wstr(handle).c_str(), NULL) != 0;
#else
	return mkdir(p.str(path::path_type::posix_path, handle).c_str(), S_IRWXU) == 0;
#endif
}

bool fs::create_directories(fs::path const& p, eastl::polyalloc::allocator_handle handle) {
#if defined(_WIN32)
	return SHCreateDirectory(nullptr, p.make_absolute(handle).wstr(handle).c_str()) == ERROR_SUCCESS;
#else
	if (create_directory(path(p.str(path::path_type::posix_path, handle).c_str())))
		return true;

	if (p.empty())
		return false;

	if (errno == ENOENT) {
		if (create_directory(p.parent_path()))
			return mkdir(p.str(path::path_type::posix_path, handle).c_str(), S_IRWXU) == 0;
		else
			return false;
	}
	return false;
#endif
}

bool fs::remove_file(const path& p, eastl::polyalloc::allocator_handle handle) {
#if !defined(_WIN32)
	return std::remove(p.str(path::path_type::posix_path, handle).c_str()) == 0;
#else
	return DeleteFileW(p.wstr(handle).c_str()) != 0;
#endif
}

bool fs::resize_file(const path& p, size_t target_length, eastl::polyalloc::allocator_handle handle) {
#if !defined(_WIN32)
	return ::truncate(p.str(path::path_type::posix_path, handle).c_str(), (off_t) target_length) == 0;
#else
	HANDLE file_handle = CreateFileW(wstr(handle).c_str(), GENERIC_WRITE, 0, nullptr, 0, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file_handle == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER size;
	size.QuadPart = (LONGLONG) target_length;
	if (SetFilePointerEx(file_handle, size, NULL, FILE_BEGIN) == 0) {
		CloseHandle(file_handle);
		return false;
	}
	if (SetEndOfFile(file_handle) == 0) {
		CloseHandle(file_handle);
		return false;
	}
	CloseHandle(file_handle);
	return true;
#endif
}

fs::path fs::cwd(eastl::polyalloc::allocator_handle handle) {
#if !defined(_WIN32)
	char temp[PATH_MAX];
	if (::getcwd(temp, PATH_MAX) == NULL)
		throw std::runtime_error("Internal error in getcwd(): " + std::string(strerror(errno)));
	return path(temp, handle);
#else
	internal::wstring temp(MAX_PATH_WINDOWS, '\0', handle);
	if (!_wgetcwd(&temp[0], MAX_PATH_WINDOWS))
		throw std::runtime_error("Internal error in getcwd(): " + std::to_string(GetLastError()));
	return path(temp, handle);
#endif
}

fs::internal::vector<fs::internal::string> fs::path::tokenize(const fs::internal::string& string,
                                                              const fs::internal::string& delim,
                                                              eastl::polyalloc::allocator_handle handle) {
	internal::string::size_type lastPos = 0, pos = string.find_first_of(delim, lastPos);
	internal::vector<internal::string> tokens(handle);

	while (lastPos != internal::string::npos) {
		if (pos != lastPos) {
			tokens.push_back(internal::substr(string, lastPos, eastl::min(string.size(), pos - lastPos), handle));
		}
		lastPos = pos;
		if (lastPos == std::string::npos || lastPos + 1 == string.length())
			break;
		pos = string.find_first_of(delim, ++lastPos);
	}

	return tokens;
}

#if defined(_WIN32)
std::wstring fs::path::wstr(fs::path::path_type type) const {
	std::string temp = str(type);
	int size = MultiByteToWideChar(CP_UTF8, 0, &temp[0], (int) temp.size(), NULL, 0);
	std::wstring result(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, &temp[0], (int) temp.size(), &result[0], size);
	return result;
}

void fs::path::set(const std::wstring& wstring, fs::path::path_type type) {
	std::string string;
	if (!wstring.empty()) {
		int size = WideCharToMultiByte(CP_UTF8, 0, &wstring[0], (int) wstring.size(), NULL, 0, NULL, NULL);
		string.resize(size, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstring[0], (int) wstring.size(), &string[0], size, NULL, NULL);
	}
	set(string, type);
}
#endif
