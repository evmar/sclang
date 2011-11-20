Given e.g. `/usr/include/cairo/cairo.h` as input, this produces
s-expressions like:

    (typedef cairo_t)
    (func uint cairo_get_reference_count (cairo_t* cr))
    (enum _cairo_fill_rule CAIRO_FILL_RULE_WINDING CAIRO_FILL_RULE_EVEN_ODD)
