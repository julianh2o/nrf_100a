#include <xc.inc>

    GLOBAL _populateLeds        ; make _add globally accessible
    SIGNAT _populateLeds,4217   ; tell the linker how it should be called

_populateLeds:

    RETURN