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
submit any Pull Requests to slgoldberg on *this* repository to contribute
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
XPLM "panel graphics" API, which is coming soon to the X-Plane v12.4.4 SDK,
as of this writing.

## Prerequisites

Components in this library assume the availability of the X-Plane XPLM3 or
later SDK, and rely on the developer including these files within their own
build projects for any X-Plane plugins that use ImGui, which must also be
installed (no less than ImGui v1.84 WIP, and currently no more than ImGui
v1.92.x). Later versions may work fine, but no guarantees are made. :-)

## Components in this Repository

* `ImgWindow` and `ImgFontAtlas` - Wrappers for the
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

This is _not_ a complete, "buildable" X-Plane sample plugin, though some
limited sample code is provided in the `sample-code` directory, to assist
with basic configuration of a shared font atlas with custom-loaded fonts,
which are actually included in the `sample-code/fonts` directory as well.
In the future, we hope to also have examples of how to _use_ the 
`ImgWindow` class to create and manage windows that are powered by
`ImGui`.

## `imgui4xp`: A full sample plugin using `ImgWindow`

For a _much_ more robust project containing a full sample X-Plane plugin
with C++ sources to demonstrate exactly how to set up and use `ImgWindow`
and `ImGui` for the user interface, while also demonstrating how to set
up `Docker` for cross-compilation (via a sample `Dockerfile`) as well
as how to use `Cmake` (as well as other componentns you may need!), see
Bill Good's project, **[imgui4xp](https://github.com/sparker256/imgui4xp)**.

