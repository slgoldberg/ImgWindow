# ImgWindow: ImGui wrapper for X-Plane Modern XPLM Window API with font support

The sources in this repository are shared with the greater X-Plane developer
community in the hope that it may save somebody a headache some day.

This repository ONLY contains the files needed for the `ImgWindow` class
and the `ImgFontAtlas` wrapper for binding to ImGui v1.84 through v1.92.x,
and has been updated to completely hide the core changes in ImGui v1.92.x
to provide simple migration from older versions of ImGui vis-a-vis fonts
and keyboard API changes in ImGui, requiring no code changes for either.

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

## Using this repository (as a "submodule" in your own project)

First of all, this repository _only_ contains the `ImgWindow` and the
`ImgFontAtlas` classes (headers and C++ implementations) and some ancillary
files including some fonts you can include in your sources and add to
your ImGui environment with ImgFontAtlas, and (in the near future) some
basic sample code.

This can be included as a "submodule" (see below) within your X-Plane plugin
sources, along with a submodule for the latest
[dear imgui](https://github.com/ocornut/imgui) sources.

This is _not_ a complete, "buildable" X-Plane sample plugin, though sample
code will be provided to assist with basic usage of the framework in a
subsequent update (hopefully coming soon).

For a fully usable example X-Plane plugin development project with
cross-compilation via a Docker Linux image and example CMake and Dockerfile
configurations, see [imgui4xp](https://github.com/sparker256/imgui4xp).

That project also demonstrates how to incorporate this repository as a
git submodule within your own project.  For example, let's say you want
to have `ImgWindow` within your own source code tree under a directory
at the top-level titled, "`third-party`".  If you already have a subfolder
containing an older version of `ImgWindow` called, "`ImgWindow`", you can
simply delete that from the project, and re-add it as a submodule, using
these example command lines (assuming you have the git cli installed):

```
% cd /path/to/project_sources
% git rm -rf third-party/ImgWindow             # (ONLY if it already existed!)
% git commit -m "Remove embedded ImgWindow code to replace with submodule"
```

To set up the "submodule" connection within your project:

```
% cd /path/to/project_sources
% git submodule add https://github.com/slgoldberg/ImgWindow third-party/ImgWindow
% git commit -m "Replace embedded ImgWindow from slgoldberg's fork"
```

This will do a **one-time** copy of this repository into your project tree,
however, you can of course update it at any time in standard git fashion by
going into the submodule directory itself, and updating, a la:

```
% cd /path/to/project_sources/third-party/ImgWindow
% git pull                                       # bring local copy up to date
% cd /path/to/project_sources
% git add third-party/ImgWindow
% git commit -m "Update embedded ImgWindow from slgoldberg's fork"
```

## Contributing to this project

Pull Requests are welcome, though it's usually better to start by contacting
the owner via PM to the [https://forums.x-plane.org](x-plane.org forum) (via
PM to `@slgoldberg` on that forum).
