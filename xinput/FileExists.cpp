#include <string>
#include <wtypes.h>

bool FileExists(const std::string& path)
{
	DWORD dwAttrib = 0;
#ifdef UNICODE
	std::wstring wpath(path.begin(), path.end());
	dwAttrib = GetFileAttributes(wpath.c_str());
#else
	dwAttrib = GetFileAttributes(path.c_str());	
#endif

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}