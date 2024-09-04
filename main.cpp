/*
        project: gitalias-libgit
        author: punixcorn
*/

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <git2.h>
#include <git2/buffer.h>
#include <git2/clone.h>
#include <git2/commit.h>
#include <git2/deprecated.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/object.h>
#include <git2/repository.h>
#include <git2/types.h>

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace std {
using fmt::format, fmt::print;
}

template <typename T>
concept isStr = requires(T a) {
    std::is_constructible_v<T> &&
        (std::is_same_v<T, std::string> || std::is_same_v<T, std::string &>);
    { a.c_str() } -> std::same_as<const char *>;
    { a + "string" } -> std::same_as<std::string>;
};

// throws err
inline void gitErr() {
    throw std::runtime_error(
        std::format("[ERR] gitalias2 : {}", git_error_last()->message));
}

template <typename T>
    requires isStr<T>
void clone_repository(git_repository **repo, T url, T path) {
    int e = git_clone(repo, url.c_str(), path.c_str(), NULL);
    if (e > 0) {
        gitErr();
    }
};

// opens a repository
template <typename T = std::string>
    requires isStr<T>
void open_repository(git_repository **repo, T path = ".") {
    int e = git_repository_open_ext(repo, path.c_str(),
                                    GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
    if (e == 0) return;

    e = git_repository_open_ext(repo, path.c_str(), 0, NULL);
    if (e == 0) return;

    git_buf findroot = {0};
    e = git_repository_discover(&findroot, path.c_str(), 0, NULL);
    if (e != 0) gitErr();

    e = git_repository_open_ext(repo, findroot.ptr,
                                GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);

    git_buf_free(&findroot);
    if (e == 0)
        fmt::print("git found\n");
    else
        gitErr();
}

struct commit_t {
    commit_t(git_commit **commit, git_repository **repo) {
        const git_oid *oid;
        int e = git_commit_lookup(commit, *repo, oid);
    };

    std::string author;
    git_signature *me;
    std::string encoding = "UTF-8";
    std::string message;
    std::string committer;
    git_signature *a;
};

// create a commit
void create_git_commit(git_repository *repo, commit_t *c) {
    git_oid tree_id, parent_id, commit_id;
    git_tree *tree;
    git_commit *parent;
    git_index *index;

    /* Get the index and write it to a tree */
    git_repository_index(&index, repo);
    git_index_write_tree(&tree_id, index);

    /* Get HEAD as a commit object to use as the parent of the commit */
    git_reference_name_to_id(&parent_id, repo, "HEAD");
    git_commit_lookup(&parent, repo, &parent_id);

    /* Do the commit */
    git_commit_create_v(&commit_id, repo, "HEAD", c->a, c->a,
                        c->encoding.c_str(), c->message.c_str(), tree, 1,
                        parent);
}
// https://stackoverflow.com/questions/27672722/libgit2-commit-example

// checks if a git repository exists in current dir  == auto getGitInfo() ->
// bool;
bool is_git_repository(git_repository **repo) {
    int e =
        git_repository_open_ext(NULL, ".", GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
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
    if (!is_git_repository(&curr)) {
        gitErr();
    }
    open_repository(&curr);
    git_repository_free(curr);
}

int main(int argc, char **argv) {
    git_libgit2_init();
    std::vector<std::string> av{};
    for (auto i = 0; i < argc; i++) {
        av.push_back(argv[i]);
    }
    try {
        main_thread(argc, av);
    } catch (const std::runtime_error &e) {
        fmt::print("{}\n", e.what());
    } catch (...) {
        fmt::print("last known error {}\n", git_error_last()->message);
    }
    git_libgit2_shutdown();
    return 0;
}
