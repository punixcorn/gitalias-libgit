#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <git2.h>
#include <git2/buffer.h>
#include <git2/checkout.h>
#include <git2/clone.h>
#include <git2/commit.h>
#include <git2/credential.h>
#include <git2/deprecated.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/merge.h>
#include <git2/object.h>
#include <git2/remote.h>
#include <git2/repository.h>
#include <git2/types.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#define PRIVATE_KEY_PATH "/home/potato/.ssh/id_ed25519"
#define PUBLIC_KEY_PATH "/home/potato/.ssh/id_ed25519.pub"
namespace gitalias {
namespace libgit {

template <typename T>
concept isStr = requires(T a) {
    std::is_constructible_v<T> &&
        (std::is_same_v<T, std::string> || std::is_same_v<T, std::string &>);
    { a.c_str() } -> std::same_as<const char *>;
    { a + "string" } -> std::same_as<std::string>;
};

/*
 * Handles any error git throws,
 * Prints out the *error* and exits
 */
inline void handle_git_err() {
    throw std::runtime_error(
        std::format("[ERR] gitalias2 : {}", git_error_last()->message));
}

/*
 * Clone and online repo `url` to `path`
 * returns a git_repository * to the cloned repo
 */
template <typename T>
    requires isStr<T>
git_repository *clone_repository(T url, T path) {
    git_repository *cloned_repo = nullptr;
    int e = git_clone(&cloned_repo, url.c_str(), path.c_str(), NULL);
    if (e > 0) {
        handle_git_err();
    }
    return cloned_repo;
};

/*
 * Get the current branch name you're on
 */
inline void get_current_branch_name(git_repository *repo,
                                    std::string &branch_name) {
    int error = 0;
    git_reference *head = NULL;

    // Retrieve the HEAD reference
    error = git_repository_head(&head, repo);
    if (error != 0) {
        handle_git_err();
    }

    // Check if HEAD is pointing to a branch
    if (git_reference_is_branch(head)) {
        // Get the branch name
        const char *_branch_name = git_reference_shorthand(head);
        branch_name = _branch_name;
    } else {
        handle_git_err();
    }
    git_reference_free(head);
}

/*
 * opens a repository at `path`
 * by default `path` is set to current directory
 */
template <typename T = std::string>
    requires isStr<T>
inline void open_repository(git_repository **repo, T path = ".") {
    fmt::println("opening repo...");
    int e = git_repository_open_ext(repo, path.c_str(),
                                    GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
    if (e == 0) return;

    e = git_repository_open_ext(repo, path.c_str(), 0, NULL);
    if (e == 0) return;

    git_buf findroot = {0};
    e = git_repository_discover(&findroot, path.c_str(), 0, NULL);
    if (e != 0) handle_git_err();

    e = git_repository_open_ext(repo, findroot.ptr,
                                GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);

    git_buf_free(&findroot);
    if (e == 0)
        fmt::print("git found\n");
    else
        handle_git_err();
}
/*
 * Struct for holding information used for making commits
 */
struct commit_t {
    commit_t(git_repository *repo) {
        git_signature *_sig = nullptr;

        // Get the default signature (name and email) for the current user
        int error = git_signature_default(&sig, repo);
        if (error < 0) {
            std::cerr << "Failed to get default signature: "
                      << git_error_last()->message << std::endl;
            return;
        }

        // Output the signature (author's name and email)
        std::cout << "Author Name: " << sig->name << std::endl;
        std::cout << "Author Email: " << sig->email << std::endl;
        this->sig = _sig;
    }
    ~commit_t() { git_signature_free(this->sig); }
    std::string message;
    std::string encoding = "UTF-8";
    git_signature *sig;
};
/*
 * Create a commit in `repo` using `c` data
 */
inline void create_git_commit(git_repository *repo, commit_t *c) {
    fmt::println("creating a commit...");
    git_oid tree_id, parent_id, commit_id;
    git_tree *tree = nullptr;
    git_commit *parent = nullptr;
    git_index *index = nullptr;

    // Get the index and write it to a tree
    if (git_repository_index(&index, repo) != 0) {
        std::cerr << "Failed to get the index.\n";
        return;
    }

    if (git_index_write_tree(&tree_id, index) != 0) {
        std::cerr << "Failed to write the index to a tree.\n";
        git_index_free(index);
        return;
    }

    if (git_tree_lookup(&tree, repo, &tree_id) != 0) {
        std::cerr << "Failed to look up the tree object.\n";
        git_index_free(index);
        return;
    }

    git_index_free(index);

    // Get HEAD as a commit object (parent)
    if (git_reference_name_to_id(&parent_id, repo, "HEAD") == 0) {
        if (git_commit_lookup(&parent, repo, &parent_id) != 0) {
            std::cerr << "Failed to lookup parent commit.\n";
            git_tree_free(tree);
            return;
        }
    }

    // Perform the commit
    int parent_count = (parent != nullptr) ? 1 : 0;
    if (git_commit_create_v(&commit_id, repo, "HEAD", c->sig, c->sig,
                            c->encoding.c_str(), c->message.c_str(), tree,
                            parent_count, parent) != 0) {
        std::cerr << "Failed to create the commit.\n";
    } else {
        std::cout << "Commit created successfully.\n";
    }

    git_tree_free(tree);
    if (parent) {
        git_commit_free(parent);
    }
}

/*
 * Checks if a git repository exists at `path`
 * by default, `path` is the current directory
 */
inline auto is_git_repository(git_repository **repo, std::string path = ".")
    -> bool {
    fmt::println("checking if a repo exists...");
    int e = git_repository_open_ext(repo, path.c_str(),
                                    GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
    if (e > 0) {
        return false;
    }
    return true;
}
/*
 * Inits a local repository at `path`
 * By default `path` is current directory
 */
inline void initRepository(const char *path = ".") {
    fmt::println("Initing a repo...");
    git_repository *repo = nullptr;
    int e = git_repository_init(&repo, path, 0);
    fmt::print("Init Repo");
    if (e > 0) {
        handle_git_err();
    }
    git_repository_free(repo);
}
/*
 * Create a Branch in `repo` using the name `branch_name`
 * From the branch name `start_point`
 * This will checkout to the `branch_name` if `checkout = true`
 * By default `checkout = false`
 *
 *  Example
 * `create_branch(repo,"new_branch","main")`
 */
inline void create_branch(git_repository *repo, const std::string &branch_name,
                          const std::string &start_point,
                          bool checkout = false) {
    std::cout << "Creating Branch...\n";
    git_reference *new_branch = nullptr;
    git_object *target = nullptr;

    // Lookup the target commit or reference
    if (git_revparse_single(&target, repo, start_point.c_str()) != 0) {
        std::cerr << "Failed to find starting point: " << start_point << "\n";
        return;
    }

    // Create the new branch
    if (git_branch_create(&new_branch, repo, branch_name.c_str(),
                          (git_commit *)target, 0) != 0) {
        std::cerr << "Failed to create branch: " << branch_name << "\n";
        git_object_free(target);
        return;
    }

    std::cout << "Branch '" << branch_name << "' created successfully.\n";

    // Optionally, checkout the new branch
    if (checkout) {
        git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        opts.checkout_strategy = GIT_CHECKOUT_SAFE;

        if (git_repository_set_head(
                repo, ("refs/heads/" + branch_name).c_str()) != 0 ||
            git_checkout_head(repo, &opts) != 0) {
            std::cerr << "Failed to checkout branch: " << branch_name << "\n";
        } else {
            std::cout << "Checked out branch: " << branch_name << "\n";
        }
    }

    // Free resources
    git_reference_free(new_branch);
    git_object_free(target);
}
/*
 * Add `files` to `repo` to be commited
 */
inline void add_files_to_index(git_repository *repo,
                               const std::vector<std::string> &files) {
    fmt::println("Adding files to repo...");
    git_index *index = nullptr;

    if (git_repository_index(&index, repo) != 0) {
        std::cerr << "Failed to get the index.\n";
        return;
    }

    for (const auto &file : files) {
        if (git_index_add_bypath(index, file.c_str()) != 0) {
            std::cerr << "Failed to add file to index: " << file << "\n";
        } else {
            std::cout << "Added file to index: " << file << "\n";
        }
    }

    if (git_index_write(index) != 0) {
        std::cerr << "Failed to write the index to disk.\n";
    }

    git_index_free(index);
}

/*
 * Callback for SSH key authentication
 */
inline int credential_cb(git_cred **out, const char *url,
                         const char *username_from_url,
                         unsigned int allowed_types, void *payload) {
    const char *private_key_path =
        PRIVATE_KEY_PATH;  //"/home/potato/.ssh/id_ed25519";
    const char *public_key_path =
        PUBLIC_KEY_PATH;  // "/home/potato/.ssh/id_ed25519.pub";
    const char *passphrase = "";
    return git_cred_ssh_key_new(out, username_from_url, public_key_path,
                                private_key_path, passphrase);
}

inline bool checkForError(int e, const char *m) {
    if (e < 0) {
        std::cout << m << '\n';
        return 1;
    }
    return 0;
}

inline int fetchhead_ref_cb(const char *ref_name, const char *remote_url,
                            const git_oid *oid, unsigned int is_merge,
                            void *payload) {
    git_oid *branchOidToMerge = (git_oid *)payload;

    git_oid_cpy(branchOidToMerge, oid);
    return 0;  // Return 0 to continue iteration
}

inline void fast_forward_merge(git_repository *repo) {
    git_remote *remote;
    int error = git_remote_lookup(&remote, repo, "origin");
    if (!checkForError(error, "Remote lookup")) {
        git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
        options.callbacks.credentials = credential_cb;
        error = git_remote_fetch(
            remote, NULL, /* refspecs, NULL to use the configured ones */
            &options,     /* options, empty for defaults */
            "pull"); /* reflog mesage, usually "fetch" or "pull", you can leave
                        it NULL for "fetch" */
        if (!checkForError(error, "Remote fetch")) {
            git_oid branchOidToMerge;
            git_repository_fetchhead_foreach(repo, fetchhead_ref_cb,
                                             &branchOidToMerge);
            git_annotated_commit *their_heads[1];

            error = git_annotated_commit_lookup(&their_heads[0], repo,
                                                &branchOidToMerge);
            checkForError(error, "Annotated commit lookup");

            git_merge_analysis_t anout;
            git_merge_preference_t pout;

            std::cout << "Try analysis";

            error = git_merge_analysis(
                &anout, &pout, repo, (const git_annotated_commit **)their_heads,
                1);

            checkForError(error, "Merge analysis");

            if (anout & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
                std::cout << "up to date";
                git_annotated_commit_free(their_heads[0]);
                git_repository_state_cleanup(repo);
                git_remote_free(remote);
                return;
            } else if (anout & GIT_MERGE_ANALYSIS_FASTFORWARD) {
                std::cout << "fast-forwarding";

                git_reference *ref;
                git_reference *newref;

                const char *name = "refs/heads/main";

                if (git_reference_lookup(&ref, repo, name) == 0)
                    git_reference_set_target(&newref, ref, &branchOidToMerge,
                                             "pull: Fast-forward");

                git_reset_from_annotated(repo, their_heads[0], GIT_RESET_HARD,
                                         NULL);

                git_reference_free(ref);
                git_repository_state_cleanup(repo);
            }

            git_annotated_commit_free(their_heads[0]);
            git_repository_state_cleanup(repo);
            git_remote_free(remote);
            return;
        }
    }
    git_remote_free(remote);
    return;
}

/*
 * Push local files to remote
 */
inline void push_to_remote(git_repository *repo, const char *remote_name,
                           const char *branch_name) {
    git_remote *remote = nullptr;
    git_remote_lookup(&remote, repo, remote_name);
    std::cout << "branch name in fun: " << branch_name << '\n';
    std::string refspec_format =
        std::format("refs/heads/{}:refs/heads/{}", branch_name, branch_name);
    const char *refspec = refspec_format.c_str();
    git_strarray refspecs = {const_cast<char **>(&refspec), 1};

    git_push_options push_opts = GIT_PUSH_OPTIONS_INIT;
    push_opts.callbacks.credentials = credential_cb;

    if (git_remote_push(remote, &refspecs, &push_opts) == 0) {
        std::cout << "Push successful.\n";
    } else {
        std::cerr << "Push failed.\n";
        handle_git_err();
    }

    git_remote_free(remote);
}

/*
 * Switch to branch `branch_name`
 */
inline void switch_branch(git_repository *repo, const char *branch_name) {
    fmt::println("Switching branch...");

    int error = 0;
    git_reference *branch_ref = NULL;
    git_object *tree = NULL;

    // Look up the branch reference
    error = git_branch_lookup(&branch_ref, repo, branch_name, GIT_BRANCH_LOCAL);
    if (error != 0) {
        handle_git_err();
    }

    // Get the branch's commit tree
    error = git_reference_peel(&tree, branch_ref, GIT_OBJECT_TREE);
    if (error != 0) {
        const git_error *e = git_error_last();
        printf("Error %d: %s\n", error,
               (e && e->message) ? e->message : "Unknown");
        git_reference_free(branch_ref);
        handle_git_err();
    }

    // Checkout the branch
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;  // Use GIT_CHECKOUT_FORCE if
                                                 // you want to override changes
    error = git_checkout_tree(repo, tree, &opts);
    if (error != 0) {
        git_object_free(tree);
        git_reference_free(branch_ref);
        handle_git_err();
    }

    // Update HEAD to point to the new branch
    error = git_repository_set_head(repo, git_reference_name(branch_ref));
    if (error != 0) {
        handle_git_err();
    }

    git_object_free(tree);
    git_reference_free(branch_ref);
}

/*
 * Delete a branch `branch_name` in `repo`
 */
inline void delete_branch(git_repository *repo, const char *branch_name) {
    int error = 0;
    git_reference *branch_ref = NULL;

    // Look up the branch reference
    error = git_branch_lookup(&branch_ref, repo, branch_name, GIT_BRANCH_LOCAL);
    if (error != 0) {
        handle_git_err();
    }

    // Delete the branch
    error = git_branch_delete(branch_ref);
    if (error != 0) {
        handle_git_err();
    } else {
        printf("Branch '%s' deleted successfully.\n", branch_name);
    }

    git_reference_free(branch_ref);
}

/*
 * Function to get all branch names in `repo`
 */
inline std::vector<std::string> get_git_branches(git_repository *repo) {
    std::vector<std::string> branch_names;
    git_branch_iterator *iter = nullptr;
    git_reference *ref = nullptr;
    git_branch_t branch_type;

    // Create an iterator for all branches
    if (git_branch_iterator_new(&iter, repo, GIT_BRANCH_ALL) != 0) {
        handle_git_err();
    }

    // Iterate over branches
    while (git_branch_next(&ref, &branch_type, iter) == 0) {
        const char *branch_name = nullptr;
        if (git_branch_name(&branch_name, ref) == 0 && branch_name) {
            branch_names.push_back(branch_name);
        }
        git_reference_free(ref);
    }

    git_branch_iterator_free(iter);
    return branch_names;
}

/*
 * Merge `source_branch` into `target_branch` in `repo`
 */
inline void merge_branches2(git_repository *repo, const char *target_branch,
                            const char *source_branch) {
    git_reference *target_ref = nullptr;
    git_reference *source_ref = nullptr;
    git_oid source_oid;
    git_annotated_commit *source_annotated = nullptr;

    // Lookup the target branch
    int error =
        git_branch_lookup(&target_ref, repo, target_branch, GIT_BRANCH_LOCAL);
    if (error != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Could not find target branch '" << target_branch
                  << "': " << (e && e->message ? e->message : "Unknown")
                  << std::endl;
        return;
    }

    // Lookup the source branch
    error =
        git_branch_lookup(&source_ref, repo, source_branch, GIT_BRANCH_LOCAL);
    if (error != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Could not find source branch '" << source_branch
                  << "': " << (e && e->message ? e->message : "Unknown")
                  << std::endl;
        git_reference_free(target_ref);
        return;
    }

    // Get the OID of the source branch
    git_reference_name_to_id(&source_oid, repo, git_reference_name(source_ref));

    // Create an annotated commit from the source branch
    error = git_annotated_commit_lookup(&source_annotated, repo, &source_oid);
    if (error != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Could not create annotated commit: "
                  << (e && e->message ? e->message : "Unknown") << std::endl;
        git_reference_free(target_ref);
        git_reference_free(source_ref);
        return;
    }

    // Perform the merge analysis
    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    error =
        git_merge_analysis(&analysis, &preference, repo,
                           (const git_annotated_commit **)&source_annotated, 1);
    if (error != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Merge analysis failed: "
                  << (e && e->message ? e->message : "Unknown") << std::endl;
        git_annotated_commit_free(source_annotated);
        git_reference_free(target_ref);
        git_reference_free(source_ref);
        return;
    }

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        std::cout
            << "Target branch is already up-to-date with source branch.\n";
    } else if (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) {
        std::cout << "Fast-forwarding the target branch.\n";

        git_reference *new_ref = nullptr;
        error = git_reference_set_target(&new_ref, target_ref, &source_oid,
                                         "Fast-forward merge");
        if (error != 0) {
            const git_error *e = git_error_last();
            std::cerr << "Error: Fast-forward merge failed: "
                      << (e && e->message ? e->message : "Unknown")
                      << std::endl;
        } else {
            std::cout << "Successfully fast-forwarded '" << target_branch
                      << "' to match '" << source_branch << "'.\n";
        }
        git_reference_free(new_ref);

        git_reset_from_annotated(repo, source_annotated, GIT_RESET_HARD,
                                 nullptr);
    } else if (analysis & GIT_MERGE_ANALYSIS_NORMAL) {
        std::cout << "Performing a manual merge.\n";

        // Perform the merge
        git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
        checkout_opts.checkout_strategy =
            GIT_CHECKOUT_SAFE | GIT_CHECKOUT_RECREATE_MISSING;

        error =
            git_merge(repo, (const git_annotated_commit **)&source_annotated, 1,
                      &merge_opts, &checkout_opts);
        if (error != 0) {
            const git_error *e = git_error_last();
            std::cerr << "Error: Manual merge failed: "
                      << (e && e->message ? e->message : "Unknown")
                      << std::endl;
        } else {
            std::cout
                << "Merge completed. Please commit the changes to finalize.\n";
        }
    } else {
        std::cerr << "Merge cannot be performed.\n";
    }

    // Cleanup
    git_annotated_commit_free(source_annotated);
    git_reference_free(target_ref);
    git_reference_free(source_ref);
}

/*
 *
 */
inline void resolve_merge(git_repository *repo) {
    git_index *index = nullptr;

    // Open the repository index
    if (git_repository_index(&index, repo) != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Could not open index: "
                  << (e && e->message ? e->message : "Unknown") << std::endl;
        return;
    }

    // Check if the index still has conflicts
    if (git_index_has_conflicts(index)) {
        std::cerr << "Error: Conflicts still exist in the index. Please "
                     "resolve them.\n";
        git_index_free(index);
        return;
    }

    // Write the resolved index to disk
    if (git_index_write(index) != 0) {
        const git_error *e = git_error_last();
        std::cerr << "Error: Could not write index to disk: "
                  << (e && e->message ? e->message : "Unknown") << std::endl;
        git_index_free(index);
        return;
    }

    std::cout << "All conflicts resolved and index updated.\n";
    git_index_free(index);
}

/*
 * Commit a merger on `repo` with commit info `c` and a `message`
 */
inline void commit_merge4(git_repository *repo, commit_t *c,
                          const char *message) {
    git_index *index = nullptr;
    git_tree *tree = nullptr;
    git_oid tree_id, commit_id;
    git_commit *parent_commit = nullptr;

    // Open the repository index
    if (git_repository_index(&index, repo) != 0) {
        std::cerr << "Error: Could not open index.\n";
        return;
    }

    // Write the index to a tree
    if (git_index_write_tree(&tree_id, index) != 0) {
        std::cerr << "Error: Could not write tree.\n";
        git_index_free(index);
        return;
    }

    if (git_tree_lookup(&tree, repo, &tree_id) != 0) {
        std::cerr << "Error: Could not lookup tree.\n";
        git_index_free(index);
        return;
    }

    // Get the current HEAD commit
    git_reference *head_ref = nullptr;
    if (git_repository_head(&head_ref, repo) != 0) {
        std::cerr << "Error: Could not get HEAD reference.\n";
        git_tree_free(tree);
        git_index_free(index);
        return;
    }

    git_oid head_oid;
    git_reference_name_to_id(&head_oid, repo, git_reference_name(head_ref));
    if (git_commit_lookup(&parent_commit, repo, &head_oid) != 0) {
        std::cerr << "Error: Could not lookup HEAD commit.\n";
        git_reference_free(head_ref);
        git_tree_free(tree);
        git_index_free(index);
        return;
    }

    // Create the commit
    if (git_commit_create_v(&commit_id, repo, "HEAD", c->sig, c->sig, nullptr,
                            message, tree, 1, parent_commit) != 0) {
        std::cerr << "Error: Could not create merge commit.\n";
    } else {
        std::cout << "Merge commit created successfully.\n";

        // Cleanup repository state
        if (git_repository_state_cleanup(repo) != 0) {
            const git_error *e = git_error_last();
            std::cerr << "Error: Could not clean up repository state: "
                      << (e && e->message ? e->message : "Unknown") << "\n";
        } else {
            std::cout << "Repository state cleaned up successfully.\n";
        }
    }

    // Cleanup resources
    git_commit_free(parent_commit);
    git_reference_free(head_ref);
    git_tree_free(tree);
    git_index_free(index);
}

/*
 * Print git status , git status
 */
inline void print_git_status(git_repository *repo) {
    git_status_list *status = NULL;
    git_status_options status_opts = GIT_STATUS_OPTIONS_INIT;
    size_t i;
    int err;

    // Step 2: Configure status options
    status_opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                        GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                        GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    // Step 3: Get the status list
    err = git_status_list_new(&status, repo, &status_opts);
    if (err != 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error getting status: %s\n",
                e && e->message ? e->message : "Unknown error");
        git_repository_free(repo);
        return;
    }

    // Step 4: Iterate through the status list
    size_t status_count = git_status_list_entrycount(status);
    for (i = 0; i < status_count; ++i) {
        const git_status_entry *entry = git_status_byindex(status, i);

        // Check the status flags
        if (entry->status & GIT_STATUS_INDEX_NEW)
            printf("New file (staged): %s\n",
                   entry->head_to_index->new_file.path);
        else if (entry->status & GIT_STATUS_INDEX_MODIFIED)
            printf("Modified (staged): %s\n",
                   entry->head_to_index->new_file.path);
        else if (entry->status & GIT_STATUS_INDEX_DELETED)
            printf("Deleted (staged): %s\n",
                   entry->head_to_index->old_file.path);

        if (entry->status & GIT_STATUS_WT_NEW)
            printf("New file (unstaged): %s\n",
                   entry->index_to_workdir->new_file.path);
        else if (entry->status & GIT_STATUS_WT_MODIFIED)
            printf("Modified (unstaged): %s\n",
                   entry->index_to_workdir->new_file.path);
        else if (entry->status & GIT_STATUS_WT_DELETED)
            printf("Deleted (unstaged): %s\n",
                   entry->index_to_workdir->old_file.path);
    }

    git_status_list_free(status);
}

/*
 * Print Git log, Same as git -l
 */
inline void print_git_log(git_repository *repo) {
    git_revwalk *walker = NULL;
    git_oid oid;
    git_commit *commit = NULL;
    int err;

    // Step 2: Create a revwalker
    err = git_revwalk_new(&walker, repo);
    if (err != 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error creating revwalker: %s\n",
                e && e->message ? e->message : "Unknown error");
        git_repository_free(repo);
        return;
    }

