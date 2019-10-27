#include <string>
#include <wtypes.h>

bool FileExists(const std::string& path)
{
#ifdef UNICODE
	std::wstring wpath(path.begin(), path.end());
	DWORD dwAttrib = GetFileAttributes(wpath.c_str());
#else
	DWORD dwAttrib = GetFileAttributes(path.c_str());
#endif

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
	        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
