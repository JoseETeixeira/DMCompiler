#include "Token.h"
#include <sstream>

namespace DMCompiler {

std::string Token::ToString() const {
    std::ostringstream oss;
    oss << "Token(" << static_cast<int>(Type) << ", \"" << Text << "\")";
    return oss.str();
}

} // namespace DMCompiler
