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

#include "vertexnova/testbed/logging_guard.h"

#if defined(VNE_TESTBED_LOGGING)
#include "vertexnova/logging/logging.h"

CREATE_VNE_LOGGER_CATEGORY("vnetestbed")
#endif

namespace vne {
namespace testbed {

#if defined(VNE_TESTBED_LOGGING)

LoggingGuard::LoggingGuard() {
    (void)VNE_LOGGER_CATEGORY;  // use category (macro may define it)
    vne::log::LoggerConfig config;
    config.name = vne::log::kDefaultLoggerName;
    config.sink = vne::log::LogSinkType::eConsole;
    config.console_pattern = "[%l] [%n] %v";
    config.log_level = vne::log::LogLevel::eInfo;
    config.async = false;
    vne::log::Logging::configureLogger(config);
}

LoggingGuard::~LoggingGuard() {
    vne::log::Logging::shutdown();
}

#else

LoggingGuard::LoggingGuard() = default;
LoggingGuard::~LoggingGuard() = default;

#endif

}  // namespace testbed
}  // namespace vne
