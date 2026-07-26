# ImgWindow: ImGui wrapper for X-Plane Modern XPLM Window API with font support

The sources in this repository are shared with the greater X-Plane developer
community in the hope that it may save somebody a headache some day.

This was originally the public XSquawkBox Public (xsb_public) repository,
which contained several components, including ImgWindow and its dependencies.

This is the new home, forked from Chris Collins' original repository by
Steve Goldberg (slgoldberg), and focused down to just ImgWindow and
ImgFontAtlas, essentially.

If users need the other public sources from xsb_public, please see the
original repository from which this was forked.  Going forward, please
submit any Push Requests to slgoldberg on *this* repository to contribute
improvements back for ImgWindow or ImgFontAtlas.  The original repository
is effectively locked down as read-only, though Chris Collins does not
appear to have officially made that change. Perhaps this fork will cause
that to happen. :-)


## Licensing Note

New sources are released under the BSD 3-Clause license, following on to
the exact license terms provided for the ImgWindow and related sources
carried forward herein.

There are no other licensed dependencies included, as this repository is
solely focused on providing developers a way to use ImgWindow to bring
ImGui to XPLM Modern Windows -- and, possibly in the future, to the new
XPLM "panel graphics" API, which is coming soon to the X-Plane v12.5 SDK
as of this writing.

## Prerequisites

Components in this library assume the availability of the X-Plane XPLM3 or
later SDK, and rely on the developer including these files within their own
build projects for any X-Plane plugins that use ImGui, which must also be
installed (no less than ImGui v1.84 WIP, and currently no more than ImGui
v1.92.x). Later versions may work fine, but no guarantees are made. :-)

## Components in this Repository

* `ImgWindow` and `ImgFontAtlas` - Wrapper for the
  [dear imgui](https://github.com/ocornut/imgui) Immediate Mode GUI library

## No longer supported (see original repository):
  * `WavFile` - Simple PCM Wavfile loader as used by XSB 1.4 onwards.
  * `XOGLUtils` - an old OpenGL2 binding library for libxplanemp1.
  * `XSquawkBox` support - these libraries were needed for it, but it's
    not the goal of this `ImgWindow`-focused project to support `XSquawkBox`
    anymore, as this is forked out from that repository to greatly simplify
    this repository.
