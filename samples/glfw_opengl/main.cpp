/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Shared entry point for glfw_opengl samples. Each sample links this
 * and registers one demo via VNETESTBED_REGISTER_DEMO. The actual
 * run is in the library (runDemoApplication) so the main TU does not
 * need the complete Application type.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/demo_application.h"

int main(int argc, char** argv) {
    return vne::testbed::runDemoApplication(argc, argv);
}
