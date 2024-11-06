#include <stdexcept>
#include <string>

namespace suplog
{
    class LogException : public std::runtime_error
    {
    public:
        LogException(const std::string &msg = "") : runtime_error(msg) {}
    };
}