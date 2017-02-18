#include <cstdlib>
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
        folder(size_t b, size_t e): begin(b), end(e) {}

        size_t begin;
        size_t end;
    };

    // Possible optimization: assume that path length cannot be > PATH_MAX
    // (typically 4096 on most Linux systems). So vector can be replaced with
    // local array of 2049 elements, avoiding memory allocation/deallocation
    // and making data context more local and cache-friendly.
    std::vector<folder> folders;

    // Number of subfolders is no more than path length / 2 + 1.
    folders.reserve(path.size() / 2 + 1);

    size_t root_pos = 0;
    size_t cur_pos = 0;
    size_t prev_pos = 0;

    const auto add_folder = 
    [&path, &folders, &root_pos, &cur_pos, &prev_pos](size_t pos)
    {
        const size_t len = pos - prev_pos;
        if (len == 0 || 
            (len == 1 && (path[pos - 1] == rel_point)) ||
            (len == 2 && path[pos - 1] == rel_point && path[pos - 2] == delimiter))
        {
            // Skip "", ".", "/." as empty subfolders
        }
        else if ((len == 2 && path[pos - 1] == rel_point && path[pos - 2] == rel_point) ||
            (len == 3 && path[pos - 1] == rel_point &&
                path[pos - 2] == rel_point && path[pos - 3] == delimiter))
        {
            // Remove previous subfolder for ".." and "/.." till reaching the root.
            if (folders.size() > root_pos) folders.pop_back();
        }
        else
        {
            // Norm subfolder, add it.
            folders.emplace_back(prev_pos, pos);
            
            // Subfolder starts from string beginning but doesn't start with "/".
            // Hmmm, guess it's domain name and new root. Don't harass it by upcoming "..".
            if (prev_pos == 0 && path[0] != delimiter) root_pos = 1;
        }

        cur_pos = pos + 1;

        // Skip adjacent path delimiters ('////' -> '/')
        while (cur_pos < path.size() && path[cur_pos] == delimiter) ++cur_pos;

        prev_pos = cur_pos - 1;
    };

    size_t delim_pos = 0;
    while ((delim_pos = path.find(delimiter, cur_pos)) != std::string::npos)
    {
        add_folder(delim_pos);
    }

    add_folder(path.size());

    std::string result;
    result.reserve(path.size());

    for (const auto& f : folders)
    {
        result.append(path.begin() + f.begin, path.begin() + f.end);
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

int main()
{
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

    return tests_failed;
}
