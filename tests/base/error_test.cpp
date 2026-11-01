#include "gl_engine/base/error.h"

#include <gtest/gtest.h>

#include <string>

namespace gl_engine::base {
namespace {

Result<int> ParsePositive(int value) {
    if (value <= 0) {
        return std::unexpected(Error{.code = ErrorCode::kParse, .message = "not positive"});
    }
    return value;
}

TEST(Error, RecordsWhereItWasBuilt) {
    const int line = __LINE__ + 1;
    const Error error{.code = ErrorCode::kIo, .message = "missing"};
    EXPECT_EQ(error.where.line(), static_cast<unsigned>(line));
    EXPECT_NE(std::string{error.where.file_name()}.find("error_test.cpp"), std::string::npos);
}

TEST(Error, ToStringNamesCodeMessageAndFile) {
    const Error error{.code = ErrorCode::kUnsupported, .message = "GL 3.3"};
    const std::string text = ToString(error);
    EXPECT_TRUE(text.starts_with("unsupported: GL 3.3 (error_test.cpp:")) << text;
}

TEST(Error, ResultCarriesValueOrError) {
    EXPECT_EQ(ParsePositive(3).value(), 3);
    const Result<int> failed = ParsePositive(-1);
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error().code, ErrorCode::kParse);
}

}  // namespace
}  // namespace gl_engine::base
