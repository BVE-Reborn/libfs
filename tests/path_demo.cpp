#include "fs/path.hpp"
#include "fs/resolver.hpp"
#include <iostream>

using namespace std;
using namespace fs;

int main(int argc, char** argv) {
#if !defined(EA_PLATFORM_WINDOWS)
	path path1("/dir 1/dir 2/");
#else
	path path1("C:\\dir 1\\dir 2\\");
#endif
	path path2("dir 3");

	cout << path1.exists() << endl;
	cout << path1 << endl;
	cout << (path1 / path2) << endl;
	cout << (path1 / path2).parent_path() << endl;
	cout << (path1 / path2).parent_path().parent_path() << endl;
	cout << (path1 / path2).parent_path().parent_path().parent_path() << endl;
	cout << (path1 / path2).parent_path().parent_path().parent_path().parent_path() << endl;
	cout << path().parent_path() << endl;
	cout << "some/path.ext:operator==() = " << (path("some/path.ext") == path("some/path.ext")) << endl;
	cout << "some/path.ext:operator==() (unequal) = " << (path("some/path.ext") == path("another/path.ext")) << endl;

	cout << "nonexistant:exists = " << path("nonexistant").exists() << endl;
	cout << "nonexistant:is_file = " << path("nonexistant").is_file() << endl;
	cout << "nonexistant:is_directory = " << path("nonexistant").is_directory() << endl;
	cout << "nonexistant:filename = " << path("nonexistant").filename() << endl;
	cout << "nonexistant:extension = " << path("nonexistant").extension() << endl;
	//	cout << "include/fs/path.hpp:exists = " << path("include/fs/path.hpp").exists() << endl;
	//	cout << "include/fs/path.hpp:is_file = " << path("include/fs/path.hpp").is_file() << endl;
	//	cout << "include/fs/path.hpp:is_directory = " << path("include/fs/path.hpp").is_directory() << endl;
	//	cout << "include/fs/path.hpp:filename = " << path("include/fs/path.hpp").filename() << endl;
	//	cout << "include/fs/path.hpp:extension = " << path("include/fs/path.hpp").extension() << endl;
	//	cout << "include/fs/path.hpp:make_absolute = " << path("include/fs/path.hpp").make_absolute() << endl;
	cout << "../include/fs:exists = " << path("../include/fs").exists() << endl;
	cout << "../include/fs:is_file = " << path("../include/fs").is_file() << endl;
	cout << "../include/fs:is_directory = " << path("../include/fs").is_directory() << endl;
	cout << "../include/fs:extension = " << path("../include/fs").extension() << endl;
	cout << "../include/fs:filename = " << path("../include/fs").filename() << endl;
	cout << "../include/fs:make_absolute = " << path("../include/fs").make_absolute() << endl;

	cout << "resolve(include/fs/path.hpp) = " << resolver().resolve(fs::path("include/fs/path.hpp")) << endl;
	cout << "resolve(nonexistant) = " << resolver().resolve(fs::path("nonexistant")) << endl;
	return 0;
}