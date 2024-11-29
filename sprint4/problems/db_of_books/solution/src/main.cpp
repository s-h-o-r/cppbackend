#include "book_manager.h"
#include "request_handler.h"

#include <iostream>

using namespace std::literals;

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        book_manager::BookManager book_manager{argv[1]};

        handler::RequestHandler handler{&book_manager};
        handler.ProcessQuery();

    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
