#pragma once
#include <map>
#include <cstring>
#include <cstdlib>
#include <cerrno>

class dsp {
public:
    virtual ~dsp() {}
};

class Meta {
public:
    void declare(const char *key, const char *value) {}
};

class UI {
public:
    // -- widget's layouts

    void openTabBox(const char *label) {}
    void openHorizontalBox(const char *label) {}
    void openVerticalBox(const char *label) {}
    void closeBox() {}

    // -- active widgets

    void addButton(const char *label, float *zone) {}
    void addCheckButton(const char *label, float *zone) {}
    void addVerticalSlider(const char *label, float *zone, float init, float min, float max, float step) {}
    void addHorizontalSlider(const char *label, float *zone, float init, float min, float max, float step) {}
    void addNumEntry(const char *label, float *zone, float init, float min, float max, float step) {}

    // -- passive widgets

    void addHorizontalBargraph(const char *label, float *zone, float min, float max) {}
    void addVerticalBargraph(const char *label, float *zone, float min, float max) {}

    // -- metadata declarations

    void declare(float *zone, const char *key, const char *val)
    {
        const char *endptr = nullptr;

        errno = 0;
        unsigned long id = std::strtoul(key, const_cast<char **>(&endptr), 10);
        id = (errno == 0 && std::strlen(key) == endptr - key) ? id : ~0ul;

        if (id != ~0ul)
            actives[id] = zone;
    }

    // -- extra

    float *getActiveById(unsigned long id)
    {
        return actives.at(id);
    }

private:
    std::map<unsigned long, float *> actives;
};
