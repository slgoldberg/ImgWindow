// ---------------------------------------------------------------------------
//  InitializeImGui.cpp
//
//  Steven L. Goldberg, 2026
//
//  This is an example of how to initialize ImGui for use in an X-Plane
//  plugin using the ImgWindow and ImgFontAtlas classes.  It is not a complete
//  example, but it shows how to set up the font atlas and load fonts into it,
//  including merging FontAwesome icons into each font, with offsets where we
//  may want to tweak the baseline to match the text in various combinations.
//
//  This is just one example, taken from the A-Better-Camera source code, just
//  to show how fonts can be loaded into a common font atlas shared across all
//  windows using ImgWindow.  Other sample code may be available as well, to
//  show how to use ImgWindow in general -- however, this is just to show the
//  initialization that most plugins will want and need to get things going.
//
//  tl/dr:
//  The InitializeImGui() function below is meant to be global, called once at
//  plugin startup, and it will set up the shared font atlas for all windows to
//  use.  It will also load the Roboto font and merge FontAwesome icons into
//  it, so that any window can use them via ImGui::PushFont() with the
//  appropriate font index.
//
//  For full compatibility with XPLMReloadPlugins() or XPLMReloadThisPlugin(),
//  the font atlas can be clealy destroyed and reset using RemoveImGui() which
//  is at the end of this file, below.  (This will even allow you to
//  dynamically clear and re-build the Font Atlas if you want to completely
//  change fonts -- e.g., if you want to allow the user to select a different
//  base font size, or a different font family, etc.  (Of course, this sample
//  InitializeImGui() function doesn't handle all that -- it assumes a fixed
//  set of fonts and default font size, around which it loads the combined
//  fonts at the base size, with FontAwesome icons merged in.)
//
//  Your mileage may vary. :-) This is just an example; no promises are made.
//  (For a complete example of how to use ImGui in an X-Plane plugin with
//  ImgWindow and ImgFontAtlas, see the GitHub repository for "imgui4xp", at
//  <https://github.com/sparker256/imgui4xp>.
//
//  IMPORTANT: If you start with the above "imgui4xp" project, ImgWindow is
//  required for that project to build -- however, since ImgWindow is defined
//  as a "submodule" within "imgui4xp", you will need to use the following
//  command *ONCE* immediately after cloning that repository locally:
//  % git submodule update --init --recursive
//
//  This will ensure you have the dependencies for "imgui4xp" loaded, which
//  will include the latest version of ImgWindow as well as this sample code
//  and the provided font files.
// ---------------------------------------------------------------------------

#include "ImgWindow.h"   /* won't correctly load in-situ (just sample code) */

// Fonts downloaded and converted in the "fonts" directory are normally 14px,
// with "senior" versions at baseline 16px:
constexpr int SYSTEM_FONT_SIZE = 14;     // roughly equivalent size for Roboto

#define BASELINE_FONT_SIZE    ( SYSTEM_FONT_SIZE )

// Load fonts from files generated via ImGui utility which converts TTF to
// compressed C++ arrays, included in the "fonts" directory as .inc files:
// (NOTE: all these fonts are explicitly loaded into the font atlas, below.)
#include "fonts/fa-solid-900.inc"                      /* FontAwesome icons */
#include "fonts/gf-roboto-regular-400.inc"
#include "fonts/gf-roboto-bold.inc"
#include "fonts/gf-robotomono-regular-400.inc"
#include "fonts/gf-roboto-italic-400.inc"
#include "fonts/gf-robotomono-regular.inc"                    /* 500 medium */

