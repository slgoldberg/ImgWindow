# RFC: ImGee / ImgWindow TimedTooltip Font Scaling Architecture (v2.4)
**Authors:** Ser Maester Vector & Ser Steven (Review / Synthesis for Sirrogerio Il Sigundo)  
**Status:** Ready for Forge Review  
**Target:** `ImgWindow::contrib/extensions/TimedTooltip` (`imgui_tooltip_ext.h`)

---

## 1. Executive Summary & Why This Still Matters

The recent course correction in commit `627e4db` was a triumphant victory: by properly fixing internal widget padding (`pad_y = ImMax(2.0f * scale, 2.0f)`) and reporting true `ItemSize`, the container was freed from the asymmetric `+ 6.0f * scale` headroom hack, banishing the tooltip "chin" and restoring optical balance.

However, **font scale coupling physics** remains an open architectural issue whenever windows are zoomed or downscaled:

1. **The Downscaled Squeeze Box ($< 1.0\times$):**
   In dense cockpits, developers routinely scale child frames down to $0.70\times$–$0.75\times$ to cram annunciators, radio stacks, or CDU numeric tables into tight physical cutouts. 
   Because tooltips inherited `FontWindowScale` linearly $1:1$, the explanatory text shrank to 7–9px microprint. 
   **As Roger brilliantly observed: tooltips live in the open sky!** A floating overlay has zero obligation to suffer the physical layout constraints of the tiny button beneath it.
2. **The Billboard Balloon Effect ($> 1.5\times$):**
   When a pilot sits back from a large 4K display or zooms a panel to $1.8\times$–$2.0\times$, a linear $1:1$ tooltip balloons in both axes, consuming nearly $4\times$ the screen real estate and occluding half the windshield.
3. **The Developer Override Paradox (The ABC Experience):**
   In real-world avionics code (like *A-Better-Camera*), developers occasionally need explicit control (e.g. making a sub-label $0.9\times$, or an annunciator tooltip $1.2\times$). If an API provides only raw unconstrained power, developers can accidentally generate unreadable 3px micro-text or 5000px crashes. 

---

## 2. Mathematical Model: The Bilateral Damped Curve

Let $S_{\text{win}}$ be the calling window's `FontWindowScale`:

$$\text{BaseDampedScale}(S_{\text{win}}) = \begin{cases}
1.0\text{f} + 0.30\text{f} \times (S_{\text{win}} - 1.0\text{f}) & \text{if } S_{\text{win}} \le 1.0\text{f} \quad \text{\bf (Lower Damped Ease down to } 0.85\times\text{\bf )} \\
1.0\text{f} + 0.28\text{f} \times (S_{\text{win}} - 1.0\text{f}) & \text{if } S_{\text{win}} > 1.0\text{f} \quad \text{\bf (Upper Damped Growth up to } 1.30\times\text{\bf )}
\end{cases}$$

$$\text{DampedScale}(S_{\text{win}}) = \text{ImClamp}(\text{BaseDampedScale}(S_{\text{win}}),\, 0.85\text{f},\, 1.30\text{f})$$

### Behavior Across Practical Simulator Zoom Levels:

| Window Zoom ($S_{\text{win}}$) | Linear $1:1$ (Old) | Damped Model (v2.4) | Visual Experience |
| :---: | :---: | :---: | :--- |
| **$0.50\times$** (Extreme Sub-Panel) | $0.50\times$ ($7.0\text{px}$) | **$\mathbf{0.85\times}$** ($11.9\text{px}$) | **Protected & Compact:** Cushioned at $0.85\times$ floor; respects tiny window space without microprint. |
| **$0.70\times$** (Cramped Sub-Panel) | $0.70\times$ ($9.8\text{px}$) | **$\mathbf{0.91\times}$** ($12.7\text{px}$) | **Ergonomic:** Proportional, highly readable, minimizes cockpit occlusion. |
| **$0.85\times$** (Compact Stack) | $0.85\times$ ($11.9\text{px}$) | **$\mathbf{0.955\times}$** ($13.4\text{px}$) | **Snug:** Perfectly matches downscaled avionics intent. |
| **$1.00\times$** (Default UI) | $1.00\times$ ($14.0\text{px}$) | **$\mathbf{1.00\times}$** ($14.0\text{px}$) | Identical baseline. |
| **$1.50\times$** (Moderate Zoom) | $1.50\times$ ($21.0\text{px}$) | **$\mathbf{1.14\times}$** ($16.0\text{px}$) | Gentle, comfortable bump for readability without bloat. |
| **$2.00\times$** (4K / Distance Zoom) | $2.00\times$ ($28.0\text{px}$) | **$\mathbf{1.28\times}$** ($17.9\text{px}$) | Readable from distance without swallowing the cockpit. |

