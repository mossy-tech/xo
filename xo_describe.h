#ifndef XO_DESCRIBE_H

#include "xo.h"
 
typedef int (*printer_type)(const char * formatter, ...)
    __attribute__((format(printf, 1, 2)));

int xo_describe(struct xo * xo,
        printer_type printer,
        int printer_check_error, int printer_nonzero_is_error);

#endif /* XO_DESCRIBE_H */
