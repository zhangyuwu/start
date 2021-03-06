// start.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <string.h>
#include <sstream>
#include <regex>
#include <list>
#include <process.h>

#define SW_CONSOLE 0x1000

using namespace std;

string get_current_directory()
{
	char buffer[4096] = { 0 };
	::GetCurrentDirectoryA(sizeof(buffer) / sizeof(char) - 1, buffer);
	return buffer;
}

bool directory_exist(string path)
{
	auto ret = ::GetFileAttributesA(path.c_str());
	return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_DIRECTORY);
}

bool file_exist(string path)
{
	auto ret = ::GetFileAttributesA(path.c_str());
	return (ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY);
}

template<class InputIterator> string join_args(InputIterator first, InputIterator last, string delimiter = " ")
{
	ostringstream oss;
	while (first != last) {
		oss << *first++;
		if (first != last) {
			oss << delimiter;
		}
	}
	return oss.str();
}

template<class T> void list_to_array(list<T> &list, T* arr, size_t size)
{
	auto index = 0;
	for (auto it = list.begin(); it != list.end() && index < size; it++) {
		arr[index++] = *it;
	}
}

string get_error_message(int error_code)
{
	string msg;
	LPVOID msgbuf;
	int count = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&msgbuf,
		0,
		NULL
	);
	msg = reinterpret_cast<char *>(msgbuf);
	LocalFree(msgbuf);
	return msg;
}

void help()
{
	cout << "Usage: start [--style=STYLE] [--title=TITLE] COMMAND [PARAMETER]" << endl;
	cout << "Options:" << endl;
	cout << "  STYLE        - can be max/min/normal/hide/console, default to normal" << endl;
	cout << "  COMMAND      - the command to run" << endl;
	cout << "  PARAMETER    - parameters to pass to COMMAND" << endl;
}

bool strequ(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

int get_style(list<char *> &list)
{
	std::regex style_pattern("^--style=(normal|max|min|hide|console)$");
	int nShowCmd = SW_NORMAL;

	if (regex_match(*list.begin(), style_pattern)) {
		auto opt = list.front();
		auto sym = strchr(opt, '=') + 1;
		if (strequ(sym, "max")) {
			nShowCmd = SW_MAXIMIZE;
		}
		else if (strequ(sym, "min")) {
			nShowCmd = SW_MINIMIZE;
		}
		else if (strequ(sym, "hide")) {
			nShowCmd = SW_HIDE;
		}
		else if (strequ(sym, "console")) {
			nShowCmd = SW_CONSOLE;
		}
		else {
			nShowCmd = SW_NORMAL;
		}
		list.pop_front();
	}
	return nShowCmd;
}

const char * get_title(list<char *> &list)
{
	std::regex title_pattern("^--title=.+$");
	const char *title = NULL;

	if (regex_match(*list.begin(), title_pattern)) {
		auto opt = list.front();
		title = strchr(opt, '=') + 1;
		list.pop_front();
	}
	return title;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		help();
		return -1;
	}

	list<char *> list(argv + 1, argv + argc);
	auto nShowCmd = get_style(list);
	auto title = get_title(list);

	if (list.size() == 0) {
		help();
		return -2;
	}

	if (nShowCmd != SW_CONSOLE) {
		auto curdir = get_current_directory();
		auto path = regex_replace(list.front(), std::regex("/"), "\\");
		list.pop_front();

		auto param = join_args(list.begin(), list.end());
		auto op = directory_exist(path) ? "explore" : "open";

		auto ret = ::ShellExecuteA(NULL, op, path.c_str(), param.c_str(), curdir.c_str(), nShowCmd);
		auto err = ::GetLastError();
		if ((INT_PTR)ret < 32) {
			cerr << "Error: failed to run command: " << argv[1] << endl;
			cerr << get_error_message(err);
		}
		return err;
	}
	else {
		if (title) {
			::SetConsoleTitleA(title);
		}

		auto cmd = join_args(list.begin(), list.end());
		return system(cmd.c_str());
	}
}

