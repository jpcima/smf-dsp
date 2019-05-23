#pragma once
#include <memory>

struct string_list_delete { void operator()(char **p) const; };
typedef std::unique_ptr<char *[], string_list_delete> string_list_ptr;

string_list_ptr string_list_dup(const char *const *list);
