

/* boost includes */
#include <sys/cdefs.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <memory>

#define arg(cmd, type) args[cmd].as<type>()

namespace gitalias {
namespace options {

namespace opt = boost::program_options;
using std::string, std::vector, std::string_view, std::nothrow;

/* global varibales -> helps prevents extreme function arguments*/
class Globals {
   public:
    Globals() {};
    ~Globals() {};
    string messagebox, addbox, reponame, repodes;
    bool mode; /* user request mode */
};

class Trips {
   public:
    Trips() { memset(this, 0, sizeof(*this)); };
    ~Trips() {};
    bool commit, message, add, init, branch, switch_, deleteBranch, merge, pull,
        push, origin, log, status, repoName, repoDes, repoMode, verbose, Rreset,
        git;
};

struct options_ret {
    opt::variables_map map;
    opt::options_description desc;
};

constexpr inline options_ret init_options(int argc, char **argv,
                                          std::shared_ptr<Globals> G) noexcept {
    opt::options_description desc(string(argv[0]).append(" options"));
    desc.add_options()("help,h", "print this message")(
        "init,i", "init a repository")("commit,c", "add all and commit")(
        "add,a", opt::value<vector<string>>()->multitoken(),
        "add [files] only")(
        "message,m", opt::value<vector<string>>()->multitoken(),
        "add a message")("branch,b", opt::value<string>()->implicit_value(""),
                         "create a branch")("switch,s", opt::value<string>(),
                                            "switch to a branch")(
        "delete,d", opt::value<string>(), "delete a branch")(
        "Merge,M", opt::value<string>(),
        "merge branch [ branch-name ] with current branch")(
        "Pull,P", opt::value<string>()->implicit_value(""), "pull from origin")(
        "push,p", opt::value<string>()->implicit_value(""), "push into origin")(
        "Clone,C", opt::value<vector<string>>()->multitoken(),
        "clone a repository with given \"user/repo-name\"")(
        "Request,R", opt::value<string>()->default_value("https"),
        "Protocol to use when cloning, [ https/ssh ]")(
        "verbose,v", "print out parsed code")("log,l", "show log files")(
        "Status,S", "show statuts")("origin,o", opt::value<string>(),
                                    "add an origin")(
        "repo,r", opt::value<string>(&G->reponame),
        "name for creating an online repo * [i]")(
        "Des,D", opt::value<string>(&G->repodes),
        "description for the online repo * [ii]")(
        "type,t", opt::value<bool>(&G->mode)->value_name("bool"),
        "if repo should be private * [iii]")(
        "undo,u", opt::value<vector<string>>()->multitoken(),
        "*Reset back to a commit ,eg gitalias -u "
        "[ hard / soft / mixed ] "
        "[ number of commits to reset back ]")(
        "Grab,G", opt::value<string>(),
        "grab a specific folder from a github repo")(
        "Visibility,V", opt::value<vector<string>>()->multitoken(),
        "Modify the status of a repo[private,public]")("Json,J",
                                                       "List the Json Options");
    // remove --git
    //("git,g", opt::value<vector<string>>()->multitoken(),"append git
    // commands [ gitalias -g \" git ... \" ]")

    opt::variables_map args;
    opt::store(opt::command_line_parser(argc, argv)
                   .options(desc)
                   .style(opt::command_line_style::default_style |
                          opt::command_line_style::allow_sticky)
                   .run(),
               args);
    opt::notify(args);
    return {args, desc};
}

inline void handle_options(opt::variables_map map, std::shared_ptr<Globals> g,
                           std::shared_ptr<Trips> t) {}

}  // namespace options
}  // namespace gitalias
