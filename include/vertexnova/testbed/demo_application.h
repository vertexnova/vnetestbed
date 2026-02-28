#pragma once
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

/**
 * @file demo_application.h
 * @brief Thin Application subclass that installs the selected demo via DemoFactory.
 *
 * Selection order: env VNE_DEMO, compile-time VNE_DEMO_ID, else CreateDefault (e.g. first registered).
 * Use for samples: construct DemoApplication, initialize(descriptor), run(), shutdown().
 */

#include "vertexnova/testbed/application.h"
#include "vertexnova/testbed/application_descriptor.h"
#include "vertexnova/testbed/demo_factory.h"

#include <cstdlib>

namespace vne {
namespace testbed {

/**
 * @class DemoApplication
 * @brief Application that installs one demo from DemoFactory after base initialize().
 */
class DemoApplication : public Application {
   public:
    DemoApplication() = default;

    /**
     * @brief Initialize base (window + renderer), then install the demo via DemoFactory.
     * Demo selection: VNE_DEMO env, VNE_DEMO_ID define, else CreateDefault().
     * @return true if base init and demo install succeeded.
     */
    bool initialize(const ApplicationDescriptor& descriptor) {
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
        DemoFactory::createDefault(*this);
        return true;
    }

   private:
    static const char* getDemoId() {
#ifdef VNE_DEMO_ID
        return VNE_STRINGIFY(VNE_DEMO_ID);
#else
        const char* env = std::getenv("VNE_DEMO");
        return env ? env : "";
#endif
    }
};

#ifndef VNE_STRINGIFY
#define VNE_STRINGIFY_IMPL(x) #x
#define VNE_STRINGIFY(x) VNE_STRINGIFY_IMPL(x)
#endif

}  // namespace testbed
}  // namespace vne
