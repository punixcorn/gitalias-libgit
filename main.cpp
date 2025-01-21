/*
        project: gitalias-libgit
        author: punixcorn
*/
// done for local handling
#include "git.hpp"
// yet to start
#include "tui.hpp"
// to handle options
#include "options.hpp"

using namespace gitalias;
int main(int argc, char **argv) {
    try {
        git_libgit2_init();

        std::shared_ptr<options::Globals> g =
            std::make_shared<options::Globals>();
        std::shared_ptr<options::Trips> t = std::make_shared<options::Trips>();

        const auto [map, desc] = options::init_options(argc, argv, g);
        options::handle_options(map, g, t);

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
