# ImgWindow Contrib Ecosystem

Welcome to the `contrib/` directory of **ImgWindow**. This space hosts community-contributed extensions, widgets, and utility packages that extend Dear ImGui within X-Plane plugins that rely on the **ImgWindow** framework to host and render their user interfaces.

---

## Ecosystem Organization

To keep contributions modular and maintainable, the ecosystem is organized into two primary categories:

```
contrib/
├── extensions/          # Heavyweight, feature-complete packages
│   ├── Markdown/        # Markdown parser & renderer with dynamic font scaling
│   └── TimedTooltip/    # Timed tooltips engine with auto-bounds clamping
│
└── widgets/             # Lightweight, reusable UI controls
    ├── imgui_extra_widgets.h   # Master roll-up header (Opt-Out by default)
    ├── toggle_button.h         # Animated toggle switches & segmented button pairs
    ├── wheel_slider.h          # Sliders & controls with mouse-wheel interaction
    └── clickable_link.h        # ButtonLink, TextLink & hand-cursor affordances
```

### 1. Packages & Extensions (`contrib/extensions/`)
* **What they are:** Complex, feature-rich additions that maintain their own internal state, use configuration structs, or integrate external libraries.
* **Namespacing:** Strictly scoped within a dedicated sub-namespace (`ImGui::<PackageName>::`, e.g. `ImGui::TimedTooltip::Text(...)`) to avoid polluting the global ImGui namespace.
* **Inclusion:** Included a-la-carte as needed.
* *See &rarr; [contrib/extensions/README.md](extensions/README.md) for full guidelines.*

### 2. Reusable Widgets (`contrib/widgets/`)
* **What they are:** Self-contained, immediate-mode UI controls that feel like native Dear ImGui controls (e.g. toggle buttons, wheel-responsive sliders, clickable link affordances).
* **Namespacing:** Injected directly into the top-level `ImGui::` namespace (`ImGui::ToggleButton(...)`, `ImGui::AdjustOnItemMouseWheel(...)`).
* **Inclusion Strategy:**
  * **A-la-carte:** Include only what you need (`#include "contrib/widgets/wheel_slider.h"`).
  * **Master Roll-Up:** Include everything via `#include "contrib/widgets/imgui_extra_widgets.h"`.
  * **Opt-Out Guards:** Every widget is wrapped in an `#ifndef IMGUI_DISABLE_EXTRA_<WIDGET>` guard so you can disable specific widgets if naming collisions occur.
* *See &rarr; [contrib/widgets/README.md](widgets/README.md) for full guidelines.*

---

## How to Contribute

We welcome community contributions! Whether porting an existing open-source widget or introducing a new UI component:

1. **Choose the Category:** Determine if your code is a standalone **Widget** (a single control in `widgets/`) or a full **Extension** (a package in `extensions/`).
2. **Namespace Appropriately:** Follow the namespacing rules above.
3. **Keep Dependencies Minimal:** Header-only or clean single-file implementations are strongly preferred.
4. **Document Your Work:** Include a dedicated `README.md` in your extension folder detailing usage, API signatures, and code examples.
5. **Open a PR:** Submit a Pull Request against the repository with a description and sample screenshot or code snippet.
