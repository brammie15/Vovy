#include <iostream>


#if _DEBUG
// ReSharper disable once CppUnusedIncludeDirective
#if __has_include(<vld.h>)
#include <vld.h>
#endif
#endif

#include <filesystem>

#include "VApp.h"


int main() {
    if (!std::filesystem::exists("resources") || std::filesystem::is_empty("resources")) {
        std::cerr << "Error: The 'resources' folder does not exist." << std::endl;
        std::cerr << "Try to rebuild because python failed to download the resources." << std::endl;
        std::cerr << "Worst case look in \"download resources.py\" and download the resources manually." << std::endl;
        return EXIT_FAILURE;
    }


    VApp app{};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;

        return EXIT_FAILURE;
    }

    std::cout << "Bye :p" << std::endl;
    return EXIT_SUCCESS;
}
