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
 * @file app/demo_application.h
 * @brief Thin Application subclass that installs the selected demo via DemoFactory.
 *
 * Selection order: env VNE_DEMO, compile-time VNE_DEMO_ID, else CreateDefault (e.g. first registered).
 * Use for samples: construct DemoApplication, initialize(descriptor), run(), shutdown().
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/demo_factory.h"

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
     * @return true if base init and demo install succeeded; false if base init failed or
     *         no demo could be installed (e.g. none registered or requested id not found).
     *         On false, shutdown() is called so the backend is cleaned up.
     */
    bool initialize(const ApplicationDescriptor& descriptor);
};

/**
 * @brief Run the registered demo app (constructs DemoApplication, init, run, shutdown).
 * Use this from the shared main so the main TU does not need the complete Application type.
 * @param argc  Pass-through from main (unused; demo id from VNE_DEMO env or VNE_DEMO_ID).
 * @param argv  Pass-through from main.
 * @param descriptor  If non-null, used for initialize(); otherwise a default descriptor is used.
 */
int runDemoApplication(int argc, char** argv, const ApplicationDescriptor* descriptor = nullptr);

}  // namespace testbed
}  // namespace vne
