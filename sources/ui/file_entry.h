#pragma once
#include <string>

struct File_Entry { char type; std::string name; };

bool operator==(const File_Entry &a, const File_Entry &b);
bool operator<(const File_Entry &a, const File_Entry &b);
