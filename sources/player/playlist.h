//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <vector>
#include <deque>
#include <random>
#include <memory>
#include <functional>
class File_Scan;

class Play_List {
public:
    virtual ~Play_List() {}
    virtual void start() = 0;
    virtual bool at_end() const = 0;
    virtual std::string current() const = 0;
    virtual bool go_next() = 0;
    virtual bool go_previous() = 0;
};

enum Repeat_Mode : unsigned {
    Repeat_Multi = 0,
    Repeat_Single = 1 << 0,
    Repeat_On = 0,
    Repeat_Off = 1 << 1,
    //
    Repeat_Mode_Max = Repeat_On|Repeat_Off|Repeat_Multi|Repeat_Single
};

class Linear_Play_List : public Play_List {
public:
    void add_file(const std::string &path);
    void start() override;
    bool at_end() const override;
    std::string current() const override;
    bool go_next() override;
    bool go_previous() override;

private:
    std::vector<std::string> files_;
    size_t index_ = 0;
};

class Random_Play_List : public Play_List {
public:
    explicit Random_Play_List(
        const std::string &root_path,
        std::function<bool (const std::string &)> file_filter);
    ~Random_Play_List();
    void start() override;
    bool at_end() const override;
    std::string current() const override;
    bool go_next() override;
    bool go_previous() override;

private:
    size_t random_file() const;

private:
    std::unique_ptr<File_Scan> file_scan_;
    mutable std::mt19937 prng_;
    size_t index_ = 0;
    static constexpr size_t history_max = 10;
    std::deque<size_t> history_;
};
