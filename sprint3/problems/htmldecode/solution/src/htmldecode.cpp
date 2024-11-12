#include "htmldecode.h"

#include <sstream>
#include <unordered_map>

using namespace std::literals;

const std::unordered_map<std::string, char> HTML_MNEMONICS = {
    {"lt"s, '<'}, {"LT"s, '<'},
    {"gt"s, '>'}, {"GT"s, '>'},
    {"amp"s, '&'}, {"AMP"s, '&'},
    {"apos"s, '\''}, {"APOS"s, '\''},
    {"quot"s, '"'}, {"QUOT"s, '"'}
};


std::string HtmlDecode(std::string_view str) {
    std::ostringstream os;

    size_t pos = str.size() == 0 ? std::string_view::npos : 0;
    while (pos != std::string_view::npos) {
        size_t amp = str.find_first_of('&', pos);

        os << str.substr(pos, amp - pos);
        if (amp != std::string_view::npos) {
            std::string mnemonic;
            size_t i = amp + 1;
            for (; i < str.size() && mnemonic.size() < 5; ++i) {
                if (HTML_MNEMONICS.contains(mnemonic)) {
                    if (i <= str.size() && str[i] == ';') {
                        ++i;
                    }
                    break;
                }
                
                if (str[i] == '&') {
                    os << str[amp] << mnemonic;
                    mnemonic.clear();
                    continue;
                }

                mnemonic.push_back(str[i]);
            }

            if (HTML_MNEMONICS.contains(mnemonic)) {
                os << HTML_MNEMONICS.at(mnemonic);
            } else {
                os << str[amp] << mnemonic;
            }
            pos = i;
        }

        if (amp == std::string_view::npos) {
            pos = amp;
        }
    }

    return os.str();
}
