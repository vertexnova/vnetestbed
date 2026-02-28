#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * RAII console logging for testbed samples and tests.
 * ----------------------------------------------------------------------
 */

namespace vne {
namespace testbed {

/**
 * @class LoggingGuard
 * @brief RAII guard for console logging in samples and tests.
 *
 * When vne::logging is available, configures the logging system with console
 * output in the constructor and shuts it down in the destructor. Use at the
 * start of main() in sample programs. When logging is not linked, a no-op.
 *
 * @code
 * int main() {
 *     vne::testbed::LoggingGuard logging_guard;
 *     // ...
 *     return 0;
 * }
 * @endcode
 */
class LoggingGuard {
   public:
    LoggingGuard();
    ~LoggingGuard();

    LoggingGuard(const LoggingGuard&) = delete;
    LoggingGuard& operator=(const LoggingGuard&) = delete;
};

}  // namespace testbed
}  // namespace vne
