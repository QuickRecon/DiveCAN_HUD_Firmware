#include "printer.h"
#include <cstdarg>

/* Mock serial_printf - does nothing for tests */
extern "C" {
void serial_printf(const char *format, ...) {
    (void)format;
    /* Stub - do nothing in tests */
}
}