> [!NOTE]
> System/Monitor DPI scaling (`io.FontGlobalScale`) multiplies everything globally and is never touched or hindered by this local window-zoom damping.

---

## 3. Policy Selection (`TooltipFontScalePolicy`)

To preserve flexibility for downstream plugins and pure-chrome tools:

```cpp
enum TooltipFontScalePolicy
{
    TooltipFontScale_Damped = 0,    // Default: Open-Sky floor (1.0x) + upper dampening curve
    TooltipFontScale_FixedChrome,   // Strategy A: Strictly 1.0x baseline (monitor DPI only)
    TooltipFontScale_LinearInherit  // Legacy 1:1 coupling (SetWindowFontScale(S_win))
};
```

Developers can configure this globally on `ImgWindow`, or on a specific `TimedTooltip` instance:
```cpp
ImGui::TimedTooltip::SetDefaultFontScalePolicy(TooltipFontScale_Damped);
```

---

## 4. The Two-Channel Override Architecture (With Foot-Gun Guard Rails)

To address the tension between developer intent ("Listen to me!") and safety guard rails:

```cpp
struct TooltipFontScaleConfig
{
    TooltipFontScalePolicy Policy = TooltipFontScale_Damped;

    // Channel 1: Relative Multiplier (Semantic Fine-Tuning)
    // Multiplies the policy-derived scale (e.g. 0.9f for a secondary hint).
    // Automatically protected by the safety floor.
    float RelativeMultiplier = 1.0f;

    // Channel 2: Absolute Forced Override (The CSS !important escape hatch)
    // 0.0f = auto / inactive (delegates to Policy).
    // > 0.0f = forces explicit scale, bypassing window zoom and damping entirely.
    // Guarded by sanity clamps to prevent zero-size crashes or 5000px explosions.
    float AbsoluteOverride = 0.0f;
};
```

### 4.1 Backward-Compatible Integration into `TooltipConfig`
To guarantee 100% backward compatibility with all existing plugins, `TooltipConfig` in `imgui_tooltip_ext.h` integrates these channels directly:
```cpp
struct TooltipConfig {
    // ... existing padding and color fields ...
    TooltipFontScalePolicy font_scale_policy = TooltipFontScale_Damped; // Default policy
    float font_scale_multiplier = 1.0f;                                // Channel 1: Relative multiplier
    float font_scale = 0.0f;                                           // Channel 2: 0.0f = auto; >0.0f = AbsoluteOverride
    // ...
};
```
*Existing callers writing `config.font_scale = 1.25f;` continue to compile and function seamlessly as an emphatic Channel 2 override!*

### 4.2 The Final Scale Resolution Logic:

```cpp
float CalculateEffectiveTooltipFontScale(float window_scale, const TooltipFontScaleConfig& cfg)
{
    // Channel 2: Emphatic Absolute Override (CSS !important)
    if (cfg.AbsoluteOverride > 0.0f)
    {
        // Guard against crashes or runaway numbers while honoring developer intent:
        return ImClamp(cfg.AbsoluteOverride, 0.75f, 2.25f);
    }

    // Channel 1: Policy Resolution
    float base_scale = 1.0f;
    switch (cfg.Policy)
    {
        case TooltipFontScale_FixedChrome:
            base_scale = 1.0f;
            break;

        case TooltipFontScale_LinearInherit:
            base_scale = window_scale;
            break;

        case TooltipFontScale_Damped:
        default:
            if (window_scale <= 1.0f)
            {
                base_scale = ImClamp(1.0f + 0.30f * (window_scale - 1.0f), 0.85f, 1.0f);
            }
            else
            {
                base_scale = ImClamp(1.0f + 0.28f * (window_scale - 1.0f), 1.0f, 1.30f);
            }
            break;
    }

    // Apply Relative Multiplier with legibility protection
    float final_scale = base_scale * cfg.RelativeMultiplier;
    return ImMax(final_scale, 0.75f); // Guard against accidental microprint
}
```

---

## 5. Summary of Benefits

1. **Zero Layout Regressions:** Operates completely orthogonally to `imgui_badges.h` and `imgui_markdown_ext.h` symmetry.
2. **True Component Autonomy:** Child frames can be squished as tight as avionics designers need without degrading tooltip readability.
3. **Cockpit Visibility Preservation:** Large parent zooms no longer create windshield-obscuring billboard tooltips.
4. **Clean Developer API:** Developers get both semantic relative scaling (`RelativeMultiplier`) and an emphatic override (`AbsoluteOverride`), each shielded with guard rails so no one shoots themselves in the foot.
5. **Automatic Width & Wrap Bounds Synergy:** Because wrap limits (`target_w = default_wrap_width * scale`) derive from `scale`, the damped curve naturally prevents horizontal wrap boundaries from ballooning out of proportion across the cockpit when zooming.