The [imgui4xp](https://github.com/sparker256/imgui4xp) project also
demonstrates how to incorporate `ImgWindow` using this repository as a
**git submodule** within your own project.

For example, let's say you want to have `ImgWindow` within your own
source code tree under a directory at the top-level titled,
"`third-party`".  If you already have a subfolder containing an older
version of `ImgWindow` called, "`ImgWindow`", you can simply delete
that from the project, and re-add it as a submodule, using these example
command lines (assuming you have the git cli installed):

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

This will do a **one-time** copy of this repository into your project tree.

However, **users** of your repository will need to pull the sources into that
submodule directory once they clone or fork it locally, like this:

```
% cd /path/to/cloned_project        # i.e., home folder containing .gitmodules
% git submodule update --init --recursive
```

Regardless, since any submodule pointing to this repository is itself just a
local git repository tree, the submodule's local folder -- even if otherwise
empty when you first clone the containing project -- will _always_ contain a
`.git` folder at the top level of the containing project's local tree. Since
it's just another git tree embedded within the containing tree, you can simply
use git tools to update it any time, for example to fetch any changes (though
they won't actually be included by default when cloned -- see above!).  For
example, if you own the containing project with `ImgWindow` as a submodule:

```
% cd /path/to/project_sources/third-party/ImgWindow
% git pull                                       # bring local copy up to date
% cd /path/to/project_sources
% git add third-party/ImgWindow
% git commit -m "Update embedded ImgWindow from slgoldberg's fork"
```

## Contributing to this project

Pull Requests (PRs) are welcome, though it's usually better to start by
contacting the owner (Steve) via private message (PM) to the
[X-Plane.org forum](https://forums.x-plane.org).  Send a private message
to `@slgoldberg` on that forum, and introduce yourself and explain what
you're hoping to accomplish.  (This is important because Steve may not
see the GitHub-based notifications as quickly.  If you are not a user of
that forum, then feel free to choose a different way to contact Steve to
start the collaboration -- worst case, by simply submitting the PR as it
is).

In general, to contribute code here, the best place to start is by "`fork`ing"
this `ImgWindow` repository, which lets you make changes locally which makes
it trivial for owner(s) of the forked repository to see your proposed changes
even before you submit a formal Pull Request (PR).

### Forking, testing, and managing changes in advance of a Pull Request:

To create your own "`fork`" of `ImgWindow` to start the process, simply
click the "**Fork**" button on the main [web page](https://github.com/slgoldberg/ImgWindow)
for [`ImgWindow` at github.com](https://github.com/slgoldberg/ImgWindow),
then use your favorite method to clone that locally so you can begin
making changes to your copy of the framework (e.g. via "git clone").

Once you have your local fork, simply do all your work *within* that local
fork.  There are two paths forward from here:

1. Standalone changes.

    This isn't as common, but if you *just* want to make minor changes such as
    fix typos or add a line or two -- and don't need a full multiple-pass test
    regime to verify your changes before you file your Pull Request, then just
    make all your changes in place, and push those back to your `master` branch
    on the upstream repository (i.e., on GitHub), so that we can see the changes
    (again -- even if you haven't sent the PR yet).
  
    If the PR is accepted, you can simply do a "git pull" in your own fork to
    see the final result with your changes already integrated, and you're set up
    to come back here and do more similar work later if you need to.

2. Testing and iterating on changes in context with your plugin using `ImgWindow`.

    This builds on 1. above, in a way that is _really_ simple to set up, and will
    save you tons of time -- assuming your plugin's source code defines `ImgWindow`
    as a "`submodule`".  If not, then there's no point to this part. :-)
  
    Note: these instructions _assume_ you've already done the bit above to make
    `ImgWindow` be a "submodule", so the actual code for `ImgWindow` is not part
    of your repository. (This is right; but read on for how to manage modifying
    both the `ImgWindow` sources within your own local fork, and testing changes
    you make to it in your plugin source project.)
  
    First, you need to change your plugin's source project by **renaming** your
    `ImgWindow` directory so that it can readily be "redirected" to different
    targets -- be that the official "submodule" straight from GitHub, or the
    local version you might be iterating on while making changes to `ImgWindow`:
  
    ```
    % cd /path/to/plugin_project
    % git mv third-party/ImgWindow third-party/ImgWindow_GITHUB
    % ln -s third-party/ImgWindow_GITHUB third-party/ImgWindow
    % git add third-party/ImgWindow    # add the symbolic link as a tracked file
    % git commit -m "Add layer of indirection using symbolic link to ImgWindow"
    ```
  
    This assumes you put `ImgWindow` inside a `third-party` directory, per the
    example earlier for adding it as a submodule.  This basically changes the
    `ImgWindow` path to the submodule sources from being a directory, to being
    a symbolic link _to_ that directory.  (Note: Windows PowerShell or other
    syntax can be inferred from this; the same type of symbolic links exist
    in Windows as well, so this will work regardless of platform as long as
    you use the right command to create the symbolic link as described.)
  
    From this point, you can more readily change which version of `ImgWindow`
    you build with (`make clean` issues aside!), by simply changing that
    symbolic link, rather than changing the rest.
  
    **Note**: This setup leaves you with needing to be sure you switch to
    the "production" version of ImgWindow when you're ready to commit and
    push everything back to the master repository.  You are more than
    welcome to wholesale copy all but the `.git` folder in a given instance
    of `ImgWindow` to keep it for specific builds, i.e. if you want to be
    sure the build works, as opposed to having to worry that there may be
    some breaking change that gets synced from the `ImgWindow` repository
    at a later date when you're not ready. But this is a separate topic.
  
    Finally, for testing, you start by making changes to your `ImgWindow`
    clone project, within *its* source tree, i.e. not from within your
    plugin's code.  And then you update your `third-party/ImgWindow`
    symbolic link to point to *this* directory where you're working on
    your fork -- in other words:
  
    ```
    % cd /path/to/plugin_project
    % rm third-party/ImgWindow   # remove the symbolic link so we can re-link it
    % ln -s /path/to/ImgWindow_fork third-party/ImgWindow  # point to local tree
    ```
  
    Of course, you would _not_ do a `git add` or `git commit` on _this_ change
    because in theory this is only something you do while testing.  Though of
    course it's up to you. :-)
    
    #### Finally: change -> test -> iterate.
    
    Once you're done with all the symlinking shenanigans above, you are set up
    to start actually modifying code and testing it out.
    
    You just have to remember that you're working in two different source
    trees in the local filesystem as you're iterating on this.  So, after you
    make a change, without needing to involve `git` at _all_ in the process
    (for this part) you just save your changes locally in the `ImgWindow`
    fork's local files, and then you test those changes by switching over to
    your **plugin**'s project repository and build and run it in X-Plane!
    
    Easy, peasy. :-)
    
    Once you have a final, working set of changes to `ImgWindow` (which by
    definition will be inside your local fork), the next step before you
    submit the PR is to just make sure the remote repository for your fork
    is up to date _vis-a-vis_ your changes!  So, on your local fork:
    
    ```
    % cd /path/to/ImgWindow_fork
    % git add ...       # (whatever files you modified that need to be staged)
    % git commit        # give clear descriptions since the PR will show these
    % git push           # pushes it back to your forked repository on GitHub!
    ```
    
    Remember, the `git push` just above doesn't send it to us or anything; it
    just refreshes your forked repository on GitHub based on your changes
    as they were already implemented (it doesn't imply anything about whether
    it is being merged into the slgoldberg fork).  That would be a separate
    action, to follow.
    
    But, having this repository up to date and visible publicly, we will then
    be able to readily peruse your changes *first*, before you send a PR, if
    you alert us to this fact.  We can have an out of band discussion, and
    if needed, you can still send a PR as a formality. :-)

---

### Errata / Missing Info?

If anything is wrong or missing from this README, please either fix it and
send us a PR, or let us know.

This file was last updated on _17-August-2026_ by Steve.