    // Step 3: Configure the walker
    git_revwalk_sorting(walker,
                        GIT_SORT_TIME | GIT_SORT_REVERSE);  // Newest first
    err = git_revwalk_push_head(walker);
    if (err != 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error pushing HEAD to walker: %s\n",
                e && e->message ? e->message : "Unknown error");
        git_revwalk_free(walker);
        git_repository_free(repo);
        return;
    }

    // Step 4: Iterate through commits
    while ((err = git_revwalk_next(&oid, walker)) == 0) {
        // Look up the commit object
        err = git_commit_lookup(&commit, repo, &oid);
        if (err != 0) {
            const git_error *e = git_error_last();
            fprintf(stderr, "Error looking up commit: %s\n",
                    e && e->message ? e->message : "Unknown error");
            break;
        }

        // Get commit details
        const char *message = git_commit_message(commit);
        const git_signature *author = git_commit_author(commit);
        char oid_str[GIT_OID_HEXSZ + 1];
        git_oid_tostr(oid_str, sizeof(oid_str), &oid);

        // Print commit details
        printf("Commit: %s\n", oid_str);
        printf("Author: %s <%s>\n", author->name, author->email);
        printf("Date: %s", ctime(&author->when.time));
        printf("\n    %s\n\n", message);

        // Free the commit object
        git_commit_free(commit);
    }

