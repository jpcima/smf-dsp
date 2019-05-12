#include "file_entry.h"
#include "utility/charset.h"

bool operator==(const File_Entry &a, const File_Entry &b)
{
    return a.type == b.type && a.name == b.name;
}

bool operator<(const File_Entry &a, const File_Entry &b)
{
    if (a.type != b.type) {
        if (a.type == 'D')
            return true;
        if (b.type == 'D')
            return false;
    }

    if (a.type == 'D' && a.name == "..")
        return true;
    if (b.type == 'D' && b.name == "..")
        return false;

    return utf8_icompare(a.name, b.name) < 0;
}
