/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/demo_application.h"

int main(int argc, char** argv) {
    vne::testbed::ApplicationDescriptor desc{};
    desc.title = "VneTestbed — Demo";
    desc.vsync_enabled = true;

    const int exit_code = vne::testbed::runDemoApplication(argc, argv, &desc);
    return exit_code;
}
