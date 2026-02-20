/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>
#include "vertexnova/testbed/testbed.h"

TEST(VneTestbed, GetVersion) {
    const char* ver = vne::testbed_ns::get_version();
    ASSERT_NE(ver, nullptr);
    EXPECT_STRNE(ver, "");
}

TEST(VneTestbed, Hello) {
    const char* msg = vne::testbed_ns::hello();
    ASSERT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "Hello from VneTestbed");
}
