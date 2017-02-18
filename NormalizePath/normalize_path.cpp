#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/// Normalizes path removing relative subpaths such as "." and "..".
/// Path delimiter is slash "/".
///
/// Note 1: consecutive delimiters are treated as one (same way as Linux does).
/// Example: "///" -> "/", "///bar////foo//" -> "/bar/foo/"
///
/// Note 2: trailing slash is kept as in input string: if input string has
/// trailing slash output will have too. If input doesn't output will not add it.
/// (Just like in assignment examples).
/// Example: "/bar" -> "/bar", "/bar/" -> "/bar/"
///
/// Note 3: no domain name detection implemented (that's not actually possible
/// with proposed function interface as local domain name cannot be distinguished
/// from folder name). Otherwise it's assumed that if path doesn't start with
/// "/", "./" or "../" than first subfolder is domain-like and it's considered
/// virtual root.
/// Example: "bar/../foo" -> "bar/foo", "/bar/../foo" -> "/foo"
std::string normalize(const std::string& path)
{
    constexpr char delimiter = '/';
    constexpr char rel_point = '.';

    struct folder
    {
        folder(size_t b, size_t s): begin(b), size(s) {}

        size_t begin;
        size_t size;
    };

    // Possible optimization: assume that path length cannot be > PATH_MAX
    // (typically 4096 on most Linux systems). So vector can be replaced with
    // local array of 2049 elements, avoiding memory allocation/deallocation
    // and making data context more local and cache-friendly.
    std::vector<folder> folders;

    // Number of subfolders is no more than path length / 2 + 1.
    folders.reserve(path.size() / 2 + 1);

    size_t root_idx = 0;

    const char* const cpath = path.c_str();
    const size_t size = path.size();
    const char* const cend = cpath + size;

    const char* cur_pos = cpath;
    const char* prev_pos = cpath;

    const auto add_folder =
    [cpath, cend, size, &folders, &root_idx, &cur_pos, &prev_pos](const char* pos)
    {
        const size_t len = pos - prev_pos;

        if (len == 0 ||
            (len == 1 && *(pos - 1) == rel_point) ||
            (len == 2 && *(pos - 1) == rel_point && *(pos - 2) == delimiter))
        {
            // Skip "", ".", "/." as empty subfolders
        }
        else if ((len == 2 && *(pos - 1) == rel_point && *(pos - 2) == rel_point) ||
            (len == 3 && *(pos - 1) == rel_point && *(pos - 2) == rel_point &&
                *(pos - 3) == delimiter))
        {
            // Remove previous subfolder for ".." and "/.." till reaching the root.
            if (folders.size() > root_idx) folders.pop_back();
        }
        else
        {
            // Norm subfolder, add it.
            folders.emplace_back(prev_pos - cpath, pos - prev_pos);

            // Subfolder starts from string beginning but doesn't start with "/".
            // Hmmm, guess it's domain name and new root. Don't harass it by upcoming "..".
            if (prev_pos == cpath && *cpath != delimiter) root_idx = 1;
        }

        cur_pos = pos + 1;

        // Skip adjacent path delimiters ('////' -> '/')
        while (cur_pos < cend && *cur_pos == delimiter) ++cur_pos;

        prev_pos = cur_pos - 1;
    };

    while (auto pos = std::strchr(cur_pos, delimiter))
    {
        add_folder(pos);
    }

    // Process last subfolder separately.
    add_folder(cend);

    std::string result;
    result.reserve(size);

    for (const auto& f : folders)
    {
        result.append(cpath + f.begin, f.size);
    }

    return result;
}

static int tests_failed = 0;

/// Simple unit testing function.
void test(const std::string& input, const std::string& expected)
{
    const auto quote = [](const auto& str)
    {
        return "\'" + str + "\'";
    };

    const auto output = normalize(input);
    const bool ok = output == expected;

    if (!ok) ++tests_failed;

    std::cout
        << (ok ? "OK" : "FAIL (expected " + quote(expected) + ")")
        << " - "
        << quote(input) << " -> " << quote(output)
        << std::endl;
}

std::string generate_path(size_t seed, size_t max_len)
{
    std::string s;

    while (s.size() < max_len)
    {
        if (seed % 2 == 0) s += "/";
        if (seed % 3 == 0) s += "bar";
        if (seed % 4 == 0) s += "foo";
        if (seed % 5 == 0) s += "baz";
        if (seed % 6 == 0) s += "/../";
        if (seed % 7 == 0) s += "/./";
        if (seed % 8 == 0) s += "/./../";
        if (seed % 9 == 0) s += "/.././";
        ++seed;
    }

    return s;
}

void performance_test()
{
    constexpr size_t max_count = 100000;
    constexpr size_t max_path_size = 4096;

    std::cout << "Starting performance test, please wait..." << std::endl;
    std::cout << "Loop count - " << max_count << std::endl;
    std::cout << "Path size - " << max_path_size << std::endl;

    std::vector<std::string> test;
    test.reserve(max_count);

    double total_size = 0;
    for (size_t i = 0; i < max_count; ++i)
    {
        test.push_back(generate_path(time(nullptr), max_path_size));
        total_size += test.back().size();
    }

    // Warm-up pass.
    for (volatile size_t i = 0; i < max_count; ++i)
    {
        volatile auto s = normalize(test[i]);
    }

    // Measure pass.
    using namespace std::chrono;
    const auto start = high_resolution_clock::now();

    for (volatile size_t i = 0; i < max_count; ++i)
    {
        volatile auto s = normalize(test[i]);
    }

    const auto end = high_resolution_clock::now();
    const duration<double, std::micro> total_time_us(end - start);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total duration " << total_time_us.count() << " us" << std::endl;
    std::cout << "Average single call duration " << (total_time_us.count() / max_count) << " us" << std::endl;

    const auto duration_sec = total_time_us.count() / 1E6;
    const auto total_size_mb = total_size / 1E6;

    std::cout << "Average throughput " << (total_size_mb / duration_sec) << " MB/s" << std::endl;
}

int main()
{
    test("../bar", "/bar");
    test("/foo/bar", "/foo/bar");
    test("/foo/bar/../baz", "/foo/baz");
    test("/foo/bar/./baz/", "/foo/bar/baz/");
    test("/foo/../../baz", "/baz");

    test("", "");
    test("/", "/");
    test("///", "/");
    test("/../.", "");
    test("/.././", "/");
    test("./.././bee", "/bee");
    test("/foo/bar", "/foo/bar");
    test("foo/bar/", "foo/bar/");
    test("foo////bar///", "foo/bar/");
    test("../bar/../bor/foo", "/bor/foo");
    test("..", "");
    test(".", "");
    test("../bar", "/bar");
    test("./bar/././", "/bar/");
    test("/bar/foo/bor/../../..", "");
    test("/bar/foo/bor/../../../", "/");
    test("/bar/foo/bor////../../../", "/");
    test("domain.com/../foo", "domain.com/foo");
    test("domain.com/./../foo/../bb/./../../../././skip_me/./../cool/./././", "domain.com/cool/");
    test("domain.com/./../foo/../bb/./../../../././skip_me/./../cool/./././../more_cool", "domain.com/more_cool");
    test("/domain.com/./../foo/../bb/./../../../././skip_me/./../cool/./././../still_cool", "/still_cool");

    test("domain.com/.../foo", "domain.com/.../foo");          // Sorry, garbage in - garbage out
    test(".../domain.com/.../foo", ".../domain.com/.../foo");  // Sorry, garbage in - garbage out

    performance_test();

    return tests_failed;
}