bool InitializeImGui ()
{
    // Create one (and exactly one) font atlas, so all fonts are shared across
    // all ImGui contexts (i.e., across all instances of ImgWindow and its
    // derived classes):
    if (!ImgWindow::sFontAtlas)
        ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();
    
    ImFontConfig mainConfig;             // reused for each font we load below
    
    // ------------- BUILD UP FONT ARRAY for ImGui::PushFont(): --------------

// -------------- Fonts[0 = IM_NORMAL_FONT]: ------------------------------

    // From Google Fonts as "gf_roboto_regular_400_compressed_{size,data}":
    strncpy(mainConfig.Name, "Roboto-Regular-400-14px", 40);
    mainConfig.GlyphOffset = { 0.f, 0.f };   // keep descenders above baseline
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (gf_roboto_regular_400_compressed_data,
         gf_roboto_regular_400_compressed_size,
         BASELINE_FONT_SIZE, &mainConfig);
    
#define FA_FONT_SIZE(x)    (x - 2) /* load FontAwesome at 2 px smaller size */
    
    // Now we merge some icons from the OpenFontsIcons font into the above
    // font (see `imgui/docs/FONTS.txt` for details about setting these up):
    ImFontConfig mergeConfig; // reused as overlay within each Font slot below
    mergeConfig.MergeMode = true;  // (NOTE: GlyphOffset is relative to above)
//  mergeConfig.GlyphOffset = { 0.f, 0.25f };  // tweak baseline to match text
    
    // We only read very selectively the individual glyphs we are actually
    // using to save on texture RAM impact:
    static ImVector<ImWchar> icon_ranges;
    ImFontGlyphRangesBuilder builder;
    // Add all icons that are actually used (they concatenate into one string
    // a la sprites):
    // (NOTE: these are just the ones A-Better-Camera uses; you can add or
    // remove any of the ones you want included in every font as needed!
    // When a specific icon is not listed here but you try to use it in your
    // ImGui code, it will just show up as a "?" or a blank square box; just
    // listing such icons here will make them show up in your plugin's UI!)
    // MARK: - - - - >   F A   I C O N S    < - - - -
    builder.AddText(ICON_FA_ALIGN_CENTER
                    ICON_FA_ANGLE_DOUBLE_DOWN
                    ICON_FA_ANGLE_DOUBLE_LEFT
                    ICON_FA_ANGLE_DOUBLE_RIGHT
                    ICON_FA_ANGLE_DOUBLE_UP
                    ICON_FA_ANGLE_DOWN
                    ICON_FA_ANGLE_LEFT
                    ICON_FA_ANGLE_RIGHT
                    ICON_FA_ANGLE_UP
                    ICON_FA_ARROW_CIRCLE_DOWN
                    ICON_FA_ARROW_CIRCLE_LEFT
                    ICON_FA_ARROW_CIRCLE_RIGHT
                    ICON_FA_ARROW_CIRCLE_UP
                    ICON_FA_ARROW_DOWN
                    ICON_FA_ARROW_LEFT
                    ICON_FA_ARROW_RIGHT
                    ICON_FA_ARROW_UP
                    ICON_FA_ARROWS_ALT_H
                    ICON_FA_ARROWS_ALT_V
                    ICON_FA_ASTERISK
                    ICON_FA_CAMERA
                    ICON_FA_CARET_DOWN
                    ICON_FA_CARET_LEFT
                    ICON_FA_CARET_RIGHT
                    ICON_FA_CARET_SQUARE_DOWN
                    ICON_FA_CARET_SQUARE_LEFT
                    ICON_FA_CARET_SQUARE_RIGHT
                    ICON_FA_CARET_SQUARE_UP
                    ICON_FA_CARET_UP
                    ICON_FA_CART_ARROW_DOWN
                    ICON_FA_CHECK
                    ICON_FA_CHECK_CIRCLE
                    ICON_FA_CHECK_SQUARE
                    ICON_FA_CHEVRON_CIRCLE_LEFT
                    ICON_FA_CHEVRON_CIRCLE_RIGHT
                    ICON_FA_CHEVRON_DOWN
                    ICON_FA_CHEVRON_LEFT
                    ICON_FA_CHEVRON_RIGHT
                    ICON_FA_CHEVRON_UP
                    ICON_FA_CLIPBOARD_LIST
                    ICON_FA_CLOCK
                    ICON_FA_COG
                    ICON_FA_CROSS
                    ICON_FA_DHARMACHAKRA
                    ICON_FA_DIAGNOSES
                    ICON_FA_EDIT
                    ICON_FA_ELLIPSIS_H
                    ICON_FA_ELLIPSIS_V
                    ICON_FA_EXCLAMATION
                    ICON_FA_EXCLAMATION_CIRCLE
                    ICON_FA_EXCLAMATION_TRIANGLE
                    ICON_FA_EYE
                    ICON_FA_EXTERNAL_LINK_SQUARE_ALT
                    ICON_FA_FAST_FORWARD
                    ICON_FA_FAST_BACKWARD
                    ICON_FA_FILE
                    ICON_FA_FILE_ALT
                    ICON_FA_FROWN
                    ICON_FA_FROWN_OPEN
                    ICON_FA_FOLDER_OPEN
                    ICON_FA_I_CURSOR
                    ICON_FA_ICONS
                    ICON_FA_INFO
                    ICON_FA_INFO_CIRCLE
                    ICON_FA_HAND_POINT_LEFT
                    ICON_FA_HAND_POINT_RIGHT
                    ICON_FA_LEVEL_UP_ALT
                    ICON_FA_LIGHTBULB
                    ICON_FA_LINK
                    ICON_FA_LIST
                    ICON_FA_LIST_ALT
                    ICON_FA_LIST_OL
                    ICON_FA_LIST_UL
                    ICON_FA_LOCATION_ARROW
                    ICON_FA_LOCK
                    ICON_FA_LOCK_OPEN
                    ICON_FA_LONG_ARROW_ALT_DOWN
                    ICON_FA_LONG_ARROW_ALT_LEFT
                    ICON_FA_LONG_ARROW_ALT_RIGHT
                    ICON_FA_LONG_ARROW_ALT_UP
                    ICON_FA_MAP_MARKER_ALT
                    ICON_FA_MINUS
                    ICON_FA_MINUS_SQUARE
                    ICON_FA_PAUSE_CIRCLE
                    ICON_FA_PLANE
                    ICON_FA_PLAY
                    ICON_FA_PLUS
                    ICON_FA_PLUS_SQUARE
                    ICON_FA_QUESTION
                    ICON_FA_QUESTION_CIRCLE
                    ICON_FA_REMOVE_FORMAT
                    ICON_FA_SAVE
                    ICON_FA_SEARCH
                    ICON_FA_SIGN_IN_ALT
                    ICON_FA_SIGN_OUT_ALT
                    ICON_FA_SLIDERS_H
                    ICON_FA_SMILE
                    ICON_FA_SMILE_BEAM
                    ICON_FA_SMILE_WINK
                    ICON_FA_SORT
                    ICON_FA_SQUARE
                    ICON_FA_SPINNER
                    ICON_FA_STEP_BACKWARD
                    ICON_FA_REDO
                    ICON_FA_ROAD
                    ICON_FA_STAR
                    ICON_FA_TACHOMETER_ALT
                    ICON_FA_TASKS
                    ICON_FA_TEXT_WIDTH
                    ICON_FA_THUMBS_UP
                    ICON_FA_TIMES
                    ICON_FA_TIMES_CIRCLE
                    ICON_FA_TOGGLE_OFF
                    ICON_FA_TOGGLE_ON
                    ICON_FA_TOOLS
                    ICON_FA_TRADEMARK
                    ICON_FA_TRASH
                    ICON_FA_TRASH_ALT
                    ICON_FA_USER_CLOCK
                    ICON_FA_UNDO
                    ICON_FA_UNDO_ALT
                    ICON_FA_UPLOAD
                    ICON_FA_USER_EDIT
                    ICON_FA_WINDOW_CLOSE
                    ICON_FA_WINDOW_MAXIMIZE
                    ICON_FA_WINDOW_RESTORE);
    builder.BuildRanges(&icon_ranges);
    
    // Merge the icon font with the text font:
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
         FA_FONT_SIZE(BASELINE_FONT_SIZE), &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[1 = IMG_TITLE_FONT]: ------------------------------

    // Add second, larger font for titles:
    // (For example, A-Better-Camera uses this for the main window title, and
    // for the ImGui extension(s) it defines for certain headings/titles.)
    int titleFontSize = BASELINE_FONT_SIZE + 3;    // titles are <default>+3px
    strncpy(mainConfig.Name, "Roboto-Regular-400-17px*", 40);
//  mainConfig.GlyphOffset = { 0.f, -0.5f };   // hand-tweaked vertical offset
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (gf_roboto_regular_400_compressed_data,
         gf_roboto_regular_400_compressed_size,
         titleFontSize, &mainConfig);
    
    mergeConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
                                          (fa_solid_900_compressed_data,
                                           fa_solid_900_compressed_size,
                                           FA_FONT_SIZE(titleFontSize),
                                           &mergeConfig,
                                           icon_ranges.Data);
    
// -------------- Fonts[2 = IM_SMALLER_FONT]: ---------------------------------
    
    // Add a third font that's smaller (yes, we could scale these, but we want
    // to be able to mix and match them, and this is the fastest way, though
    // it takes slightly longer to start up, and slightly more memory):
    int smallerFontSize = BASELINE_FONT_SIZE - 2;  // <small> = <default> - 2px
    strncpy(mainConfig.Name, "Roboto-Regular-400-12px*", 40);
//  mainConfig.GlyphOffset = { 0.f, 0.75f };    // hand-tweaked relative offset
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (gf_roboto_regular_400_compressed_data,
         gf_roboto_regular_400_compressed_size,
         smallerFontSize, &mainConfig);
    
//  mergeConfig.GlyphOffset = { 0.f, 1.25f };  // needed due to different sizes
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
                                          (fa_solid_900_compressed_data,
                                           fa_solid_900_compressed_size,
                                           FA_FONT_SIZE(smallerFontSize),
                                           &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[3 = IM_BOLD_FONT]: ------------------------------------

    // (Note: using 700 since 900 was a little too much.)
    int boldFontSize = BASELINE_FONT_SIZE;                 // <default> px size
    strncpy(mainConfig.Name, "Roboto-Bold-700-14px*", 40);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
                                          (gf_roboto_bold_compressed_data,
                                           gf_roboto_bold_compressed_size,
                                           boldFontSize, &mainConfig);
    
    // Merge in Font Awesome icons at same font size:
    strncpy(mergeConfig.Name, mainConfig.Name, 40);
//  mergeConfig.GlyphOffset = { 0.f, 0.5f };// manually tweaked vertical offset
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
         FA_FONT_SIZE(boldFontSize), &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[4 = IM_BOLD_LARGER_FONT]: -----------------------------
    
    int boldLargeFontSize = BASELINE_FONT_SIZE + 2;               // just +2 px
    strncpy(mainConfig.Name, "Roboto-Bold-700-16px*", 40);
//  mainConfig.GlyphOffset = { 0.f, -1.5f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
                                          (gf_roboto_bold_compressed_data,
                                           gf_roboto_bold_compressed_size,
                                           boldLargeFontSize, &mainConfig);
        
    // Merge in Font Awesome icons at same font size:
    strncpy(mergeConfig.Name, mainConfig.Name, 40);
//  mergeConfig.GlyphOffset = { 0.f, -1.f };    // needs a lot of tweaking now!
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
         FA_FONT_SIZE(boldLargeFontSize), &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[5 = IM_MONO_NORMAL_FONT]: -----------------------------
    
    int monoFontSize = BASELINE_FONT_SIZE;
    strncpy(mainConfig.Name, "RobotoMono-Regular-400-15px", 40);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
                                    (gf_robotomono_regular_400_compressed_data,
                                     gf_robotomono_regular_400_compressed_size,
                                     monoFontSize,
                                     &mainConfig);
    
    // Merge in Font Awesome glyphs with whichever size to this one as well:
    strncpy(mergeConfig.Name, mainConfig.Name, 40);
//  mergeConfig.GlyphOffset = { 0.f, 2.75f };   // (this one does seem extreme)
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
        (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
         FA_FONT_SIZE(monoFontSize), &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[6 = IM_ITALIC_FONT]: ----------------------------------

    int italicFontSize = BASELINE_FONT_SIZE;
    strncpy(mainConfig.Name, "Roboto-Italic-400-14px", 40);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
                                          gf_roboto_italic_400_compressed_data,
                                          gf_roboto_italic_400_compressed_size,
                                                          italicFontSize,
                                                          &mainConfig);
    
    // Merge in Font Awesome icons at same font size:
    // FIXME: Find and download FontAwesome *italics* 900, so they mesh better!
    strncpy(mergeConfig.Name, mainConfig.Name, 40);
//  mergeConfig.GlyphOffset = { 0.f, 0.5f };           // tweak vertical offset
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
    (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
     FA_FONT_SIZE(italicFontSize), &mergeConfig, icon_ranges.Data);
    
// -------------- Fonts[7 = IM_MONO_MEDIUM_FONT]: -----------------------------
    
    int mediumMonoFontSize = BASELINE_FONT_SIZE + 1;  // (runs a little small!)
    strncpy(mainConfig.Name, "RobotoMono-500-14px", 40);
//  mainConfig.GlyphOffset = { 0.f, -0.5f };           // tweak vertical offset
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
                                        (gf_robotomono_regular_compressed_data,
                                         gf_robotomono_regular_compressed_size,
                                         mediumMonoFontSize,
                                         &mainConfig);
    
    // Merge in Font Awesome icons at an extra reduced font size (relatively):
    strncpy(mergeConfig.Name, mainConfig.Name, 40);
    mergeConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF
    (fa_solid_900_compressed_data, fa_solid_900_compressed_size,
     FA_FONT_SIZE(mediumMonoFontSize), &mergeConfig, icon_ranges.Data);
    
// ----------------------------------------------------------------------------

    // Callers can now use ImGui::PushFont() with the 8 loaded fonts above.
    return true;
}

/// Remove the static data created by ImGui -- specifically, the Font Atlas, so
/// we can force it to re-load when calling InitializeImGui().
void RemoveImGui ()
{
    // Clear away our "Font Atlas" that may have been previously loaded, for
    // example if the user changes the all-important "senior citizen mode"
    // to be on or off, we need to completely re-build it, so this lets us
    // remove it so it can be re-built readily (by simply toggling ABC off/on):

#ifdef IMGUI_V190_REFACTOR             /* needed with ImGui v1.9x and later: */
    if (ImGui::GetCurrentContext() != NULL)
        ImGui::GetIO().Fonts = NULL;  // don't let ImGui keep using font atlas!
#endif

    if (ImgWindow::sFontAtlas)
        ImgWindow::sFontAtlas.reset(); // release our singleton to delete atlas
}

