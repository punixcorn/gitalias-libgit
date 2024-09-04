/*
        project: gitalias-libgit
        author: punixcorn
*/
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <git2.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/object.h>
#include <git2/repository.h>
#include <git2/types.h>

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
namespace std {
using fmt::format, fmt::print;
}

inline void gitErr() {
    throw std::runtime_error(
        std::format("{} {}", __LINE__, git_error_last()->message));
}
// checks if a git repository exists in current dir  == auto getGitInfo() ->
// bool;
bool is_git_repository(git_repository **repo) {
    int e = git_repository_open(repo, ".");
    if (e > 0) {
        return false;
    }
    return true;
}

void initRepository(git_repository **repo) {
    int e = git_repository_init(repo, ".", 0);
    if (e > 0) {
        gitErr();
    }
}

void main_thread(int &argc, std::vector<std::string> &argv) {
    git_repository *curr = NULL;
    initRepository(&curr);
    exit(0);
    if (!is_git_repository(&curr)) {
        gitErr();
    }

    git_repository_free(curr);
}

int main(int argc, char **argv) {
    git_libgit2_init();
    std::vector<std::string> av{};
    for (auto i = 0; i < argc; i++) {
        av.push_back(argv[i]);
    }
    main_thread(argc, av);
    git_libgit2_shutdown();
    return 0;
}
