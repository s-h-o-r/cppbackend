#include <boost/log/trivial.hpp>

namespace logger {

template <class RequestHandler>
class LoggingRequestHandler {
    static void LogRequest();
    static void LogResponse();

public:
    explicit LoggingRequestHandler(RequestHandler& handler)
        : decorated_(handler) {
    }

    

private:
    RequestHandler& decorated_;
};
} // namespace logger
