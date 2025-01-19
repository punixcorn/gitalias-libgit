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

#include <cstdlib>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

// done for local handling
#include "git.hpp"
// yet to start
#include "tui.hpp"

// handle options using boost_options

using namespace gitalias;
int main(int argc, char **argv) {
    try {
        git_libgit2_init();
        auto argv_vec = std::span(std::move(argv), argc - 1) |
                        std::ranges::views::transform(
                            [](char *str) { return std::string(str); }) |
                        std::ranges::to<std::vector<std::string>>();

        (void)(argv);
        libgit::main_thread(argc, argv_vec);
        tui::tui_init();

        git_libgit2_shutdown();
    } catch (const std::runtime_error &e) {
        git_libgit2_shutdown();
        fmt::print("{}\n", e.what());
        throw e;
    } catch (...) {
        git_libgit2_shutdown();
        fmt::print("last known error {}\n", git_error_last()->message);
        throw;
    }
    return 0;
}
