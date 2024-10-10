#include "urlencode.h"
#include <sstream>


std::string UrlEncode(std::string_view str) {
    std::ostringstream os;
    for (size_t i = 0; i < str.size(); ++i) {
        auto symbol = str[i];
        if (symbol == ' ') {
            os << '+';
        } else if ((symbol >= 48 && symbol <= 57)
                   || (symbol >= 65 && symbol <= 90)
                   || (symbol >= 97 && symbol <= 122)
                   || symbol == '-' || symbol == '_' || symbol == '.' || symbol == '~') {
            os << symbol;
        } else {
            os << '%' << std::hex << static_cast<int>(symbol);
        }
    }

    return os.str();
}
