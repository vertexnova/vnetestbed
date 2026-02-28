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

#include "vertexnova/testbed/app/demo_application.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/demo_factory.h"

#include <cstdlib>

namespace vne {
namespace testbed {

namespace {

const char* getDemoId() {
#ifdef VNE_DEMO_ID
    return VNE_DEMO_ID;  // CMake passes -DVNE_DEMO_ID="triangle" etc.
#else
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // getenv is deprecated; we only read, no _dupenv_s needed
#endif
    const char* env = std::getenv("VNE_DEMO");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    return env ? env : "";
#endif
}

}  // namespace

bool DemoApplication::initialize(const ApplicationDescriptor& descriptor) {
    if (!Application::initialize(descriptor)) {
        return false;
    }
    const char* id = getDemoId();
    if (id && id[0] != '\0') {
        if (DemoFactory::createDemo(*this, id)) {
            return true;
        }
    }
    if (DemoFactory::createDemo(*this)) {
        return true;
    }
    if (DemoFactory::createDefault(*this)) {
        return true;
    }
    shutdown();
    return false;
}

}  // namespace testbed
}  // namespace vne
