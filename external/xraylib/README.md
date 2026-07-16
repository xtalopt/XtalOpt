Minimal vendored subset of xraylib for XtalOpt XRD support.

This subset is derived from xraylib 4.2.1 and contains only the code and
precomputed data needed for these two functions:
- `FF_Rayl`
- `MomentTransf`

Included source files:
- `scattering.c`
- `splint.c`
- `xraylib-error.c`
- `xraylib-aux.c`
- `xrayglob_rayl.c` (generated subset of upstream `xrayglob_inline.c`)

This is intentionally not a full upstream xraylib checkout.
