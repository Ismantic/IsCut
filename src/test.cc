#include <iostream>
#include <string>
#include <vector>

#include "cut.h"

std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string r;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) r += sep;
        r += v[i];
    }
    return r;
}

int passed = 0;
int failed = 0;

void check(bool cond, const std::string& name) {
    if (cond) {
        passed++;
    } else {
        failed++;
        std::cout << "FAIL: " << name << std::endl;
    }
}

void test_basic_cut() {
    cut::Cutter cutter;
    cutter.Build({"ab", "cd", "abcd", "ef"}, {5, 5, 100, 5});

    auto rs = cutter.Cut("abcdef");
    std::string result = join(rs, "/");
    std::cout << "basic: " << result << std::endl;
    check(result == "abcd/ef", "basic cut");
}

void test_chinese_cut() {
    cut::Cutter cutter;
    cutter.Build(
        {"\xe5\x8d\x97\xe4\xba\xac",
         "\xe5\x8d\x97\xe4\xba\xac\xe5\xb8\x82",
         "\xe5\xb8\x82\xe9\x95\xbf",
         "\xe9\x95\xbf\xe6\xb1\x9f",
         "\xe5\xa4\xa7\xe6\xa1\xa5",
         "\xe6\xb1\x9f\xe5\xa4\xa7\xe6\xa1\xa5",
         "\xe9\x95\xbf\xe6\xb1\x9f\xe5\xa4\xa7\xe6\xa1\xa5"},
        {100, 80, 50, 90, 60, 30, 120});

    auto rs = cutter.Cut(
        "\xe5\x8d\x97\xe4\xba\xac\xe5\xb8\x82"
        "\xe9\x95\xbf\xe6\xb1\x9f\xe5\xa4\xa7\xe6\xa1\xa5");
    std::string result = join(rs, "/");
    std::cout << "chinese: " << result << std::endl;
    check(rs.size() >= 2 && rs.size() <= 4, "chinese cut");
}

void test_no_match() {
    cut::Cutter cutter;
    cutter.Build({"ab", "cd"}, {10, 10});

    auto rs = cutter.Cut("xyz");
    std::cout << "no match: " << join(rs, "/") << std::endl;
    check(rs.size() == 3, "no match");
}

void test_han_only_cut() {
    cut::Cutter cutter;
    cutter.Build(
        {"他", "是", "英国", "英国人", "人"},
        {50, 40, 80, 100, 30});

    auto rs = cutter.Cut("他是英国人Tom", true);
    std::string result = join(rs, "/");
    std::cout << "han_only: " << result << std::endl;
    check(result == "他/是/英国人/Tom", "han-only cut");
}

int main() {
    test_basic_cut();
    test_chinese_cut();
    test_no_match();
    test_han_only_cut();
    std::cout << "\nPassed: " << passed << ", Failed: " << failed << std::endl;
    return failed > 0 ? 1 : 0;
}
