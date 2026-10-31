#include "gl_engine/renderer/gl_handle.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace gl_engine::renderer {
namespace {

// Records deletions instead of calling GL, so the handle is tested without a context.
struct CountingTag {
    static std::vector<GLuint>& Deleted() {
        static std::vector<GLuint> deleted;
        return deleted;
    }
    static void Delete(GLuint id) {
        Deleted().push_back(id);
    }
};
using TestHandle = GlHandle<CountingTag>;

class GlHandleTest : public ::testing::Test {
  protected:
    void SetUp() override {
        CountingTag::Deleted().clear();
    }
};

TEST_F(GlHandleTest, DeletesOnDestruction) {
    {
        const TestHandle handle{7};
    }
    EXPECT_EQ(CountingTag::Deleted(), std::vector<GLuint>{7});
}

TEST_F(GlHandleTest, EmptyHandleDeletesNothing) {
    {
        const TestHandle handle;
    }
    EXPECT_TRUE(CountingTag::Deleted().empty());
}

TEST_F(GlHandleTest, MoveConstructionTransfersOwnership) {
    {
        TestHandle source{3};
        const TestHandle target{std::move(source)};
        EXPECT_EQ(target.Get(), 3U);
        EXPECT_FALSE(source);  // NOLINT(bugprone-use-after-move): the moved-from state is the test
    }
    EXPECT_EQ(CountingTag::Deleted(), std::vector<GLuint>{3});
}

TEST_F(GlHandleTest, MoveAssignmentDeletesThePreviousObject) {
    TestHandle target{1};
    TestHandle source{2};
    target = std::move(source);
    EXPECT_EQ(CountingTag::Deleted(), std::vector<GLuint>{1});
    EXPECT_EQ(target.Get(), 2U);
}

TEST_F(GlHandleTest, ResetDeletesOnce) {
    TestHandle handle{5};
    handle.Reset();
    handle.Reset();
    EXPECT_FALSE(handle);
    EXPECT_EQ(CountingTag::Deleted(), std::vector<GLuint>{5});
}

}  // namespace
}  // namespace gl_engine::renderer