    if (err != GIT_ITEROVER) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error during revision walk: %s\n",
                e && e->message ? e->message : "Unknown error");
    }

    // Step 5: Cleanup
    git_revwalk_free(walker);
}

/*
 * set the message in `commit` with `message`
 */
inline void set_commit_message(commit_t *commit, std::string message) noexcept {
    fmt::println("setting message :{}", message);
    commit->message = message;
}

inline void init_repository(git_repository *repo) {
    if (!is_git_repository(&repo)) {
        initRepository();
    }
    open_repository(&repo);
}

#ifdef debug
inline void main_thread(int &argc, std::vector<std::string> &vc) {
    git_repository *curr = nullptr;

    if (!is_git_repository(&curr)) {
        initRepository();
    }
    open_repository(&curr);
    std::string branch;
    get_current_branch_name(curr, branch);
    commit_t commit(curr);
    commit_message(&commit, "fixes");
    add_files_to_index(curr, vc);
    create_git_commit(curr, &commit);
    const auto pull_and_merge = [&]() { fast_forward_merge(curr); };
    const bool checkout_to_new_branch = false;
    create_branch(curr, "newBranch", "master", checkout_to_new_branch);
    push_to_remote(curr, "origin", branch.c_str());
    create_branch(curr, "foo", branch, false);
    switch_branch(curr, "main");
    for (const auto &i : get_git_branches(curr)) {
        fmt::println("{}", i);
    }
    std::ranges::for_each(get_git_branches(curr) |
                              std::ranges::views::filter([](std::string &str) {
                                  if (str.find("HEAD") != std::string::npos) {
                                      return false;
                                  }
                                  return true;
                              }),
                          [](std::string str) { fmt::println("{}", str); });

    // https://github.com/libgit2/libgit2/issues/3940 - unclear merge in
    // docs https://github.com/libgit2/libgit2/blob/main/examples/merge.c
    // -merge.c
    const auto merge = [&]() {
        merge_branches2(curr, "main", "foo");
        commit_merge4(curr, &commit, "the merger of main and foo 2");
        resolve_merge(curr);
    };
    print_git_status(curr);
    print_git_log(curr);

    git_repository_free(curr);
}
#endif
}  // namespace libgit
}  // namespace gitalias
