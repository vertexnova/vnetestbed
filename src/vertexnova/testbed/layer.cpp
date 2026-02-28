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

#include "vertexnova/testbed/layer.h"

namespace vne {
namespace testbed {

ILayer::ILayer(const std::string& name)
    : name_{name} {}

void ILayer::setEnabled(bool enabled) {
    if (is_enabled_ != enabled) {
        is_enabled_ = enabled;
        if (is_enabled_) {
            onEnable();
        } else {
            onDisable();
        }
    }
}

}  // namespace testbed
}  // namespace vne
