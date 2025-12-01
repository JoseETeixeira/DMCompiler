#pragma once

namespace DMCompiler {

namespace Limits {
    /// Maximum iterations without advancing token position before breaking out of loop
    constexpr int MAX_NO_PROGRESS_ITERATIONS = 1000;
    
    /// Maximum nesting depth for recursive expression parsing
    constexpr int MAX_NESTING_DEPTH = 1000;

    /// Maximum number of tokens to process before aborting (10 million)
    constexpr int MAX_TOKENS = 10000000;

    /// Maximum length of a string literal (1MB)
    constexpr int MAX_STRING_LENGTH = 1024 * 1024;

    /// Maximum length of an identifier (1000 chars)
    constexpr int MAX_IDENTIFIER_LENGTH = 1000;
}

} // namespace DMCompiler
