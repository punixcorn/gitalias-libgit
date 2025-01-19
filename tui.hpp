#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string.hpp>
#include <iostream>
#include <string>
#include <vector>
namespace gitalias {
namespace tui {
void tui_init() {
    using namespace ftxui;

    // Variables to store input and selection
    std::string user_input;
    int selected_index = 0;

    // Header
    auto header = Renderer([] {
        return hbox({
                   text("=== Welcome to FTXUI ===") | bold |
                       color(Color::Green),
               }) |
               center;
    });

    // Input box
    auto input = Input(&user_input, "Enter your name");

    // List of items
    std::vector<std::string> items = {
        "Option 1: Say Hello",
        "Option 2: Display Info",
        "Option 3: Exit",
    };
    auto menu = Menu(&items, &selected_index);

    // Main layout combining all elements
    auto main_container = Container::Vertical({
        input,
        menu,
    });

    auto main_renderer = Renderer(main_container, [&] {
        return vbox({
                   header->Render(),
                   text("Please provide your input below:") | dim,
                   input->Render(),
                   text("Choose an option:") | dim,
                   menu->Render(),
               }) |
               border | center;
    });

    // Create the interactive screen
    auto screen = ScreenInteractive::FitComponent();
    screen.Loop(main_renderer);

    // Process results
    if (!user_input.empty()) {
        std::cout << "Hello, " << user_input << "!\n";
    }
    std::cout << "You selected: " << items[selected_index] << "\n";
}

}  // namespace tui

}  // namespace gitalias
