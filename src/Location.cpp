#include "Location.h"
#include <sstream>

namespace DMCompiler {

Location Location::Internal("internal", 0, 0);

std::string Location::ToString() const {
    std::ostringstream oss;
    oss << SourceFile << ":" << Line << ":" << Column;
    return oss.str();
}

} // namespace DMCompiler
