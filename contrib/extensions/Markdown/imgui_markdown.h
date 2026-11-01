#pragma once

// License: zlib
// Copyright (c) 2019 Juliette Foucaut & Doug Binks

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <cmath>
#include "imgui_internal.h"

// ============================================================================
// Automatic Mouse-Wheel Defeat on Vertically Scrollable Viewports
// ----------------------------------------------------------------------------
// In windows or child panels with active vertical scrolling (ScrollMax.y > 0),
// spinning the mouse wheel to scroll through content can inadvertently adjust
// controls if the cursor passes over a wheel-enabled widget. To prevent this,
// discrete widgets automatically refuse wheel inputs and suppress custom cursors
// when the viewport is scrollable, giving Dear ImGui's window scrolling priority.
// Opt-out compile-time: #define CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT
// Opt-out runtime:      ImGui::SetMouseWheelAutoDefeat(false);
// ============================================================================
#ifndef IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED
#define IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED

#ifndef IMGUI_CONTRIB_INTERNAL_INCLUDED
  #if defined(__has_include)
    #if __has_include("imgui_internal.h")
      #include "imgui_internal.h"
      #define IMGUI_CONTRIB_INTERNAL_INCLUDED 1
    #endif
  #endif
#endif
#if !defined(IMGUI_CONTRIB_INTERNAL_INCLUDED) && (defined(IMGUI_WINDOW_HARD_MIN_SIZE) || defined(IMGUI_INTERNAL_H))
  #define IMGUI_CONTRIB_INTERNAL_INCLUDED 1
#endif

namespace ImGui {
namespace Contrib {
    inline bool& GetMouseWheelAutoDefeatEnabled() {
        static bool s_Enabled = true;
        return s_Enabled;
    }

    inline bool IsCurrentWindowScrollableY() {
#if defined(CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT) || \
    defined(IMGUI_CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT) || \
    defined(IMGUI_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT)
        return false;
#else
        if (!GetMouseWheelAutoDefeatEnabled()) {
            return false;
        }
#if defined(IMGUI_CONTRIB_INTERNAL_INCLUDED) || defined(IMGUI_INTERNAL_H)
        ImGuiContext* g = ImGui::GetCurrentContext();
        if (g && g->CurrentWindow) {
            for (ImGuiWindow* w = g->CurrentWindow; w != nullptr; w = w->ParentWindow) {
                if (w->ScrollMax.y > 0.0f && !(w->Flags & ImGuiWindowFlags_NoScrollWithMouse))
                    return true;
                if (!(w->Flags & ImGuiWindowFlags_ChildWindow))
                    break;
            }
            return false;
        }
#endif
        return ImGui::GetScrollMaxY() > 0.0f;
#endif
    }
} // namespace Contrib

    inline void SetMouseWheelAutoDefeat(bool enable = true) {
        Contrib::GetMouseWheelAutoDefeatEnabled() = enable;
    }
    inline bool GetMouseWheelAutoDefeat() {
        return Contrib::GetMouseWheelAutoDefeatEnabled();
    }
} // namespace ImGui
#endif // IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED

#ifndef ORIGINAL	/* brat's hack */
namespace ImGui
{
/// Get font scale as we prefer to use it:
inline IMGUI_API float GetFontScale (bool useParent = false)
{
#ifdef NEW_ZOOM_METHOD
	return GetIO().FontGlobalScale;
#else
	ImGuiWindow *window = GetCurrentContext()->CurrentWindow;
	if ((useParent || (window && window->FontWindowScale == 1.f &&
					   window->ParentWindow &&
					   window->ParentWindow->FontWindowScale != 1.f)) &&
		window->ParentWindow)
		return window->ParentWindow->FontWindowScale;
	else if (window)
		return window->FontWindowScale;
	else
		return 1.f;				  // fall-back in case something's really wrong
#endif
}

/// Helper to get effective visual scale considering both window zoom and font size
inline IMGUI_API float GetVisualScale()
{
	float font_scale = GetFontScale(false);
	float font_size = GetFontSize();
	if (font_scale > 0.0f && fabsf(font_scale - 1.0f) > 0.001f)
	{
		float base_sz = font_size / font_scale;
		if (base_sz > 0.0f && fabsf(base_sz - 14.0f) > 1.0f)
		{
			return font_scale * (base_sz / 14.0f);
		}
		return font_scale;
	}
	return (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
}
}
#endif

// Supported link delimiters:
// ImGee natively supports BOTH standard CommonMark [...] and legacy/alternative {...} brackets simultaneously.
// Define IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY (or CONTRIB_MARKDOWN_CURLY_ONLY) to strictly require {...} brackets,
// preserving unescaped square brackets [...] as literal text without needing backslash escapes.
#define MD_SQUARE_BRACKET_OPEN '['
#define MD_SQUARE_BRACKET_CLOSE ']'
#define MD_CURLY_BRACKET_OPEN '{'
#define MD_CURLY_BRACKET_CLOSE '}'

namespace ImGui
{
    //-----------------------------------------------------------------------------
    // Basic types
    //-----------------------------------------------------------------------------

    struct Link;
    struct MarkdownConfig;

    struct MarkdownLinkCallbackData                                 // for both links and images
    {
        const char*             text;                               // text between square brackets []
        int                     textLength;
        const char*             link;                               // text between brackets ()
        int                     linkLength;
        void*                   userData;
        bool                    isImage;                            // true if '!' is detected in front of the link syntax
    };

    struct MarkdownTooltipCallbackData                              // for tooltips
    {
        MarkdownLinkCallbackData linkData;
        const char*              linkIcon;
    };
    
    struct MarkdownImageData
    {
        bool                    isValid = false;                    // if true, will draw the image
        bool                    useLinkCallback = false;            // if true, linkCallback will be called when image is clicked
        ImTextureID             user_texture_id = 0;                // see ImGui::Image
        ImVec2                  size = ImVec2( 100.0f, 100.0f );    // see ImGui::Image
        ImVec2                  uv0 = ImVec2( 0, 0 );               // see ImGui::Image
        ImVec2                  uv1 = ImVec2( 1, 1 );               // see ImGui::Image
        ImVec4                  tint_col = ImVec4( 1, 1, 1, 1 );    // see ImGui::Image
        ImVec4                  border_col = ImVec4( 0, 0, 0, 0 );  // see ImGui::Image
    };

    enum class MarkdownFormatType
    {
         NORMAL_TEXT,
         HEADING,
         UNORDERED_LIST,
         ORDERED_LIST,
         LINK,
         EMPHASIS,
         CODE_BLOCK,
    };

    struct MarkdownFormatInfo
    {
        MarkdownFormatType      type    = MarkdownFormatType::NORMAL_TEXT;
        int32_t                 level   = 0;                               // Set for headings: 1 for H1, 2 for H2 etc.
		char                    emphasisChar  = '\0';                      // The character used for emphasis (e.g. *, _, ^, >)
        bool                    itemHovered = false;                       // Currently only set for links when mouse hovered, only valid when start_ == false
        const MarkdownConfig*   config  = NULL;
        bool                    isStartOfDoc = false;                      // True if this block is at start of markdown document/text (no preceding non-whitespace content)
        bool                    isPrecededBySeparator = false;             // True if preceded by a heading separator or thematic break
        bool                    isPrecededByBlankLine = false;             // True if preceded by an empty line
        const char*             blockLang = nullptr;                       // Language identifier for CODE_BLOCK (e.g. "cpp", "lua")
        int32_t                 blockLangLength = 0;
    };

    typedef void                MarkdownLinkCallback( MarkdownLinkCallbackData data );    
    typedef void                MarkdownTooltipCallback( MarkdownTooltipCallbackData data );

    inline void defaultMarkdownTooltipCallback( MarkdownTooltipCallbackData data_ )
    {
        if( data_.linkData.isImage )
        {
            ImGui::SetTooltip( "%.*s", data_.linkData.linkLength, data_.linkData.link );
        }
        else
        {
            ImGui::SetTooltip( "%s Open in browser\n%.*s", data_.linkIcon, data_.linkData.linkLength, data_.linkData.link );
        }
    }

    typedef MarkdownImageData   MarkdownImageCallback( MarkdownLinkCallbackData data );

    inline MarkdownImageCallback*& GetDefaultMarkdownImageCallback() {
        static MarkdownImageCallback* s_DefaultImageCallback = nullptr;
        return s_DefaultImageCallback;
    }

    inline void SetDefaultMarkdownImageCallback(MarkdownImageCallback* callback) {
        GetDefaultMarkdownImageCallback() = callback;
    }
    typedef void                MarkdownFormalCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    struct MarkdownHeadingFormat
    {   
        ImFont*                 font;                               // ImGui font
        bool                    separator;                          // if true, an underlined separator is drawn after the header
    };

    struct MarkdownTagData
    {
        const char*             tagName;
        size_t                  tagNameLength;
        const char*             tagParam;
        size_t                  tagParamLength;
        const char*             innerText;
        size_t                  innerTextLength;
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownTagCallback( const MarkdownTagData& data );

    struct MarkdownCodeBlockData
    {
        const char*             code = nullptr;
        size_t                  codeLength = 0;
        const char*             language = nullptr;
        size_t                  languageLength = 0;
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownCodeBlockCallback( const MarkdownCodeBlockData& data );

    struct MarkdownTableState
    {
        size_t                  tableIndex = 0;                 // 0-based index of table in the markdown document
        int                     textStart = 0;                  // Start byte offset of table in markdown text
        int                     textEnd = 0;                    // End byte offset of table (including trailing comments)
        int                     rowsEnd = 0;                    // End byte offset of last data row
        float                   totalWidth = 0.0f;              // Total table width (normalized to 1.0x scale; 0.0f = auto content width)
        float                   presetWidth = 0.0f;             // Author preset width (0.0f = auto content width)
        int                     sortColumn = -1;                // Sorted column index (-1 = none)
        int                     sortDirection = 0;              // 0 = None, 1 = Ascending, 2 = Descending
        std::string             sortColumnName;                 // Plaintext header of the sorted column
        std::vector<float>      columnWidths;                   // Per-column widths (normalized)
        bool                    hasPreset = false;              // True if <!-- table-preset: ... --> exists
        bool                    hasState = false;               // True if <!-- table-state: ... --> exists
        bool                    isCommitted = true;             // True if change was committed (clicked sort, double-click toggled, drag released)
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownTableStateCallback( const MarkdownTableState& state, void* userData );

    struct MarkdownConfig
    {
        static const int        NUMHEADINGS = 4;

        MarkdownLinkCallback*   linkCallback = NULL;
        MarkdownTooltipCallback* tooltipCallback = NULL;
        MarkdownImageCallback*  imageCallback = NULL;
        MarkdownTagCallback*    tagCallback = NULL;
        MarkdownCodeBlockCallback* codeBlockCallback = NULL;        // Optional host custom syntax/layout handler
        MarkdownTableStateCallback* tableStateCallback = NULL;      // Optional notification callback when table sort or size changes
        const char*             (*codeBlockDescriptionCallback)( const char* lang, size_t langLength, void* userData ) = NULL; // Optional host language tooltip description callback
        void                    (*setClipboardTextFn)( const char* text, void* userData ) = NULL; // Optional host OS clipboard provider override
        bool                    showCodeCopyButton = true;          // If true, shows one-click [Copy] button in code header
        int                     tabSize = 4;                        // Tab stop width for \t expansion (default: 4)
        const char*             linkIcon = "";                      // icon displayd in link tooltip
        const char*             emphasisChars = "*_`~$+";           // string of characters recognized as emphasis markers (*, _, `, ~, $, +)
        bool                    underlineWithUnderscores = false;   // If true, __text__ renders as underline instead of bold
        MarkdownHeadingFormat   headingFormats[ NUMHEADINGS ] = { { NULL, true }, { NULL, true }, { NULL, true }, { NULL, true } };
        void*                   userData = NULL;
        MarkdownFormalCallback* formatCallback = defaultMarkdownFormatCallback;
        float                   bulletSpacing = 0.0f;               // 0.0f = default (4.0f); > 0.0f = custom gap between bullet and text
        float                   wrapWidth = 0.0f;                   // 0.0f = wrap at GetContentRegionAvail().x; > 0.0f = explicit wrap width; < 0.0f = no word-wrap (single line per \n)
        ImGuiTableFlags         tableFlags = 0;                     // Optional table flags override (0 = default GFM styling: Borders | RowBg | SizingStretchProp | NoSavedSettings)

        // Semantic font aliases (if set, takes priority over headingFormats):
        ImFont*                 h1Font = nullptr;                   // If nullptr, falls back to headingFormats[0].font
        ImFont*                 h2Font = nullptr;                   // If nullptr, falls back to headingFormats[1].font
        ImFont*                 h3Font = nullptr;                   // If nullptr, falls back to headingFormats[2].font
        ImFont*                 boldFont = nullptr;                 // If nullptr, falls back to headingFormats[3].font
        ImFont*                 italicFont = nullptr;               // If nullptr, falls back to ImGuiCol_TextDisabled
        ImFont*                 monoFont = nullptr;                 // If nullptr, falls back to ambient window font
        ImFont*                 regularFont = nullptr;              // If nullptr, falls back to ambient window font
    };

    //-----------------------------------------------------------------------------
    // External interface (Low-Level Rendering Engine)
    //-----------------------------------------------------------------------------

    namespace Markdown
    {
        inline void Render( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ );
    }

    //-----------------------------------------------------------------------------
    // Internals
    //-----------------------------------------------------------------------------

    struct TextRegion;
    struct Line;
    inline void UnderLine( ImColor col_ );
    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ );

    inline bool IsCharInsideWord( char c_ )
    {
        return c_ != ' ' && c_ != '\t' && c_ != '\n' && c_ != '\r' && 
               c_ != '.' && c_ != ',' && c_ != ';' && c_ != '!' && c_ != '?' && 
               c_ != '\"' && c_ != '(' && c_ != ')' && c_ != '[' && c_ != ']' && 
               c_ != '{' && c_ != '}';
    }

    // Typographic wrap helper: Prevents orphaned opening quotes/brackets and severed contractions/hyphens
    inline const char* AdjustWrapForTypography( const char* text_, const char* endLine, const char* text_end_ )
    {
        if( endLine <= text_ || endLine >= text_end_ )
            return endLine;

        char c_ = *endLine;
        char prev_c = *(endLine - 1);

        // 1. Orphaned Opening Delimiter: e.g. " word -> break before the opening delimiter so it travels with the word!
        if( ( prev_c == '\"' || prev_c == '\'' || prev_c == '(' || prev_c == '[' || prev_c == '{' ) &&
            ( endLine - 1 > text_ && *(endLine - 2) == ' ' ) )
        {
            return endLine - 1;
        }

        // 2. Severed Contraction or Hyphen: e.g. "man's" or "semi-final" -> break before the whole compound word!
        if( ( prev_c == '\'' || prev_c == '-' ) &&
            ( endLine - 1 > text_ && IsCharInsideWord( *(endLine - 2) ) ) &&
            ( IsCharInsideWord( c_ ) ) )
        {
            const char* rewind = endLine - 2;
            while( rewind > text_ && *rewind != ' ' )
                --rewind;

            if( rewind > text_ && *rewind == ' ' )
                return rewind;
        }

        return endLine;
    }

    struct TextRegion
    {
        TextRegion() : wrapWidth( 0.0f ), indentX( 0.0f ), leadIndents( 0 )
        {
        }
        ~TextRegion()
        {
            ResetIndent();
        }

        void RenderTextWrapped( const char* text_, const char* text_end_, bool bIndentToHere_ = false )
        {
            (void)bIndentToHere_;
            if( wrapWidth < 0.0f )
            {
                ImGui::TextUnformatted( text_, text_end_ );
                return;
            }
#ifdef ORIGINAL
            float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
			float		scale = ImGui::GetFontScale(false);// use parent's scale
#endif
            float       lineStartX = ImGui::GetCurrentWindow()->WorkRect.Min.x;
            if( ImGuiTable* cur_table = ImGui::GetCurrentTable() )
            {
                if( cur_table->CurrentColumn != -1 && cur_table->CurrentColumn < cur_table->ColumnsCount )
                {
                    lineStartX = cur_table->Columns[cur_table->CurrentColumn].WorkMinX;
                }
            }
            float       usedOnLine = ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
            float       widthLeft = ( wrapWidth > 0.0f ) ? ImMax( 10.0f, wrapWidth - usedOnLine ) : GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // If we are continuing on an existing line with prior content (usedOnLine > 0),
            // and the first word cannot cleanly fit in widthLeft, push the word to the next line.
            // When wrapping to a new line, strip any leading spaces that served only as inter-item separation on the previous line.
            if( usedOnLine > 0.0f )
            {
                const char* first_non_space = text_;
                while( first_non_space < text_end_ && *first_non_space == ' ' ) ++first_non_space;

                bool breaks_first_word = false;
                if( endLine <= first_non_space && first_non_space < text_end_ )
                {
                    breaks_first_word = true;
                }
                else if( endLine > first_non_space && endLine < text_end_ )
                {
                    const char* word_end = first_non_space;
                    while( word_end < text_end_ && IsCharInsideWord( *word_end ) ) ++word_end;
                    if( endLine < word_end )
                    {
                        breaks_first_word = true;
                    }
                }

                if( breaks_first_word )
                {
                    // Push to next line and suppress inter-item leading space!
                    ImGui::NewLine();
                    text_ = first_non_space;
                    widthLeft = ( wrapWidth > 0.0f ) ? wrapWidth : ImGui::GetContentRegionAvail().x;
                    endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                    endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                }
            }
            
            ImGui::TextUnformatted( text_, endLine );
            widthLeft = ( wrapWidth > 0.0f ) ? wrapWidth : GetContentRegionAvail().x;
            while( endLine < text_end_ )
            {
                text_ = endLine;
                while( text_ < text_end_ && *text_ == ' ' ) { ++text_; }    // skip spaces at start of line
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine && text_ < text_end_ ) 
                {
                    endLine++;
                }
                ImGui::TextUnformatted( text_, endLine );
            }
        }

        static float GetBulletSpacing( float custom_spacing = -1.0f )
        {
            if( custom_spacing >= 0.0f )
                return custom_spacing;
            return 4.0f; // Clean, elegant 4px spacing instead of bloated gap
        }

        void ApplyBullet( float custom_spacing = -1.0f )
        {
            float startX = ImGui::GetCursorScreenPos().x;
            ImGui::Bullet();
            ImGui::SameLine( 0.0f, GetBulletSpacing( custom_spacing ) );
            float textStartX = ImGui::GetCursorScreenPos().x;
            float bulletIndent = textStartX - startX;
            if( bulletIndent > 0.0f )
            {
                ImGui::Indent( bulletIndent );
                indentX += bulletIndent;
            }
        }

        void RenderListTextWrapped( const char* text_, const char* text_end_, float bullet_spacing = -1.0f )
        {
            ApplyBullet( bullet_spacing );
            RenderTextWrapped( text_, text_end_ );
        }

        void ApplyOrderedNumber( const char* num_str, const char* num_str_end, float custom_spacing = -1.0f )
        {
            float startX = ImGui::GetCursorScreenPos().x;
            ImGui::TextUnformatted( num_str, num_str_end );
            float spacing = ( custom_spacing >= 0.0f ) ? custom_spacing : ( ImGui::GetStyle().ItemSpacing.x * 0.5f );
            ImGui::SameLine( 0.0f, spacing );
            float textStartX = ImGui::GetCursorScreenPos().x;
            float numIndent = textStartX - startX;
            if( numIndent > 0.0f )
            {
                ImGui::Indent( numIndent );
                indentX += numIndent;
            }
        }

        void RenderOrderedListTextWrapped( const char* num_str, const char* num_str_end, const char* text_, const char* text_end_, float custom_spacing = -1.0f )
        {
            ApplyOrderedNumber( num_str, num_str_end, custom_spacing );
            RenderTextWrapped( text_, text_end_ );
        }

        bool RenderLinkText( const char* text_, const char* text_end_, const Link& link_, 
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ );

        void RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ = false );

        float GetIndent() const
        {
            return indentX;
        }

        int GetLeadIndents() const
        {
            return leadIndents;
        }

        void ApplyLeadIndent( int count )
        {
            while( leadIndents < count )
            {
                ImGui::Indent();
                ++leadIndents;
            }
            while( leadIndents > count )
            {
                ImGui::Unindent();
                --leadIndents;
            }
        }

        void ResetIndent()
        {
            if( indentX > 0.0f )
            {
                ImGui::Unindent( indentX );
            }
            indentX = 0.0f;
            while( leadIndents > 0 )
            {
                ImGui::Unindent();
                --leadIndents;
            }
        }

        float       wrapWidth;

    private:
        float       indentX;
        int         leadIndents;
    };

    struct Line {
        bool isHeading = false;
        bool isEmphasis = false;
        bool isUnorderedListStart = false;
        bool isOrderedListStart = false;
        int  orderedListNumStart = 0;
        int  orderedListNumEnd = 0;
        bool isLeadingSpace = true;     // spaces at start of line
        int  leadSpaceCount = 0;
        int  headingCount = 0;
        int  emphasisCount = 0;
		char emphasisChar  = '\0';
        int  lineStart = 0;
        int  lineEnd   = 0;
        int  lastRenderPosition = 0;     // lines may get rendered in multiple pieces
        bool isPrecededBySeparator = false;
        bool isPrecededByBlankLine = false;
        bool hadSeparator = false;
        bool wasBlank = false;
    };

    struct TextBlock {                  // subset of line
        int start = 0;
        int stop  = 0;
        int size() const
        {
            return stop - start;
        }
    };

    struct Link {
        enum LinkState {
            NO_LINK,
            HAS_SQUARE_BRACKET_OPEN,
            HAS_SQUARE_BRACKETS,
            HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
        };
        LinkState state = NO_LINK;
        TextBlock text;
        TextBlock url;
        bool isImage = false;
        int num_brackets_open = 0;
        char openBracketChar = 0;
    };

	struct Emphasis {
		enum EmphasisState {
			NONE,
			LEFT,
			MIDDLE,
			RIGHT,
		};
        EmphasisState state = NONE;
        TextBlock text;
        char sym;
	};

    inline void DrawTextDecorationLine( ImColor col_, bool is_strikethrough_ )
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        float y = is_strikethrough_ ? std::floor( ( min.y + max.y ) * 0.5f ) : std::floor( max.y - 1.0f );
        ImGui::GetWindowDrawList()->AddLine( ImVec2( min.x, y ), ImVec2( max.x, y ), col_, 1.0f );
    }

    inline void UnderLine( ImColor col_ )
    {
        DrawTextDecorationLine( col_, false );
    }

    inline bool IsMarkdownEscapable( char c_ )
    {
        return ( c_ == '!' || c_ == '\"' || c_ == '#' || c_ == '$' || c_ == '%' || c_ == '&' ||
                 c_ == '\'' || c_ == '(' || c_ == ')' || c_ == '*' || c_ == '+' || c_ == ',' ||
                 c_ == '-' || c_ == '.' || c_ == '/' || c_ == ':' || c_ == ';' || c_ == '<' ||
                 c_ == '=' || c_ == '>' || c_ == '?' || c_ == '@' || c_ == '[' || c_ == '\\' ||
                 c_ == ']' || c_ == '^' || c_ == '_' || c_ == '`' || c_ == '{' || c_ == '|' ||
                 c_ == '}' || c_ == '~' );
    }

    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ )
    {
        // Apply line-level leading space indentation (nested list depth)
        int numLeadIndents = line_.leadSpaceCount / 2;
        textRegion_.ApplyLeadIndent( numLeadIndents );

        // render
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.isPrecededBySeparator = line_.isPrecededBySeparator;
        formatInfo.isPrecededByBlankLine = line_.isPrecededByBlankLine;

        // Determine if this line/block is at the start of the markdown document (no preceding non-whitespace content)
        bool isAtStart = true;
        int checkLimit = line_.lineStart + line_.leadSpaceCount;
        for( int k = 0; k < checkLimit; ++k )
        {
            char ch = markdown_[k];
            if( ch == '\0' ) { break; }
            if( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
            {
                isAtStart = false;
                break;
            }
        }
        formatInfo.isStartOfDoc = isAtStart;

        int textStart = line_.lastRenderPosition + 1;
        int textSize = line_.lineEnd - textStart;
        if( line_.isUnorderedListStart )    // render unordered list
        {
            formatInfo.type = MarkdownFormatType::UNORDERED_LIST;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            const char* textEnd = text + textSize - 1;
            while( text < textEnd && *text == ' ' ) { ++text; }
            textRegion_.RenderListTextWrapped( text, textEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
        }
        else if( line_.isOrderedListStart ) // render ordered list (e.g. 1. , 2. )
        {
            formatInfo.type = MarkdownFormatType::ORDERED_LIST;
            mdConfig_.formatCallback( formatInfo, true );
            const char* numStart = markdown_ + line_.orderedListNumStart;
            const char* numEnd = markdown_ + line_.orderedListNumEnd;
            const char* text = markdown_ + textStart;
            const char* textEnd = text + textSize;
            while( text < textEnd && *text == ' ' ) { ++text; }
            textRegion_.RenderOrderedListTextWrapped( numStart, numEnd, text, textEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
        }
        else if( line_.isHeading )          // render heading
        {
            formatInfo.level = line_.headingCount;
            formatInfo.type = MarkdownFormatType::HEADING;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            textRegion_.RenderTextWrapped( text, text + textSize - 1 );

            MarkdownHeadingFormat fmt;
            if( line_.headingCount > MarkdownConfig::NUMHEADINGS )
            {
                fmt = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
            }
            else
            {
                fmt = mdConfig_.headingFormats[ line_.headingCount - 1 ];
            }
            if( fmt.separator )
            {
                line_.hadSeparator = true;
            }
        }
		else if( line_.isEmphasis )         // render emphasis
		{
			formatInfo.level = line_.emphasisCount;
			formatInfo.emphasisChar = line_.emphasisChar;
			formatInfo.type = MarkdownFormatType::EMPHASIS;
			mdConfig_.formatCallback(formatInfo, true);
			const char* text = markdown_ + textStart;
			textRegion_.RenderTextWrapped(text, text + textSize);
		}
        else                                // render a normal paragraph chunk
        {
            bool isLineEmpty = true;
            for( int k = textStart; k < line_.lineEnd; ++k )
            {
                if( markdown_[k] != ' ' && markdown_[k] != '\t' && markdown_[k] != '\r' )
                {
                    isLineEmpty = false;
                    break;
                }
            }
            if( isLineEmpty )
            {
                line_.wasBlank = true;
            }

            formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped( text, text + textSize );
        }
        mdConfig_.formatCallback( formatInfo, false );
    }

    inline void ExpandTabs( const char* line_start, const char* line_end, std::string& out, int tab_size = 4 )
    {
        out.clear();
        int col = 0;
        for( const char* p = line_start; p < line_end; ++p )
        {
            if( *p == '\t' )
            {
                int spaces = tab_size - ( col % tab_size );
                if( spaces == 0 ) spaces = tab_size;
                out.append( spaces, ' ' );
                col += spaces;
            }
            else
            {
                out.push_back( *p );
                ++col;
            }
        }
    }

    inline void DefaultRenderCodeLine( const std::string& line_str, bool is_lua, bool is_cpp, float wrap_screen_x = 0.0f, float scale = 1.0f )
    {
        if( line_str.empty() )
        {
            ImGui::NewLine();
            return;
        }

        if( !is_lua && !is_cpp )
        {
            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 220, 225, 235, 255 ) );
            if( wrap_screen_x > 0.0f )
            {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                ImGui::PushTextWrapPos( wrap_local_x );
                ImGui::TextUnformatted( line_str.c_str() );
                ImGui::PopTextWrapPos();
            }
            else
            {
                ImGui::TextUnformatted( line_str.c_str() );
            }
            ImGui::PopStyleColor();
            return;
        }

        // Check leading whitespace
        size_t first_non_space = line_str.find_first_not_of( " \t" );
        float line_start_x = ImGui::GetCursorScreenPos().x;
        float indent_w = 0.0f;
        if( first_non_space != std::string::npos && first_non_space > 0 )
        {
            indent_w = ImGui::CalcTextSize( line_str.c_str(), line_str.c_str() + first_non_space ).x;
        }
        float max_continuation = ( wrap_screen_x > line_start_x ) ? ( ( wrap_screen_x - line_start_x ) * 0.5f ) : ( 32.0f * scale );
        float continuation_indent = ImMin( indent_w + 16.0f * scale, max_continuation );

        if( first_non_space != std::string::npos )
        {
            if( ( is_lua && first_non_space + 1 < line_str.length() && line_str[first_non_space] == '-' && line_str[first_non_space + 1] == '-' ) ||
                ( is_cpp && first_non_space + 1 < line_str.length() && line_str[first_non_space] == '/' && line_str[first_non_space + 1] == '/' ) )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 106, 153, 85, 255 ) ); // Comment green
                if( wrap_screen_x > 0.0f )
                {
                    ImGuiWindow* win = ImGui::GetCurrentWindow();
                    float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                    ImGui::PushTextWrapPos( wrap_local_x );
                    ImGui::TextUnformatted( line_str.c_str() );
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::TextUnformatted( line_str.c_str() );
                }
                ImGui::PopStyleColor();
                return;
            }
            if( is_cpp && line_str[first_non_space] == '#' )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 198, 120, 221, 255 ) ); // Preprocessor purple
                if( wrap_screen_x > 0.0f )
                {
                    ImGuiWindow* win = ImGui::GetCurrentWindow();
                    float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                    ImGui::PushTextWrapPos( wrap_local_x );
                    ImGui::TextUnformatted( line_str.c_str() );
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::TextUnformatted( line_str.c_str() );
                }
                ImGui::PopStyleColor();
                return;
            }
        }

        // Scan tokens across the line
        size_t pos = 0;
        size_t len = line_str.length();
        float min_x_for_wrap = line_start_x;

        auto renderToken = [&]( const char* text_start, const char* text_end, ImU32 col, bool is_last ) {
            float tok_w = ImGui::CalcTextSize( text_start, text_end ).x;
            float cur_x = ImGui::GetCursorScreenPos().x;

            if( wrap_screen_x > 0.0f && ( cur_x + tok_w > wrap_screen_x ) && ( cur_x > min_x_for_wrap + 1.0f ) )
            {
                ImGui::NewLine();
                ImGui::SetCursorScreenPos( ImVec2( line_start_x + continuation_indent, ImGui::GetCursorScreenPos().y ) );
                min_x_for_wrap = line_start_x + continuation_indent;
            }

            ImGui::PushStyleColor( ImGuiCol_Text, col );
            if( wrap_screen_x > 0.0f )
            {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                ImGui::PushTextWrapPos( wrap_local_x );
                ImGui::TextUnformatted( text_start, text_end );
                ImGui::PopTextWrapPos();
            }
            else
            {
                ImGui::TextUnformatted( text_start, text_end );
            }
            ImGui::PopStyleColor();

            if( !is_last )
            {
                ImGui::SameLine( 0.0f, 0.0f );
            }
        };

        while( pos < len )
        {
            // 1. Comments:
            if( ( is_lua && pos + 1 < len && line_str[pos] == '-' && line_str[pos + 1] == '-' ) ||
                ( is_cpp && pos + 1 < len && line_str[pos] == '/' && line_str[pos + 1] == '/' ) )
            {
                renderToken( line_str.c_str() + pos, line_str.c_str() + len, IM_COL32( 106, 153, 85, 255 ), true );
                break;
            }

            // 2. Strings:
            if( line_str[pos] == '\"' || line_str[pos] == '\'' )
            {
                char q = line_str[pos];
                size_t q_end = pos + 1;
                while( q_end < len && line_str[q_end] != q )
                {
                    if( line_str[q_end] == '\\' && q_end + 1 < len ) ++q_end;
                    ++q_end;
                }
                if( q_end < len ) ++q_end;
                renderToken( line_str.c_str() + pos, line_str.c_str() + q_end, IM_COL32( 206, 145, 120, 255 ), q_end >= len );
                pos = q_end;
                continue;
            }

            // 3. Identifiers / words:
            if( isalpha( (unsigned char)line_str[pos] ) || line_str[pos] == '_' )
            {
                size_t w_end = pos;
                while( w_end < len && ( isalnum( (unsigned char)line_str[w_end] ) || line_str[w_end] == '_' ) )
                {
                    ++w_end;
                }
                std::string word = line_str.substr( pos, w_end - pos );
                bool is_kw = false;
                if( is_lua )
                {
                    static const char* kws[] = { "local", "function", "if", "then", "else", "elseif", "end", "return", "for", "while", "do", "break", "nil", "true", "false", nullptr };
                    for( int k = 0; kws[k]; ++k ) { if( word == kws[k] ) { is_kw = true; break; } }
                }
                else if( is_cpp )
                {
                    static const char* kws[] = { "int", "float", "double", "bool", "void", "char", "auto", "const", "class", "struct", "if", "else", "return", "for", "while", "true", "false", "nullptr", nullptr };
                    for( int k = 0; kws[k]; ++k ) { if( word == kws[k] ) { is_kw = true; break; } }
                }

                ImU32 col = is_kw ? IM_COL32( 198, 120, 221, 255 ) : IM_COL32( 220, 225, 235, 255 );
                renderToken( word.c_str(), word.c_str() + word.length(), col, w_end >= len );
                pos = w_end;
                continue;
            }

            // 4. Other characters (whitespace, punctuation, numbers):
            size_t other_end = pos;
            while( other_end < len && !isalpha( (unsigned char)line_str[other_end] ) && line_str[other_end] != '_' &&
                   line_str[other_end] != '\"' && line_str[other_end] != '\'' &&
                   !( is_lua && other_end + 1 < len && line_str[other_end] == '-' && line_str[other_end + 1] == '-' ) &&
                   !( is_cpp && other_end + 1 < len && line_str[other_end] == '/' && line_str[other_end + 1] == '/' ) )
            {
                ++other_end;
            }
            renderToken( line_str.c_str() + pos, line_str.c_str() + other_end, IM_COL32( 220, 225, 235, 255 ), other_end >= len );
            pos = other_end;
        }
    }

    namespace MarkdownDetail {
        typedef void (*CodeBlockTooltipFn)( ImGuiID id, bool hovered, const char* text, bool toggle_on_click );
        typedef void (*DisabledCopyTooltipFn)( ImGuiID id, bool hovered );
        typedef void (*ClearTooltipFn)();

        inline CodeBlockTooltipFn& GetCodeBlockTooltipFn() {
            static CodeBlockTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline DisabledCopyTooltipFn& GetDisabledCopyTooltipFn() {
            static DisabledCopyTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline ClearTooltipFn& GetClearTooltipFn() {
            static ClearTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline void ShowCodeBlockTooltip( ImGuiID id, bool hovered, const char* text, bool toggle_on_click = false ) {
            if( GetCodeBlockTooltipFn() ) {
                GetCodeBlockTooltipFn()( id, hovered, text, toggle_on_click );
            }
        }

        inline void ClearActiveTooltip() {
            if( GetClearTooltipFn() ) {
                GetClearTooltipFn()();
            }
        }
    }

    inline bool IsMarkdownClipboardSupported( const MarkdownConfig& config )
    {
        // 1. Explicit host callback always satisfies clipboard support:
        if( config.setClipboardTextFn != nullptr )
        {
            return true;
        }

        // 2. Windows (IBM desktop plugin or _WIN32): Dear ImGui defaults to Win32 OpenClipboard/SetClipboardData
#if (defined(IBM) && IBM) || defined(_WIN32)
#if defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)
        return false;
#else
        return true;
#endif

        // 3. macOS (APL desktop plugin or __APPLE__): Requires IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS
#elif (defined(APL) && APL) || defined(__APPLE__)
#if defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)
        return true;
#else
        return false;
#endif

        // 4. Linux (LIN desktop plugin or __linux__): Vanilla Dear ImGui has no default X11 clipboard
#elif (defined(LIN) && LIN) || defined(__linux__)
        return false;
#else
        return false;
#endif
    }

    inline void RenderCodeBlock( const char* code_, size_t codeLength_, const char* lang_, size_t langLength_, const MarkdownConfig& mdConfig_, size_t block_index = 0, int fence_indent = 0 )
    {
        if( !code_ ) return;

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        if( !window ) return;

        float scale = ImGui::GetVisualScale();
        float pad_x = 12.0f * scale;
        float pad_y = 8.0f * scale;
        float rounding = 4.0f * scale;

        ImFont* ambient_font = ImGui::GetFont();
        float ambient_font_sz = ImGui::GetFontSize();
        ImFont* regular_font = mdConfig_.regularFont ? mdConfig_.regularFont : ( ( ambient_font && ambient_font != mdConfig_.monoFont ) ? ambient_font : ( ( ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->Fonts.Size > 0 ) ? ImGui::GetIO().Fonts->Fonts[0] : nullptr ) );
        float regular_font_sz = ambient_font_sz;
        ImFont* mono_font = mdConfig_.monoFont;
        if( mono_font )
        {
            ImGui::PushFont( mono_font );
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );

        ImVec2 start_pos = ImGui::GetCursorScreenPos();
        float avail_width = ( mdConfig_.wrapWidth > 0.0f ) ? mdConfig_.wrapWidth : ImGui::GetContentRegionAvail().x;
        if( avail_width <= 20.0f ) avail_width = 200.0f;

        std::string lang_tag;
        if( langLength_ > 0 && lang_ )
        {
            lang_tag.assign( lang_, langLength_ );
        }
        bool is_markdown = ( lang_tag == "markdown" || lang_tag == "md" );
        bool is_lua = ( lang_tag == "lua" || lang_tag == "lua51" );
        bool is_cpp = ( lang_tag == "cpp" || lang_tag == "c" || lang_tag == "h" || lang_tag == "hpp" );
        bool is_plaintext = ( lang_tag == "text" || lang_tag == "plaintext" || lang_tag == "txt" );
        bool is_custom = ( mdConfig_.codeBlockCallback != nullptr );
        bool is_recognized = is_markdown || is_lua || is_cpp || is_plaintext || is_custom;

        // Generate unique, scroll-stable ID combining content hash and block index:
        ImU32 content_hash = ImHashData( code_, codeLength_ );
        char id_buf[48];
        snprintf( id_buf, sizeof(id_buf), "##cb_%08X_%zu", content_hash, block_index );
        ImGuiID code_id = ImGui::GetID( id_buf );
        ImGui::PushID( (int)code_id );

        ImGuiStorage* storage = ImGui::GetStateStorage();
        char prev_id_buf[32];
        snprintf( prev_id_buf, sizeof(prev_id_buf), "##cb_prev_%zu", block_index );
        ImGuiID preview_id = ImGui::GetID( prev_id_buf );

        bool is_preview = false;
        if( is_markdown && storage )
        {
            is_preview = storage->GetBool( code_id, false ) || storage->GetBool( preview_id, false );
        }

        float font_sz = ImGui::GetFontSize();
        float control_h = font_sz + 4.0f * scale;
        float header_h = 0.0f;
        if( lang_tag.length() > 0 || mdConfig_.showCodeCopyButton || is_markdown )
        {
            header_h = control_h + 8.0f * scale;
        }
        float control_y = start_pos.y + ( header_h - control_h ) * 0.5f + 0.5f * scale;

        ImDrawList* dl = window->DrawList;
        ImDrawListSplitter splitter;
        splitter.Split( dl, 2 );
        splitter.SetCurrentChannel( dl, 1 ); // Foreground text channel

        ImVec2 content_clip_min = ImVec2( start_pos.x, start_pos.y + header_h );
        ImVec2 content_clip_max = ImVec2( start_pos.x + avail_width, window->ClipRect.Max.y );
        dl->PushClipRect( content_clip_min, content_clip_max, true );

        ImGui::SetCursorScreenPos( ImVec2( start_pos.x + pad_x, start_pos.y + header_h + pad_y ) );
        ImGui::BeginGroup();

        if( is_markdown && is_preview )
        {
            if( mono_font ) ImGui::PopFont();
            MarkdownConfig preview_config = mdConfig_;
            preview_config.wrapWidth = avail_width - 2.0f * pad_x;
            if( fence_indent > 0 )
            {
                std::string stripped_preview;
                const char* cp = code_;
                const char* cend = code_ + codeLength_;
                while( cp < cend )
                {
                    const char* ls = cp;
                    while( cp < cend && *cp != '\n' && *cp != '\r' ) ++cp;
                    const char* le = cp;
                    int stripped = 0;
                    while( ls < le && stripped < fence_indent && ( *ls == ' ' || *ls == '\t' ) )
                    {
                        stripped += ( *ls == '\t' ) ? 4 : 1;
                        ++ls;
                    }
                    stripped_preview.append( ls, le - ls );
                    if( cp < cend && *cp == '\r' ) { stripped_preview.push_back( *cp ); ++cp; }
                    if( cp < cend && *cp == '\n' ) { stripped_preview.push_back( *cp ); ++cp; }
                }
                ImGui::Markdown::Render( stripped_preview.c_str(), stripped_preview.length(), preview_config );
            }
            else
            {
                ImGui::Markdown::Render( code_, codeLength_, preview_config );
            }
            if( mono_font ) ImGui::PushFont( mono_font );
        }
        else if( mdConfig_.codeBlockCallback )
        {
            MarkdownCodeBlockData cbData;
            cbData.code = code_;
            cbData.codeLength = codeLength_;
            cbData.language = lang_;
            cbData.languageLength = langLength_;
            cbData.config = &mdConfig_;
            cbData.userData = mdConfig_.userData;
            mdConfig_.codeBlockCallback( cbData );
        }
        else
        {
            float wrap_screen_x = start_pos.x + avail_width - pad_x;
            const char* p = code_;
            const char* end = code_ + codeLength_;
            std::string line_buf;
            while( p < end )
            {
                const char* line_start = p;
                while( p < end && *p != '\n' && *p != '\r' ) ++p;
                const char* line_end = p;

                // CommonMark Sec 4.5: strip up to fence_indent leading spaces/tabs
                int stripped = 0;
                while( line_start < line_end && stripped < fence_indent && ( *line_start == ' ' || *line_start == '\t' ) )
                {
                    stripped += ( *line_start == '\t' ) ? 4 : 1;
                    ++line_start;
                }

                ExpandTabs( line_start, line_end, line_buf, mdConfig_.tabSize > 0 ? mdConfig_.tabSize : 4 );
                DefaultRenderCodeLine( line_buf, is_lua, is_cpp, wrap_screen_x, scale );

                if( p < end && *p == '\r' ) ++p;
                if( p < end && *p == '\n' ) ++p;
            }
        }

        // Add bottom container padding inside the group so CursorMaxPos accounts for it
        ImGui::Dummy( ImVec2( 0.0f, pad_y ) );
        ImGui::EndGroup();
        dl->PopClipRect();

        ImVec2 content_max = ImGui::GetItemRectMax();
        float content_w = ( content_max.x - start_pos.x ) + pad_x;

        ImVec2 raw_sz = ( regular_font && regular_font != mono_font )
            ? regular_font->CalcTextSizeA( regular_font_sz, FLT_MAX, -1.0f, "Raw" )
            : ImGui::CalcTextSize( "Raw" );
        ImVec2 prev_sz = ( regular_font && regular_font != mono_font )
            ? regular_font->CalcTextSizeA( regular_font_sz, FLT_MAX, -1.0f, "Preview" )
            : ImGui::CalcTextSize( "Preview" );
        float toggle_gap = 5.0f * scale;
        float toggle_track_h = ImMax( 11.0f * scale, font_sz * 0.72f );
        float toggle_track_w = toggle_track_h * 1.9f;
        float toggle_total_w = raw_sz.x + toggle_gap + toggle_track_w + toggle_gap + prev_sz.x + 4.0f * scale;

        float chip_w = ( lang_tag.length() > 0 ) ? ( ImGui::CalcTextSize( lang_tag.c_str() ).x + 12.0f * scale ) : 0.0f;
        float toggle_w = is_markdown ? toggle_total_w : 0.0f;

        float icon_w = 10.0f * scale;
        float icon_gap = 4.0f * scale;
        ImVec2 copy_max_text_sz = ImGui::CalcTextSize( "Copied!" );
        float fixed_copy_w = icon_w + icon_gap + copy_max_text_sz.x + 14.0f * scale;
        float copy_w = mdConfig_.showCodeCopyButton ? fixed_copy_w : 0.0f;

        float min_header_w = pad_x * 2.0f + chip_w + ( toggle_w > 0.0f ? ( toggle_w + 16.0f * scale ) : 0.0f ) + ( copy_w > 0.0f ? ( copy_w + 12.0f * scale ) : 0.0f );

        float needed_width = ImMax( min_header_w, content_w );

        // Persistent maximum width cache: prevents jarring container shrink when toggling between Raw and Preview!
        ImGuiID max_width_id = code_id + 0x5744; // 'WD' (width cache)
        float* p_max_w = storage ? storage->GetFloatRef( max_width_id, 0.0f ) : nullptr;
        if( p_max_w )
        {
            if( *p_max_w > avail_width )
            {
                *p_max_w = avail_width;
            }
            if( needed_width > *p_max_w )
            {
                *p_max_w = ImMin( needed_width, avail_width );
            }
            needed_width = ImMax( needed_width, *p_max_w );
        }

        float min_block_w = ImMin( 100.0f * scale, avail_width );
        float block_width = ImClamp( needed_width, min_block_w, avail_width );
        float block_height = content_max.y - start_pos.y;

        ImVec2 box_min = start_pos;
        ImVec2 box_max = ImVec2( start_pos.x + block_width, start_pos.y + block_height );

        splitter.SetCurrentChannel( dl, 0 ); // Background channel
        ImU32 bg_col = IM_COL32( 22, 22, 26, 240 );
        ImU32 border_col = IM_COL32( 60, 60, 70, 180 );
        dl->AddRectFilled( box_min, box_max, bg_col, rounding );

        if( header_h > 0.0f )
        {
            ImU32 header_bg = IM_COL32( 32, 33, 40, 240 );
            ImU32 divider_col = IM_COL32( 50, 52, 62, 200 );
            ImVec2 header_max( box_max.x, start_pos.y + header_h );
#ifndef ImDrawFlags_RoundCornersTop
#define ImDrawFlags_RoundCornersTop ( 1 | 2 ) // ImDrawCornerFlags_Top
#endif
            dl->AddRectFilled( box_min, header_max, header_bg, rounding, ImDrawFlags_RoundCornersTop );
            dl->AddLine( ImVec2( box_min.x, header_max.y ), header_max, divider_col, 1.0f * scale );
        }

        dl->AddRect( box_min, box_max, border_col, rounding, 0, 1.0f * scale );

        splitter.SetCurrentChannel( dl, 1 ); // Foreground channel

        if( header_h > 0.0f )
        {
            // 1. Language pill on the LEFT (Steve TextLink with tooltip & hand cursor):
            if( lang_tag.length() > 0 )
            {
                ImVec2 chip_min( start_pos.x + pad_x, control_y );
                ImVec2 chip_max( chip_min.x + chip_w, control_y + control_h );

                ImGui::SetCursorScreenPos( chip_min );
                ImGui::PushID( (int)code_id + 3 );
                ImGui::InvisibleButton( "##lang_chip", ImVec2( chip_w, control_h ) );
                bool chip_hovered = ImGui::IsItemHovered();
                if( chip_hovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                }

                const char* tip_text = nullptr;
                if( mdConfig_.codeBlockDescriptionCallback )
                {
                    tip_text = mdConfig_.codeBlockDescriptionCallback( lang_, langLength_, mdConfig_.userData );
                }
                if( !tip_text )
                {
                    if( is_cpp ) tip_text = "C++ format (Syntax highlighted)";
                    else if( is_lua ) tip_text = "Lua format (Syntax highlighted)";
                    else if( is_markdown ) tip_text = "Markdown format (Supports live Preview)";
                    else if( is_plaintext ) tip_text = "Plain text format (Unformatted)";
                    else if( is_custom ) tip_text = "Custom host language syntax";
                    else tip_text = "Plain text (Unrecognized language)";
                }

                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 3, chip_hovered, tip_text, true );
                ImGui::PopID();

                // Style colors:
                ImU32 chip_bg = IM_COL32( 40, 42, 48, 180 );
                ImU32 chip_bg_hover = IM_COL32( 50, 52, 60, 210 );
                ImU32 chip_fg = IM_COL32( 150, 155, 165, 220 );

                if( is_lua )
                {
                    chip_bg = IM_COL32( 24, 75, 135, 230 );
                    chip_bg_hover = IM_COL32( 32, 95, 170, 245 );
                    chip_fg = IM_COL32( 215, 235, 255, 255 );
                }
                else if( is_cpp )
                {
                    chip_bg = IM_COL32( 40, 60, 140, 230 );
                    chip_bg_hover = IM_COL32( 52, 78, 175, 245 );
                    chip_fg = IM_COL32( 220, 230, 255, 255 );
                }
                else if( is_markdown )
                {
                    chip_bg = IM_COL32( 28, 95, 75, 230 );
                    chip_bg_hover = IM_COL32( 36, 120, 95, 245 );
                    chip_fg = IM_COL32( 200, 245, 225, 255 );
                }
                else if( is_plaintext )
                {
                    chip_bg = IM_COL32( 45, 60, 80, 220 );
                    chip_bg_hover = IM_COL32( 58, 76, 102, 245 );
                    chip_fg = IM_COL32( 210, 225, 235, 255 );
                }
                else if( is_custom )
                {
                    chip_bg = IM_COL32( 75, 45, 120, 230 );
                    chip_bg_hover = IM_COL32( 95, 58, 150, 245 );
                    chip_fg = IM_COL32( 235, 215, 255, 255 );
                }

                ImU32 cur_bg = chip_hovered ? chip_bg_hover : chip_bg;
                dl->AddRectFilled( chip_min, chip_max, cur_bg, 3.0f * scale );
                if( !is_recognized )
                {
                    dl->AddRect( chip_min, chip_max, IM_COL32( 65, 68, 75, 180 ), 3.0f * scale, 0, 1.0f * scale );
                }

                ImVec2 tag_sz = ImGui::CalcTextSize( lang_tag.c_str() );
                float text_x = chip_min.x + ( chip_w - tag_sz.x ) * 0.5f;
                float text_y = control_y + ( control_h - font_sz ) * 0.5f;
                dl->AddText( ImVec2( text_x, text_y ), chip_fg, lang_tag.c_str() );
            }

            // 2. Copy button on far RIGHT (Actionable push-button with vector icon & tooltip):
            float now = (float)ImGui::GetTime();
            float last_time = storage ? storage->GetFloat( code_id, -1000.0f ) : -1000.0f;
            bool is_recently_copied = ( now >= last_time ) && ( ( now - last_time ) < 1.5f );
            const char* copy_label = is_recently_copied ? "Copied!" : "Copy";

            float cur_copy_x = box_max.x - fixed_copy_w - pad_x;

            bool is_clipboard_supported = IsMarkdownClipboardSupported( mdConfig_ );

            if( mdConfig_.showCodeCopyButton )
            {
                ImVec2 btn_min( cur_copy_x, control_y );
                ImVec2 btn_max( cur_copy_x + fixed_copy_w, control_y + control_h );

                ImGui::SetCursorScreenPos( btn_min );
                ImGui::PushID( (int)code_id + 2 );

                bool btn_clicked = false;
                bool btn_hovered = false;
                bool btn_active = false;

                if( is_clipboard_supported )
                {
                    btn_clicked = ImGui::InvisibleButton( "##copy_btn", ImVec2( fixed_copy_w, control_h ) );
                    btn_hovered = ImGui::IsItemHovered();
                    btn_active = ImGui::IsItemActive();

                    if( btn_hovered )
                    {
                        ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    }

                    if( btn_clicked )
                    {
                        std::string clip_text;
                        if( fence_indent > 0 )
                        {
                            const char* cp = code_;
                            const char* cend = code_ + codeLength_;
                            while( cp < cend )
                            {
                                const char* ls = cp;
                                while( cp < cend && *cp != '\n' && *cp != '\r' ) ++cp;
                                const char* le = cp;
                                int stripped = 0;
                                while( ls < le && stripped < fence_indent && ( *ls == ' ' || *ls == '\t' ) )
                                {
                                    stripped += ( *ls == '\t' ) ? 4 : 1;
                                    ++ls;
                                }
                                clip_text.append( ls, le - ls );
                                if( cp < cend && *cp == '\r' ) { clip_text.push_back( *cp ); ++cp; }
                                if( cp < cend && *cp == '\n' ) { clip_text.push_back( *cp ); ++cp; }
                            }
                        }
                        else
                        {
                            clip_text.assign( code_, codeLength_ );
                        }

                        if( mdConfig_.setClipboardTextFn )
                        {
                            mdConfig_.setClipboardTextFn( clip_text.c_str(), mdConfig_.userData );
                        }
                        ImGui::SetClipboardText( clip_text.c_str() );
                        if( storage )
                        {
                            storage->SetFloat( code_id, now );
                        }
                        is_recently_copied = true;
                        MarkdownDetail::ClearActiveTooltip();
                    }

                    // Rich TimedTooltip when clipboard is active
                    if( !is_recently_copied )
                    {
                        const char* copy_tip = is_markdown
                            ? "Copies raw markdown plain text to your clipboard (unformatted text)."
                            : "Copies the contents of this code block to your clipboard.";
                        MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 2, btn_hovered, copy_tip, false );
                    }
                }
                else
                {
                    ImGui::InvisibleButton( "##copy_btn_disabled", ImVec2( fixed_copy_w, control_h ) );
                    btn_hovered = ImGui::IsItemHovered();

                    if( MarkdownDetail::GetDisabledCopyTooltipFn() )
                    {
                        MarkdownDetail::GetDisabledCopyTooltipFn()( ImGui::GetItemID(), btn_hovered );
                    }
                    else if( btn_hovered )
                    {
                        ImGui::SetTooltip( "System clipboard integration is not enabled on this platform build." );
                    }
                }

                // Render Actionable Button Visuals:
                ImU32 btn_bg, btn_border, btn_text_col, icon_col;
                if( !is_clipboard_supported )
                {
                    btn_bg = IM_COL32( 36, 38, 44, 140 );
                    btn_border = IM_COL32( 55, 58, 66, 160 );
                    btn_text_col = IM_COL32( 115, 120, 130, 160 );
                    icon_col = IM_COL32( 105, 110, 120, 150 );
                }
                else if( is_recently_copied )
                {
                    btn_bg = IM_COL32( 28, 92, 45, 230 );
                    btn_border = IM_COL32( 45, 140, 70, 240 );
                    btn_text_col = IM_COL32( 180, 250, 195, 255 );
                    icon_col = IM_COL32( 180, 250, 195, 255 );
                }
                else if( btn_active )
                {
                    btn_bg = IM_COL32( 65, 70, 84, 255 );
                    btn_border = IM_COL32( 95, 105, 125, 255 );
                    btn_text_col = IM_COL32( 240, 243, 250, 255 );
                    icon_col = IM_COL32( 220, 230, 245, 255 );
                }
                else if( btn_hovered )
                {
                    btn_bg = IM_COL32( 52, 56, 68, 240 );
                    btn_border = IM_COL32( 85, 95, 115, 240 );
                    btn_text_col = IM_COL32( 235, 240, 248, 255 );
                    icon_col = IM_COL32( 210, 220, 235, 240 );
                }
                else
                {
                    btn_bg = IM_COL32( 40, 42, 50, 210 );
                    btn_border = IM_COL32( 65, 70, 82, 180 );
                    btn_text_col = IM_COL32( 195, 202, 215, 230 );
                    icon_col = IM_COL32( 165, 175, 192, 210 );
                }

                float btn_rounding = 3.5f * scale;
                dl->AddRectFilled( btn_min, btn_max, btn_bg, btn_rounding );
                dl->AddRect( btn_min, btn_max, btn_border, btn_rounding, 0, 1.0f * scale );

                // Layout icon & text centered inside button:
                ImVec2 cur_text_sz = ImGui::CalcTextSize( copy_label );
                float total_inner_w = icon_w + icon_gap + cur_text_sz.x;
                float start_inner_x = btn_min.x + ( fixed_copy_w - total_inner_w ) * 0.5f;
                float icon_x = start_inner_x;
                float icon_y = btn_min.y + ( control_h - 10.0f * scale ) * 0.5f;

                // Draw Icon:
                if( is_recently_copied )
                {
                    // Checkmark vector icon
                    ImVec2 p1( icon_x, icon_y + 5.5f * scale );
                    ImVec2 p2( icon_x + 3.2f * scale, icon_y + 8.5f * scale );
                    ImVec2 p3( icon_x + 9.0f * scale, icon_y + 1.5f * scale );
                    dl->AddLine( p1, p2, icon_col, 1.5f * scale );
                    dl->AddLine( p2, p3, icon_col, 1.5f * scale );
                }
                else
                {
                    // Classic overlapping double-sheet copy icon
                    // Back page:
                    ImVec2 b_min( icon_x + 2.5f * scale, icon_y );
                    ImVec2 b_max( icon_x + 9.5f * scale, icon_y + 7.5f * scale );
                    dl->AddRect( b_min, b_max, icon_col, 1.0f * scale, 0, 1.0f * scale );

                    // Front page:
                    ImVec2 f_min( icon_x, icon_y + 2.5f * scale );
                    ImVec2 f_max( icon_x + 7.0f * scale, icon_y + 10.0f * scale );
                    dl->AddRectFilled( f_min, f_max, btn_bg, 1.0f * scale ); // mask out back sheet
                    dl->AddRect( f_min, f_max, icon_col, 1.0f * scale, 0, 1.0f * scale );
                }

                // Draw Label:
                float text_x = start_inner_x + icon_w + icon_gap;
                float text_y = btn_min.y + ( control_h - font_sz ) * 0.5f;
                dl->AddText( ImVec2( text_x, text_y ), btn_text_col, copy_label );

                ImGui::PopID();
            }

            // 3. Markdown Mode Toggle: Raw [===O] Preview
            if( is_markdown )
            {
                float toggle_x = ( mdConfig_.showCodeCopyButton ? cur_copy_x : ( box_max.x - pad_x ) ) - toggle_total_w - 14.0f * scale;

                ImGui::PushID( (int)code_id + 1 );
                ImVec2 toggle_start( toggle_x, control_y );

                // Zone 1: Raw text button
                ImGui::SetCursorScreenPos( toggle_start );
                if( ImGui::InvisibleButton( "##raw_zone", ImVec2( raw_sz.x + 2.0f * scale, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool raw_hovered = ImGui::IsItemHovered();
                if( raw_hovered ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 4, raw_hovered, "Switch to raw markdown text format (Unrendered plain text with markdown syntax).", false );

                // Zone 2: Switch track (middle)
                ImVec2 track_pos( toggle_start.x + raw_sz.x + 2.0f * scale + toggle_gap, control_y + ( control_h - toggle_track_h ) * 0.5f );
                ImGui::SetCursorScreenPos( ImVec2( track_pos.x, control_y ) );
                if( ImGui::InvisibleButton( "##track_zone", ImVec2( toggle_track_w, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool track_hovered = ImGui::IsItemHovered();
                if( track_hovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    if( !ImGui::Contrib::IsCurrentWindowScrollableY() )
                    {
                        ImGuiIO& io = ImGui::GetIO();
                        float wheel = ( io.MouseWheel != 0.0f ) ? io.MouseWheel : io.MouseWheelH;
                        if( wheel > 0.0f && !is_preview )
                        {
                            is_preview = true;
                            if( storage )
                            {
                                storage->SetBool( code_id, true );
                                storage->SetBool( preview_id, true );
                            }
                            io.MouseWheel = 0.0f;
                            io.MouseWheelH = 0.0f;
                            MarkdownDetail::ClearActiveTooltip();
                        }
                        else if( wheel < 0.0f && is_preview )
                        {
                            is_preview = false;
                            if( storage )
                            {
                                storage->SetBool( code_id, false );
                                storage->SetBool( preview_id, false );
                            }
                            io.MouseWheel = 0.0f;
                            io.MouseWheelH = 0.0f;
                            MarkdownDetail::ClearActiveTooltip();
                        }
                    }
                }
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 1, track_hovered, "Toggle between raw source markdown and rendered preview (Click or mouse wheel).", false );

                // Zone 3: Preview text button
                ImVec2 prev_pos( track_pos.x + toggle_track_w + toggle_gap, control_y );
                ImGui::SetCursorScreenPos( prev_pos );
                if( ImGui::InvisibleButton( "##prev_zone", ImVec2( prev_sz.x + 2.0f * scale, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool prev_hovered = ImGui::IsItemHovered();
                if( prev_hovered ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 5, prev_hovered, "Switch to live rendered markdown preview (Formatted headings, lists, pills, and styles).", false );

                // Animation smoothing
                ImGuiID anim_id = ImGui::GetID( "##raw_prev_anim" );
                float* p_t = storage ? storage->GetFloatRef( anim_id, is_preview ? 1.0f : 0.0f ) : nullptr;
                float target_t = is_preview ? 1.0f : 0.0f;
                if( p_t )
                {
                    float speed = 12.0f;
                    float dt = ImGui::GetIO().DeltaTime;
                    if( *p_t < target_t ) *p_t = ImMin( target_t, *p_t + speed * dt );
                    else if( *p_t > target_t ) *p_t = ImMax( target_t, *p_t - speed * dt );
                }
                float t = p_t ? *p_t : target_t;

                // Visual Rendering:
                // Text "Raw"
                ImU32 raw_text_col = !is_preview ? IM_COL32( 240, 243, 250, 255 ) : IM_COL32( 135, 140, 150, 180 );
                float r_font_sz = ( regular_font && regular_font != mono_font ) ? regular_font_sz : font_sz;
                float raw_y = control_y + ( control_h - r_font_sz ) * 0.5f;
                if( regular_font && regular_font != mono_font )
                {
                    dl->AddText( regular_font, r_font_sz, ImVec2( toggle_start.x, raw_y ), raw_text_col, "Raw" );
                }
                else
                {
                    dl->AddText( ImVec2( toggle_start.x, raw_y ), raw_text_col, "Raw" );
                }

                // Track pill
                float track_r = toggle_track_h * 0.5f;
                ImVec2 pill_min = track_pos;
                ImVec2 pill_max( track_pos.x + toggle_track_w, track_pos.y + toggle_track_h );

                ImVec4 c_off( 0.16f, 0.17f, 0.20f, 0.90f );
                ImVec4 c_on( 0.15f, 0.35f, 0.65f, 0.95f );
                ImVec4 track_c(
                    c_off.x + ( c_on.x - c_off.x ) * t,
                    c_off.y + ( c_on.y - c_off.y ) * t,
                    c_off.z + ( c_on.z - c_off.z ) * t,
                    c_off.w + ( c_on.w - c_off.w ) * t
                );
                ImU32 track_col = ImGui::ColorConvertFloat4ToU32( track_c );
                dl->AddRectFilled( pill_min, pill_max, track_col, track_r );
                dl->AddRect( pill_min, pill_max, IM_COL32( 70, 74, 85, 180 ), track_r, 0, 1.0f * scale );

                // Sliding thumb
                float thumb_r = track_r - 1.25f * scale;
                if( thumb_r < 2.0f ) thumb_r = 2.0f;
                float thumb_x_min = pill_min.x + track_r;
                float thumb_x_max = pill_max.x - track_r;
                float thumb_x = thumb_x_min + ( thumb_x_max - thumb_x_min ) * t;
                float thumb_y = pill_min.y + track_r;
                dl->AddCircleFilled( ImVec2( thumb_x, thumb_y ), thumb_r, IM_COL32( 250, 250, 252, 255 ) );
                dl->AddCircle( ImVec2( thumb_x, thumb_y ), thumb_r, IM_COL32( 0, 0, 0, 60 ), 0, 1.0f * scale );

                // Text "Preview"
                ImU32 prev_text_col = is_preview ? IM_COL32( 240, 243, 250, 255 ) : IM_COL32( 135, 140, 150, 180 );
                float prev_y = control_y + ( control_h - r_font_sz ) * 0.5f;
                if( regular_font && regular_font != mono_font )
                {
                    dl->AddText( regular_font, r_font_sz, ImVec2( prev_pos.x, prev_y ), prev_text_col, "Preview" );
                }
                else
                {
                    dl->AddText( ImVec2( prev_pos.x, prev_y ), prev_text_col, "Preview" );
                }

                ImGui::PopID();
            }
        }

        splitter.Merge( dl );

        if( mono_font )
        {
            ImGui::PopFont();
        }

        ImGui::PopID();

        // Align cursor to box bottom and submit spacing item cleanly with block_width
        ImGui::SetCursorScreenPos( ImVec2( start_pos.x, box_max.y ) );
        ImGui::Dummy( ImVec2( block_width, 4.0f * scale ) );
    }

    //-----------------------------------------------------------------------------
    // GitHub Flavored Markdown (GFM) Tables
    //-----------------------------------------------------------------------------

    enum class TableColumnAlign
    {
        LEFT,
        CENTER,
        RIGHT
    };

    struct TableCell
    {
        const char* start = nullptr;
        const char* end = nullptr;

        size_t size() const { return ( end >= start ) ? (size_t)( end - start ) : 0; }
        bool empty() const { return size() == 0; }
    };

    namespace TableDetail
    {
        inline bool SplitRow( const char* line_start, const char* line_end, std::vector<TableCell>& out_cells )
        {
            out_cells.clear();
            if( !line_start || line_start >= line_end ) return false;

            // 1. Trim leading spaces/tabs
            while( line_start < line_end && ( *line_start == ' ' || *line_start == '\t' ) ) ++line_start;
            // 2. Trim trailing spaces/tabs/newlines
            while( line_end > line_start && ( line_end[-1] == ' ' || line_end[-1] == '\t' || line_end[-1] == '\r' || line_end[-1] == '\n' ) ) --line_end;

            if( line_start >= line_end ) return false;

            // 3. Skip optional leading '|'
            if( *line_start == '|' )
            {
                ++line_start;
            }

            // 4. Strip optional trailing '|' (must not be escaped with '\')
            if( line_end > line_start && line_end[-1] == '|' )
            {
                bool is_escaped = false;
                const char* check = line_end - 2;
                while( check >= line_start && *check == '\\' )
                {
                    is_escaped = !is_escaped;
                    --check;
                }
                if( !is_escaped )
                {
                    --line_end;
                }
            }

            // 5. Scan cells split by unescaped '|' (respecting backticks)
            const char* cell_start = line_start;
            bool in_code = false;
            bool is_escaped = false;

            for( const char* p = line_start; p < line_end; ++p )
            {
                char c = *p;
                if( is_escaped )
                {
                    is_escaped = false;
                    continue;
                }
                if( c == '\\' )
                {
                    is_escaped = true;
                    continue;
                }
                if( c == '`' )
                {
                    in_code = !in_code;
                    continue;
                }
                if( c == '|' && !in_code )
                {
                    const char* cs = cell_start;
                    const char* ce = p;
                    while( cs < ce && ( *cs == ' ' || *cs == '\t' ) ) ++cs;
                    while( ce > cs && ( ce[-1] == ' ' || ce[-1] == '\t' ) ) --ce;
                    TableCell cell;
                    cell.start = cs;
                    cell.end = ce;
                    out_cells.push_back( cell );
                    cell_start = p + 1;
                }
            }

            // Final cell
            const char* cs = cell_start;
            const char* ce = line_end;
            while( cs < ce && ( *cs == ' ' || *cs == '\t' ) ) ++cs;
            while( ce > cs && ( ce[-1] == ' ' || ce[-1] == '\t' ) ) --ce;
            TableCell last_cell;
            last_cell.start = cs;
            last_cell.end = ce;
            out_cells.push_back( last_cell );

            return !out_cells.empty();
        }

        inline bool ParseDelimiterCell( const TableCell& cell, TableColumnAlign& out_align )
        {
            if( cell.empty() ) return false;
            const char* start = cell.start;
            const char* end = cell.end;

            while( start < end && ( *start == ' ' || *start == '\t' ) ) ++start;
            while( end > start && ( end[-1] == ' ' || end[-1] == '\t' ) ) --end;
            if( start >= end ) return false;

            bool left_colon = ( *start == ':' );
            if( left_colon ) ++start;

            bool right_colon = ( end > start && end[-1] == ':' );
            if( right_colon ) --end;

            if( start >= end ) return false; // Must have at least one hyphen

            for( const char* k = start; k < end; ++k )
            {
                if( *k != '-' ) return false;
            }

            if( left_colon && right_colon )
            {
                out_align = TableColumnAlign::CENTER;
            }
            else if( right_colon )
            {
                out_align = TableColumnAlign::RIGHT;
            }
            else
            {
                out_align = TableColumnAlign::LEFT;
            }
            return true;
        }

        inline bool ParseDelimiterRow( const char* line_start, const char* line_end, std::vector<TableColumnAlign>& out_alignments )
        {
            out_alignments.clear();
            std::vector<TableCell> cells;
            if( !SplitRow( line_start, line_end, cells ) || cells.empty() ) return false;

            for( size_t i = 0; i < cells.size(); ++i )
            {
                TableColumnAlign align;
                if( !ParseDelimiterCell( cells[i], align ) )
                {
                    out_alignments.clear();
                    return false;
                }
                out_alignments.push_back( align );
            }
            return true;
        }

        inline bool CheckLineHasPipe( const char* line_start, const char* line_end )
        {
            bool in_code = false;
            bool is_escaped = false;
            for( const char* p = line_start; p < line_end; ++p )
            {
                char c = *p;
                if( is_escaped ) { is_escaped = false; continue; }
                if( c == '\\' ) { is_escaped = true; continue; }
                if( c == '`' ) { in_code = !in_code; continue; }
                if( c == '|' && !in_code ) return true;
            }
            return false;
        }

        inline int FindLineEnd( const char* md, int start, int len )
        {
            int pos = start;
            while( pos < len && md[pos] != '\r' && md[pos] != '\n' )
            {
                ++pos;
            }
            return pos;
        }

        inline int SkipNewline( const char* md, int line_end, int len )
        {
            int pos = line_end;
            if( pos < len && md[pos] == '\r' && pos + 1 < len && md[pos + 1] == '\n' )
            {
                return pos + 2;
            }
            else if( pos < len && ( md[pos] == '\r' || md[pos] == '\n' ) )
            {
                return pos + 1;
            }
            return pos;
        }

        inline bool StrCaseEqualN( const char* a, const char* b, size_t n )
        {
            for( size_t i = 0; i < n; ++i )
            {
                if( tolower( (unsigned char)a[i] ) != tolower( (unsigned char)b[i] ) )
                    return false;
            }
            return true;
        }

        inline float CalcCellVisualWidth( const TableCell& cell, bool is_header, const MarkdownConfig& mdConfig_ )
        {
            if( cell.empty() ) return 0.0f;

            const char* p = cell.start;
            const char* end = cell.end;
            while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
            while( end > p && ( end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n' ) ) --end;
            if( p >= end ) return 0.0f;

            float scale = ImGui::GetVisualScale();

            // If cell begins with an HTML-style tag: <tag_name ...>inner_text</tag_name>
            if( *p == '<' )
            {
                const char* tag_close = (const char*)memchr( p, '>', end - p );
                if( tag_close && tag_close < end )
                {
                    // Find tag name
                    const char* tn_start = p + 1;
                    while( tn_start < tag_close && ( *tn_start == ' ' || *tn_start == '\t' ) ) ++tn_start;
                    const char* tn_end = tn_start;
                    while( tn_end < tag_close && *tn_end != ' ' && *tn_end != '=' && *tn_end != '\t' ) ++tn_end;

                    size_t tn_len = (size_t)( tn_end - tn_start );
                    std::string tag_name( tn_start, tn_len );
                    for( size_t k = 0; k < tag_name.size(); ++k )
                        tag_name[k] = (char)tolower( (unsigned char)tag_name[k] );

                    // Look for closing tag </tag_name>
                    std::string close_tag = "</" + tag_name + ">";
                    const char* content_start = tag_close + 1;
                    const char* content_end = nullptr;
                    const char* found_close = nullptr;

                    for( const char* s = content_start; s + close_tag.length() <= end; ++s )
                    {
                        if( StrCaseEqualN( s, close_tag.c_str(), close_tag.length() ) )
                        {
                            content_end = s;
                            found_close = s;
                            break;
                        }
                    }

                    if( found_close )
                    {
                        // Check font for inner text
                        ImFont* chosen_font = nullptr;
                        if( is_header )
                        {
                            if( mdConfig_.boldFont ) chosen_font = mdConfig_.boldFont;
                            else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
                                chosen_font = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
                        }
                        else if( ( tag_name == "mono" || tag_name == "code" ) && mdConfig_.monoFont )
                        {
                            chosen_font = mdConfig_.monoFont;
                        }
                        else if( mdConfig_.regularFont )
                        {
                            chosen_font = mdConfig_.regularFont;
                        }

                        ImVec2 inner_sz;
                        if( chosen_font )
                        {
                            ImGui::PushFont( chosen_font );
                            inner_sz = ImGui::CalcTextSize( content_start, content_end );
                            ImGui::PopFont();
                        }
                        else
                        {
                            inner_sz = ImGui::CalcTextSize( content_start, content_end );
                        }

                        float visual_w = inner_sz.x;
                        if( tag_name == "pill" )
                        {
                            float pad_x = 7.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "badge" || tag_name == "tag" )
                        {
                            float pad_x = 5.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "kbd" || tag_name == "key" || tag_name == "kdb" )
                        {
                            float pad_x = 5.5f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "btn" )
                        {
                            visual_w += ImGui::GetStyle().FramePadding.x * 2.0f * scale;
                        }
                        else if( tag_name == "mono" || tag_name == "code" || tag_name == "backdrop" || tag_name == "bg" || tag_name == "highlight" )
                        {
                            float pad_x = 3.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }

                        // Any trailing content after closing tag? (e.g. "<pill>OK</pill> extra")
                        const char* after = found_close + close_tag.length();
                        while( after < end && ( *after == ' ' || *after == '\t' ) ) ++after;
                        if( after < end )
                        {
                            visual_w += ImGui::GetStyle().ItemSpacing.x * scale + ImGui::CalcTextSize( after, end ).x;
                        }

                        return visual_w;
                    }
                }
            }

            // Check if markdown link [text](url) or {text}(url)
            if( ( *p == '[' || *p == '{' ) )
            {
                char close_b = ( *p == '[' ) ? ']' : '}';
                const char* b_close = (const char*)memchr( p, close_b, end - p );
                if( b_close && b_close + 1 < end && *(b_close + 1) == '(' )
                {
                    const char* link_text_start = p + 1;
                    const char* link_text_end = b_close;
                    if( is_header && mdConfig_.boldFont )
                    {
                        ImGui::PushFont( mdConfig_.boldFont );
                        ImVec2 sz = ImGui::CalcTextSize( link_text_start, link_text_end );
                        ImGui::PopFont();
                        return sz.x;
                    }
                    return ImGui::CalcTextSize( link_text_start, link_text_end ).x;
                }
            }

            // Markdown formatting symbols: strip leading/trailing **, *, __, _, ~~, ++
            const char* s = p;
            const char* e = end;
            while( s < e && ( *s == ' ' || *s == '\t' ) ) ++s;
            while( e > s && ( e[-1] == ' ' || e[-1] == '\t' ) ) --e;

            bool is_bold = ( ( e - s >= 4 && s[0] == '*' && s[1] == '*' && e[-1] == '*' && e[-2] == '*' ) ||
                             ( e - s >= 4 && s[0] == '_' && s[1] == '_' && e[-1] == '_' && e[-2] == '_' ) );
            bool is_mono = ( e - s >= 2 && s[0] == '`' && e[-1] == '`' );

            while( e > s && ( e[-1] == '*' || e[-1] == '_' || e[-1] == '~' || e[-1] == '+' || e[-1] == '`' ) ) --e;
            while( s < e && ( *s == '*' || *s == '_' || *s == '~' || *s == '+' || *s == '`' ) ) ++s;
            while( s < e && ( *s == ' ' || *s == '\t' ) ) ++s;
            while( e > s && ( e[-1] == ' ' || e[-1] == '\t' ) ) --e;

            ImFont* chosen_font = nullptr;
            if( is_header )
            {
                if( mdConfig_.boldFont ) chosen_font = mdConfig_.boldFont;
                else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
                    chosen_font = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
            }
            else if( is_bold && mdConfig_.boldFont )
            {
                chosen_font = mdConfig_.boldFont;
            }
            else if( is_mono && mdConfig_.monoFont )
            {
                chosen_font = mdConfig_.monoFont;
            }
            else if( mdConfig_.regularFont )
            {
                chosen_font = mdConfig_.regularFont;
            }

            ImVec2 sz;
            if( chosen_font )
            {
                ImGui::PushFont( chosen_font );
                sz = ImGui::CalcTextSize( s, e );
                ImGui::PopFont();
            }
            else
            {
                sz = ImGui::CalcTextSize( s, e );
            }

            if( is_mono )
            {
                sz.x += 6.0f * scale; // monospace code padding
            }

            return sz.x;
        }

        inline std::string GetCellPlainText( const TableCell& cell )
        {
            if( cell.empty() ) return "";
            const char* p = cell.start;
            const char* end = cell.end;
            while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
            while( end > p && ( end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n' ) ) --end;

            std::string out;
            out.reserve( (size_t)( end - p ) );
            while( p < end )
            {
                if( *p == '<' )
                {
                    const char* close = (const char*)memchr( p, '>', end - p );
                    if( close && close < end )
                    {
                        p = close + 1;
                        continue;
                    }
                }
                if( *p != '*' && *p != '_' && *p != '`' && *p != '~' )
                {
                    out.push_back( *p );
                }
                ++p;
            }
            size_t s = 0;
            while( s < out.size() && ( out[s] == ' ' || out[s] == '\t' ) ) ++s;
            size_t e = out.size();
            while( e > s && ( out[e - 1] == ' ' || out[e - 1] == '\t' ) ) --e;
            return out.substr( s, e - s );
        }

        inline int NaturalCompare( const std::string& a, const std::string& b )
        {
            size_t i = 0, j = 0;
            while( i < a.size() && j < b.size() )
            {
                // Skip thousand-separator commas in numbers (e.g. 5,500 vs 12,000)
                if( a[i] == ',' && i + 1 < a.size() && isdigit((unsigned char)a[i+1]) && i > 0 && isdigit((unsigned char)a[i-1]) )
                {
                    i++;
                    continue;
                }
                if( b[j] == ',' && j + 1 < b.size() && isdigit((unsigned char)b[j+1]) && j > 0 && isdigit((unsigned char)b[j-1]) )
                {
                    j++;
                    continue;
                }

                bool a_is_num = isdigit( (unsigned char)a[i] ) || ( ( a[i] == '+' || a[i] == '-' ) && i + 1 < a.size() && isdigit( (unsigned char)a[i+1] ) );
                bool b_is_num = isdigit( (unsigned char)b[j] ) || ( ( b[j] == '+' || b[j] == '-' ) && j + 1 < b.size() && isdigit( (unsigned char)b[j+1] ) );

                if( a_is_num && b_is_num )
                {
                    std::string subA, subB;
                    size_t k = i;
                    while( k < a.size() && ( isdigit((unsigned char)a[k]) || a[k] == '.' || a[k] == '+' || a[k] == '-' || a[k] == ',' ) )
                    {
                        if( a[k] != ',' ) subA.push_back( a[k] );
                        k++;
                    }
                    size_t advA = k - i;
                    k = j;
                    while( k < b.size() && ( isdigit((unsigned char)b[k]) || b[k] == '.' || b[k] == '+' || b[k] == '-' || b[k] == ',' ) )
                    {
                        if( b[k] != ',' ) subB.push_back( b[k] );
                        k++;
                    }
                    size_t advB = k - j;
                    char* endA = nullptr;
                    char* endB = nullptr;
                    double vA = strtod( subA.c_str(), &endA );
                    double vB = strtod( subB.c_str(), &endB );
                    if( vA < vB ) return -1;
                    if( vA > vB ) return 1;
                    i += advA;
                    j += advB;
                    continue;
                }

                char ca = (char)tolower( (unsigned char)a[i] );
                char cb = (char)tolower( (unsigned char)b[j] );
                if( ca < cb ) return -1;
                if( ca > cb ) return 1;
                i++;
                j++;
            }
            if( i < a.size() ) return 1;
            if( j < b.size() ) return -1;
            return 0;
        }

        inline int CompareCells( const TableCell& a, const TableCell& b )
        {
            if( a.empty() && b.empty() ) return 0;
            if( a.empty() ) return 1;
            if( b.empty() ) return -1;

            std::string strA = GetCellPlainText( a );
            std::string strB = GetCellPlainText( b );
            return NaturalCompare( strA, strB );
        }

        inline bool StrCaseEqual( const std::string& a, const std::string& b )
        {
            if( a.size() != b.size() ) return false;
            for( size_t i = 0; i < a.size(); ++i )
            {
                if( tolower( (unsigned char)a[i] ) != tolower( (unsigned char)b[i] ) )
                    return false;
            }
            return true;
        }

        struct MarkdownTableMetadata
        {
            // Author baseline preset (<!-- table-preset: ... -->)
            bool                hasPreset = false;
            float               presetWidth = 0.0f;
            int                 presetSortCol = -1;
            int                 presetSortDir = 0;
            std::string         presetSortColName;
            std::vector<float>  presetColWidths;

            // User active / saved state (<!-- table-state: ... --> or <!-- table: ... -->)
            bool                hasState = false;
            float               stateWidth = 0.0f;
            int                 stateSortCol = -1;
            int                 stateSortDir = 0;
            std::string         stateSortColName;
            std::vector<float>  stateColWidths;

            // Byte offsets within markdown_
            int                 tableStart = 0;
            int                 rowsEnd = 0;
            int                 presetCommentStart = -1;
            int                 presetCommentEnd = -1;
            int                 stateCommentStart = -1;
            int                 stateCommentEnd = -1;
            int                 fullTableEnd = 0;
        };

        inline void ParseTableDirective(
            const char* p_start, size_t len,
            bool is_preset,
            MarkdownTableMetadata& meta,
            const std::vector<TableCell>& headers )
        {
            const char* p = p_start;
            const char* end = p_start + len;

            float& target_w = is_preset ? meta.presetWidth : meta.stateWidth;
            int& target_sort_col = is_preset ? meta.presetSortCol : meta.stateSortCol;
            int& target_sort_dir = is_preset ? meta.presetSortDir : meta.stateSortDir;
            std::string& target_col_name = is_preset ? meta.presetSortColName : meta.stateSortColName;
            std::vector<float>& target_cols = is_preset ? meta.presetColWidths : meta.stateColWidths;

            if( is_preset ) meta.hasPreset = true;
            else meta.hasState = true;

            while( p < end )
            {
                while( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',' || *p == ';' ) ) ++p;
                if( p >= end ) break;

                const char* tok_start = p;
                while( p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '=' && *p != ',' && *p != ';' ) ++p;
                std::string key( tok_start, (size_t)( p - tok_start ) );
                for( char& c : key ) c = (char)tolower( (unsigned char)c );

                std::string val;
                if( p < end && *p == '=' )
                {
                    ++p; // Skip '='
                    while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
                    if( p < end && ( *p == '"' || *p == '\'' ) )
                    {
                        char q = *p++;
                        const char* v_start = p;
                        while( p < end && *p != q ) ++p;
                        val.assign( v_start, (size_t)( p - v_start ) );
                        if( p < end && *p == q ) ++p;
                    }
                    else
                    {
                        const char* v_start = p;
                        while( p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != ',' && *p != ';' ) ++p;
                        val.assign( v_start, (size_t)( p - v_start ) );
                    }
                }

                if( key == "width" || key == "w" )
                {
                    if( !val.empty() )
                    {
                        if( val == "auto" || val == "default" )
                        {
                            target_w = 0.0f;
                        }
                        else
                        {
                            target_w = (float)atof( val.c_str() );
                        }
                    }
                }
                else if( key == "cols" || key == "columns" )
                {
                    target_cols.clear();
                    const char* cp = val.c_str();
                    while( *cp )
                    {
                        while( *cp == ' ' || *cp == ',' || *cp == ';' ) ++cp;
                        if( !*cp ) break;
                        float cw = (float)atof( cp );
                        target_cols.push_back( cw );
                        while( *cp && *cp != ',' && *cp != ';' && *cp != ' ' ) ++cp;
                    }
                }
                else if( key == "sort" )
                {
                    if( !val.empty() )
                    {
                        if( val == "none" || val == "default" || val == "-1" )
                        {
                            target_sort_col = -1;
                            target_sort_dir = 0;
                            target_col_name.clear();
                        }
                        else
                        {
                            bool is_num = true;
                            for( size_t k = 0; k < val.size(); ++k )
                            {
                                if( k == 0 && ( val[k] == '-' || val[k] == '+' ) ) continue;
                                if( !isdigit( (unsigned char)val[k] ) ) { is_num = false; break; }
                            }

                            if( is_num )
                            {
                                int idx = atoi( val.c_str() );
                                target_sort_col = idx;
                                if( idx >= 0 && (size_t)idx < headers.size() )
                                {
                                    target_col_name = GetCellPlainText( headers[idx] );
                                }
                            }
                            else
                            {
                                target_col_name = val;
                                target_sort_col = -1;
                                for( size_t c = 0; c < headers.size(); ++c )
                                {
                                    std::string h_name = GetCellPlainText( headers[c] );
                                    if( StrCaseEqual( h_name, val ) )
                                    {
                                        target_sort_col = (int)c;
                                        break;
                                    }
                                }
                            }

                            if( target_sort_dir == 0 )
                            {
                                target_sort_dir = 1;
                            }
                        }
                    }
                }
                else if( key == "dir" || key == "direction" )
                {
                    std::string d = val;
                    for( char& c : d ) c = (char)tolower( (unsigned char)c );
                    if( d == "asc" || d == "ascending" || d == "up" )
                        target_sort_dir = 1;
                    else if( d == "desc" || d == "descending" || d == "down" )
                        target_sort_dir = 2;
                    else if( d == "none" || d == "default" )
                        target_sort_dir = 0;
                }
                else if( key == "asc" || key == "ascending" )
                {
                    target_sort_dir = 1;
                }
                else if( key == "desc" || key == "descending" )
                {
                    target_sort_dir = 2;
                }
                else if( key == "none" )
                {
                    target_sort_dir = 0;
                }
                else if( key == "default" || key == "preset" )
                {
                    meta.hasPreset = true;
                }
                else if( key == "user" || key == "state" )
                {
                    meta.hasState = true;
                }
            }
        }

        // Checks if current line is the start of a GFM Markdown Table.
        // Returns the end byte offset of the table (including attached metadata directives) if valid, or -1 if not a table.
        inline int TryParseTable(
            const char* md, int md_len, int line_start,
            std::vector<TableCell>& out_headers,
            std::vector<TableColumnAlign>& out_alignments,
            std::vector<std::vector<TableCell>>& out_rows,
            MarkdownTableMetadata& out_meta )
        {
            out_headers.clear();
            out_alignments.clear();
            out_rows.clear();
            out_meta = MarkdownTableMetadata();
            out_meta.tableStart = line_start;

            // 1. Current line (Header row candidate)
            int line1_end = FindLineEnd( md, line_start, md_len );
            if( !CheckLineHasPipe( md + line_start, md + line1_end ) )
            {
                return -1;
            }

            // 2. Next line (Delimiter row candidate)
            int line2_start = SkipNewline( md, line1_end, md_len );
            if( line2_start >= md_len ) return -1;

            int line2_lead_spaces = 0;
            while( line2_start + line2_lead_spaces < md_len && 
                   ( md[line2_start + line2_lead_spaces] == ' ' || md[line2_start + line2_lead_spaces] == '\t' ) )
            {
                line2_lead_spaces += ( md[line2_start + line2_lead_spaces] == '\t' ) ? 4 : 1;
            }
            if( line2_lead_spaces > 3 ) return -1;

            int line2_end = FindLineEnd( md, line2_start, md_len );
            if( !ParseDelimiterRow( md + line2_start, md + line2_end, out_alignments ) || out_alignments.empty() )
            {
                return -1;
            }

            if( !SplitRow( md + line_start, md + line1_end, out_headers ) || out_headers.empty() )
            {
                return -1;
            }

            size_t num_cols = out_alignments.size();
            while( out_headers.size() < num_cols )
            {
                TableCell empty_cell;
                empty_cell.start = md + line1_end;
                empty_cell.end = md + line1_end;
                out_headers.push_back( empty_cell );
            }
            if( out_headers.size() > num_cols )
            {
                out_headers.resize( num_cols );
            }

            // 3. Subsequent Data Rows
            int cur_line_start = SkipNewline( md, line2_end, md_len );
            int table_end = line2_end;

            while( cur_line_start < md_len )
            {
                int cur_line_end = FindLineEnd( md, cur_line_start, md_len );

                bool is_blank = true;
                for( int k = cur_line_start; k < cur_line_end; ++k )
                {
                    if( md[k] != ' ' && md[k] != '\t' && md[k] != '\r' )
                    {
                        is_blank = false;
                        break;
                    }
                }
                if( is_blank )
                {
                    break;
                }

                int lead_sp = 0;
                while( cur_line_start + lead_sp < cur_line_end && ( md[cur_line_start + lead_sp] == ' ' || md[cur_line_start + lead_sp] == '\t' ) )
                {
                    ++lead_sp;
                }
                char first_c = ( cur_line_start + lead_sp < cur_line_end ) ? md[cur_line_start + lead_sp] : '\0';

                // Heading or code fence interrupts table
                if( first_c == '#' || first_c == '`' || first_c == '~' )
                {
                    break;
                }

                if( !CheckLineHasPipe( md + cur_line_start, md + cur_line_end ) )
                {
                    break;
                }

                std::vector<TableCell> row_cells;
                if( SplitRow( md + cur_line_start, md + cur_line_end, row_cells ) )
                {
                    while( row_cells.size() < num_cols )
                    {
                        TableCell empty_cell;
                        empty_cell.start = md + cur_line_end;
                        empty_cell.end = md + cur_line_end;
                        row_cells.push_back( empty_cell );
                    }
                    if( row_cells.size() > num_cols )
                    {
                        row_cells.resize( num_cols );
                    }
                    out_rows.push_back( row_cells );
                }

                table_end = cur_line_end;
                cur_line_start = SkipNewline( md, cur_line_end, md_len );
            }

            out_meta.rowsEnd = table_end;
            out_meta.fullTableEnd = table_end;

            // 4. Scan for attached table metadata comment directives (<!-- table-preset: ... --> and/or <!-- table-state: ... -->)
            while( cur_line_start < md_len )
            {
                int check_line_end = FindLineEnd( md, cur_line_start, md_len );
                int p = cur_line_start;
                while( p < check_line_end && ( md[p] == ' ' || md[p] == '\t' ) ) ++p;

                if( p + 4 <= check_line_end && strncmp( md + p, "<!--", 4 ) == 0 )
                {
                    int content_p = p + 4;
                    while( content_p < check_line_end && ( md[content_p] == ' ' || md[content_p] == '\t' ) ) ++content_p;

                    bool is_table_directive = false;
                    bool is_preset = false;

                    if( StrCaseEqualN( md + content_p, "table-preset:", 13 ) )
                    {
                        is_table_directive = true;
                        is_preset = true;
                        content_p += 13;
                    }
                    else if( StrCaseEqualN( md + content_p, "table-state:", 12 ) )
                    {
                        is_table_directive = true;
                        is_preset = false;
                        content_p += 12;
                    }
                    else if( StrCaseEqualN( md + content_p, "table:", 6 ) )
                    {
                        is_table_directive = true;
                        content_p += 6;
                        while( content_p < check_line_end && ( md[content_p] == ' ' || md[content_p] == '\t' ) ) ++content_p;
                        if( StrCaseEqualN( md + content_p, "default", 7 ) )
                        {
                            is_preset = true;
                            content_p += 7;
                        }
                        else if( StrCaseEqualN( md + content_p, "preset", 6 ) )
                        {
                            is_preset = true;
                            content_p += 6;
                        }
                        else if( StrCaseEqualN( md + content_p, "user", 4 ) )
                        {
                            is_preset = false;
                            content_p += 4;
                        }
                        else if( StrCaseEqualN( md + content_p, "state", 5 ) )
                        {
                            is_preset = false;
                            content_p += 5;
                        }
                        else
                        {
                            is_preset = false;
                        }
                    }

                    if( is_table_directive )
                    {
                        int close_pos = -1;
                        for( int k = content_p; k + 3 <= md_len; ++k )
                        {
                            if( md[k] == '-' && md[k+1] == '-' && md[k+2] == '>' )
                            {
                                close_pos = k;
                                break;
                            }
                        }
                        if( close_pos != -1 )
                        {
                            int comment_end = close_pos + 3;
                            int comment_line_end = FindLineEnd( md, comment_end, md_len );

                            ParseTableDirective( md + content_p, (size_t)( close_pos - content_p ), is_preset, out_meta, out_headers );

                            if( is_preset )
                            {
                                out_meta.presetCommentStart = cur_line_start;
                                out_meta.presetCommentEnd = comment_line_end;
                            }
                            else
                            {
                                out_meta.stateCommentStart = cur_line_start;
                                out_meta.stateCommentEnd = comment_line_end;
                            }

                            table_end = comment_line_end;
                            out_meta.fullTableEnd = comment_line_end;
                            cur_line_start = SkipNewline( md, comment_line_end, md_len );
                            continue;
                        }
                    }
                }
                break;
            }

            return table_end;
        }

        inline int TryParseTable(
            const char* md, int md_len, int line_start,
            std::vector<TableCell>& out_headers,
            std::vector<TableColumnAlign>& out_alignments,
            std::vector<std::vector<TableCell>>& out_rows )
        {
            MarkdownTableMetadata dummy_meta;
            return TryParseTable( md, md_len, line_start, out_headers, out_alignments, out_rows, dummy_meta );
        }
    } // namespace TableDetail

    inline void RenderTableCell( const TableCell& cell, TableColumnAlign align, bool is_header, const MarkdownConfig& mdConfig_ )
    {
        if( cell.empty() )
        {
            return;
        }

        float avail = ImGui::GetContentRegionAvail().x;
        float text_w = TableDetail::CalcCellVisualWidth( cell, is_header, mdConfig_ );
        float scale = ImGui::GetVisualScale();

        // In Dear ImGui table cells, GetContentRegionAvail().x already accounts for style.CellPadding.x.
        // For header cells, reserve margin for the sort arrow (14px).
        // For body cells, reserve a 6px breathing gutter so right-aligned glyphs don't clip against the border.
        float right_margin = ( is_header ? 14.0f : 6.0f ) * scale;

        // Apply alignment offset if CENTER or RIGHT
        if( align == TableColumnAlign::RIGHT )
        {
            if( text_w + right_margin < avail )
            {
                float offset = avail - text_w - right_margin;
                if( offset > 0.0f )
                {
                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );
                }
            }
        }
        else if( align == TableColumnAlign::CENTER )
        {
            if( text_w < avail )
            {
                float offset = ( avail - text_w ) * 0.5f;
                if( offset > 0.0f )
                {
                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );
                }
            }
        }

        bool pushed_font = false;
        if( is_header )
        {
            if( mdConfig_.boldFont )
            {
                ImGui::PushFont( mdConfig_.boldFont );
                pushed_font = true;
            }
            else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
            {
                ImGui::PushFont( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font );
                pushed_font = true;
            }
        }

        MarkdownConfig cell_config = mdConfig_;
        // If text fits comfortably in the column, do NOT wrap (prevents right-aligned sub-pixel wrap anomalies)
        // If text genuinely exceeds available column width (e.g. user dragged divider to narrow column), enable wrapping!
        if( text_w + right_margin > avail + 0.5f )
        {
            cell_config.wrapWidth = avail;
        }
        else
        {
            cell_config.wrapWidth = -1.0f;
        }
        Markdown::Render( cell.start, cell.size(), cell_config );

        if( pushed_font )
        {
            ImGui::PopFont();
        }
    }

    inline void RenderTableBlock(
        const std::vector<TableCell>& headers,
        const std::vector<TableColumnAlign>& alignments,
        const std::vector<std::vector<TableCell>>& rows,
        const TableDetail::MarkdownTableMetadata& metadata,
        const MarkdownConfig& mdConfig_,
        size_t table_idx )
    {
        size_t num_cols = alignments.size();
        if( num_cols == 0 ) return;

        // 1. Stable table ID: hash header text + index so ID is 100% deterministic across frames
        ImGuiID header_hash = 0;
        for( size_t c = 0; c < headers.size(); ++c )
        {
            if( headers[c].start && headers[c].end > headers[c].start )
            {
                header_hash = ImHashData( headers[c].start, (size_t)( headers[c].end - headers[c].start ), header_hash );
            }
        }
        header_hash = ImHashData( &table_idx, sizeof(table_idx), header_hash );

        char table_id[64];
        snprintf( table_id, sizeof(table_id), "##md_table_%08x_%d", (unsigned int)header_hash, (int)table_idx );

        // Persistent sort & width state stored in ImGuiStorage for this deterministic table hash
        ImGuiID sort_col_key   = header_hash;
        ImGuiID sort_dir_key   = header_hash + 1;
        ImGuiID width_key      = header_hash + 2;
        ImGuiID reset_key      = header_hash + 3;
        ImGuiID last_width_key = header_hash + 4;
        ImGuiID init_key       = header_hash + 5;
        ImGuiID preset_w_key   = header_hash + 6;
        ImGuiID meta_hash_key  = header_hash + 7;
        ImGuiID dragging_key   = header_hash + 8;

        // Detect if metadata was loaded or edited in raw markdown
        ImGuiID current_meta_hash = 1;
        if( metadata.hasPreset )
        {
            current_meta_hash = ImHashData( &metadata.presetWidth, sizeof(float), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.presetSortCol, sizeof(int), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.presetSortDir, sizeof(int), current_meta_hash );
        }
        if( metadata.hasState )
        {
            current_meta_hash = ImHashData( &metadata.stateWidth, sizeof(float), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.stateSortCol, sizeof(int), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.stateSortDir, sizeof(int), current_meta_hash );
        }

        bool need_init = !ImGui::GetStateStorage()->GetBool( init_key, false ) ||
                         ( ImGui::GetStateStorage()->GetInt( meta_hash_key, 0 ) != (int)current_meta_hash );

        if( need_init )
        {
            int initial_sort_col = -1;
            int initial_sort_dir = (int)ImGuiSortDirection_None;
            float initial_norm_w = 0.0f;
            float preset_norm_w  = 0.0f;

            if( metadata.hasPreset )
            {
                preset_norm_w    = metadata.presetWidth;
                initial_norm_w   = preset_norm_w;
                initial_sort_col = metadata.presetSortCol;
                initial_sort_dir = metadata.presetSortDir;
            }
            if( metadata.hasState )
            {
                initial_norm_w = metadata.stateWidth;
                if( metadata.stateSortCol >= -1 ) initial_sort_col = metadata.stateSortCol;
                if( metadata.stateSortDir >= 0 )  initial_sort_dir = metadata.stateSortDir;
            }

            ImGui::GetStateStorage()->SetInt( sort_col_key, initial_sort_col );
            ImGui::GetStateStorage()->SetInt( sort_dir_key, initial_sort_dir );
            ImGui::GetStateStorage()->SetFloat( width_key, initial_norm_w );
            ImGui::GetStateStorage()->SetFloat( preset_w_key, preset_norm_w );
            ImGui::GetStateStorage()->SetInt( meta_hash_key, (int)current_meta_hash );
            ImGui::GetStateStorage()->SetBool( init_key, true );
        }

        int active_sort_col = ImGui::GetStateStorage()->GetInt( sort_col_key, -1 );
        int active_sort_dir = ImGui::GetStateStorage()->GetInt( sort_dir_key, (int)ImGuiSortDirection_None );
        float user_norm_w   = ImGui::GetStateStorage()->GetFloat( width_key, 0.0f );
        float preset_norm_w = ImGui::GetStateStorage()->GetFloat( preset_w_key, 0.0f );

        ImGuiTableFlags flags = mdConfig_.tableFlags;
        if( flags == 0 )
        {
            flags = ImGuiTableFlags_Borders | 
                    ImGuiTableFlags_RowBg | 
                    ImGuiTableFlags_SizingStretchProp | 
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Sortable |
                    ImGuiTableFlags_SortTristate |
                    ImGuiTableFlags_NoSavedSettings;
        }
        else if( flags & ImGuiTableFlags_Sortable )
        {
            flags |= ImGuiTableFlags_SortTristate;
        }

        float scale = ImGui::GetVisualScale();
        float user_table_w = ( user_norm_w > 0.0f ) ? ( user_norm_w * scale ) : 0.0f;

        // 2. Measure natural content width for each column to prevent 100% window width blowout
        float total_content_w = 0.0f;
        std::vector<float> col_widths( num_cols, 0.0f );
        for( size_t col = 0; col < num_cols; ++col )
        {
            TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
            float max_w = TableDetail::CalcCellVisualWidth( h_cell, true, mdConfig_ );
            if( flags & ImGuiTableFlags_Sortable )
            {
                max_w += 14.0f * scale; // Extra margin for header sort indicator arrow
            }
            for( size_t r = 0; r < rows.size(); ++r )
            {
                if( col < rows[r].size() )
                {
                    float cw = TableDetail::CalcCellVisualWidth( rows[r][col], false, mdConfig_ );
                    if( cw > max_w ) max_w = cw;
                }
            }
            // Add padding: left + right cell padding + extra breathing room (scaled by zoom/font scale)
            max_w += ( ImGui::GetStyle().CellPadding.x * 2.0f + 16.0f ) * scale;
            if( max_w < 30.0f * scale ) max_w = 30.0f * scale;
            col_widths[col] = max_w;
            total_content_w += max_w;
        }

        bool has_h_scroll = ( ImGui::GetCurrentWindow() && ( ImGui::GetCurrentWindow()->Flags & ImGuiWindowFlags_HorizontalScrollbar ) != 0 );
        float min_table_w = (float)num_cols * 28.0f * scale;
        float avail_w = ( mdConfig_.wrapWidth > 0.0f ) ? mdConfig_.wrapWidth : ImGui::GetContentRegionAvail().x;
        float table_w = total_content_w;
        if( avail_w > 0.0f && !has_h_scroll )
        {
            table_w = ImMin( total_content_w, avail_w );
            if( user_table_w > 0.0f )
            {
                table_w = ImClamp( user_table_w, ImMin( min_table_w, avail_w ), avail_w );
            }
        }
        else if( user_table_w > 0.0f )
        {
            table_w = ImMax( user_table_w, min_table_w );
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );

        ImGuiTable* table = nullptr;
        bool sort_changed = false;

        auto fire_state_callback = [&]( bool is_committed )
        {
            if( !mdConfig_.tableStateCallback ) return;
            MarkdownTableState st;
            st.tableIndex = table_idx;
            st.textStart = metadata.tableStart;
            st.textEnd = metadata.fullTableEnd;
            st.rowsEnd = metadata.rowsEnd;
            st.totalWidth = ImGui::GetStateStorage()->GetFloat( width_key, 0.0f );
            st.presetWidth = preset_norm_w;
            st.sortColumn = active_sort_col;
            st.sortDirection = active_sort_dir;
            if( active_sort_col >= 0 && (size_t)active_sort_col < headers.size() )
            {
                st.sortColumnName = TableDetail::GetCellPlainText( headers[active_sort_col] );
            }
            st.hasPreset = metadata.hasPreset;
            st.hasState = ( metadata.hasState || st.totalWidth > 0.0f || st.sortColumn >= 0 );
            st.isCommitted = is_committed;
            st.config = &mdConfig_;
            st.userData = mdConfig_.userData;

            if( table )
            {
                st.columnWidths.resize( num_cols );
                for( size_t c = 0; c < num_cols; ++c )
                {
                    if( c < (size_t)table->ColumnsCount )
                    {
                        float cw = table->Columns[c].WidthGiven;
                        st.columnWidths[c] = ( scale > 0.0f ) ? ( cw / scale ) : cw;
                    }
                }
            }
            mdConfig_.tableStateCallback( st, mdConfig_.userData );
        };

        if( ImGui::BeginTable( table_id, (int)num_cols, flags, ImVec2( table_w, 0.0f ) ) )
        {
            // 1. Setup column headers with proportional stretch weights based on content
            for( size_t col = 0; col < num_cols; ++col )
            {
                TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
                std::string col_name = TableDetail::GetCellPlainText( h_cell );
                if( col_name.empty() ) col_name = "##col";
                ImGui::TableSetupColumn( col_name.c_str(), ImGuiTableColumnFlags_WidthStretch, col_widths[col] );
            }

            // 2. Submit Header Row (with interactive sort click, hand cursor, and visual sort indicators)
            float header_h = ImMax( ImGui::GetFrameHeight(), ImGui::GetFontSize() + ImGui::GetStyle().CellPadding.y * 2.0f );
            ImGui::TableNextRow( ImGuiTableRowFlags_Headers, header_h );
            table = ImGui::GetCurrentTable();

            for( size_t col = 0; col < num_cols; ++col )
            {
                if( !ImGui::TableSetColumnIndex( (int)col ) ) continue;

                ImGuiTableColumn* column = ( table && (int)col < table->ColumnsCount ) ? &table->Columns[col] : nullptr;
                bool is_sortable = ( ( flags & ImGuiTableFlags_Sortable ) && column && !( column->Flags & ImGuiTableColumnFlags_NoSort ) );

                if( is_sortable && table )
                {
                    ImRect cell_r = ImGui::TableGetCellBgRect( table, (int)col );
                    if( cell_r.Max.y <= cell_r.Min.y )
                    {
                        cell_r.Max.y = cell_r.Min.y + header_h;
                    }

                    ImGuiID header_id = ImGui::GetID( (int)col );
                    ImGui::ItemAdd( cell_r, header_id, NULL, ImGuiItemFlags_NoNav );
                    bool hovered = false, held = false;
                    bool pressed = ImGui::ButtonBehavior( cell_r, header_id, &hovered, &held, ImGuiButtonFlags_AllowOverlap );

                    if( hovered )
                    {
                        ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    }

                    if( pressed && table->ReorderColumn != (int)col )
                    {
                        if( active_sort_col != (int)col )
                        {
                            active_sort_col = (int)col;
                            active_sort_dir = (int)ImGuiSortDirection_Ascending;
                        }
                        else
                        {
                            // Cycle: Ascending -> Descending -> None (Tristate)
                            if( active_sort_dir == (int)ImGuiSortDirection_Ascending )
                            {
                                active_sort_dir = (int)ImGuiSortDirection_Descending;
                            }
                            else if( active_sort_dir == (int)ImGuiSortDirection_Descending )
                            {
                                active_sort_col = -1;
                                active_sort_dir = (int)ImGuiSortDirection_None;
                            }
                            else
                            {
                                active_sort_dir = (int)ImGuiSortDirection_Ascending;
                            }
                        }
                        ImGui::GetStateStorage()->SetInt( sort_col_key, active_sort_col );
                        ImGui::GetStateStorage()->SetInt( sort_dir_key, active_sort_dir );
                        sort_changed = true;
                    }

                    if( held || hovered )
                    {
                        ImU32 col_bg = ImGui::GetColorU32( held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, col_bg, (int)col );
                    }

                    // Sync internal ImGui column sort state
                    column->SortOrder = ( (int)col == active_sort_col ) ? 0 : -1;
                    column->SortDirection = ( (int)col == active_sort_col ) ? (ImU8)active_sort_dir : (ImU8)ImGuiSortDirection_None;
                }

                TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
                RenderTableCell( h_cell, alignments[col], true, mdConfig_ );

                // Draw sort indicator arrow if this column is sorted
                if( is_sortable && active_sort_col == (int)col && active_sort_dir != (int)ImGuiSortDirection_None )
                {
                    ImRect cell_r = ImGui::TableGetCellBgRect( table, (int)col );
                    if( cell_r.Max.y <= cell_r.Min.y ) cell_r.Max.y = cell_r.Min.y + header_h;

                    float arrow_scale = 0.65f;
                    float w_arrow = ImTrunc( ImGui::GetFontSize() * arrow_scale + ImGui::GetStyle().FramePadding.x );
                    float x = ImMax( cell_r.Min.x, cell_r.Max.x - w_arrow );
                    float y = cell_r.Min.y + ImMax( 0.0f, ( ( cell_r.Max.y - cell_r.Min.y ) - ImGui::GetFontSize() ) * 0.5f );
                    ImGui::RenderArrow( ImGui::GetWindowDrawList(), ImVec2( x, y ), ImGui::GetColorU32( ImGuiCol_Text ),
                                        ( active_sort_dir == (int)ImGuiSortDirection_Ascending ) ? ImGuiDir_Up : ImGuiDir_Down,
                                        arrow_scale );
                }
            }

            if( sort_changed )
            {
                fire_state_callback( true );
            }

            // 3. Prepare Row Indices & Handle Sorting
            std::vector<size_t> row_indices( rows.size() );
            for( size_t i = 0; i < rows.size(); ++i ) row_indices[i] = i;

            if( ( flags & ImGuiTableFlags_Sortable ) && !rows.empty() && active_sort_col >= 0 && (size_t)active_sort_col < num_cols && active_sort_dir != (int)ImGuiSortDirection_None )
            {
                std::stable_sort( row_indices.begin(), row_indices.end(),
                    [&]( size_t a_idx, size_t b_idx ) -> bool {
                        const std::vector<TableCell>& rA = rows[a_idx];
                        const std::vector<TableCell>& rB = rows[b_idx];
                        TableCell cA = ( (size_t)active_sort_col < rA.size() ) ? rA[active_sort_col] : TableCell();
                        TableCell cB = ( (size_t)active_sort_col < rB.size() ) ? rB[active_sort_col] : TableCell();

                        int cmp = TableDetail::CompareCells( cA, cB );
                        if( active_sort_dir == (int)ImGuiSortDirection_Descending )
                            return cmp > 0;
                        else
                            return cmp < 0;
                    } );
            }

            // 4. Submit Data Rows in sorted order
            for( size_t r = 0; r < row_indices.size(); ++r )
            {
                ImGui::TableNextRow();
                const std::vector<TableCell>& row = rows[row_indices[r]];
                for( size_t col = 0; col < num_cols; ++col )
                {
                    if( !ImGui::TableSetColumnIndex( (int)col ) ) continue;
                    TableCell c_cell = ( col < row.size() ) ? row[col] : TableCell();
                    RenderTableCell( c_cell, alignments[col], false, mdConfig_ );
                }
            }

            ImGui::EndTable();

            // 5. Rightmost Border Resize Grip: allows expanding/contracting total table width
            if( table && ( flags & ImGuiTableFlags_Resizable ) && avail_w > 0.0f && table->OuterRect.Max.x > table->OuterRect.Min.x )
            {
                float right_x = table->OuterRect.Max.x;
                float top_y   = table->OuterRect.Min.y;
                float bot_y   = table->OuterRect.Max.y;
                const float hit_half_w = 4.0f * scale;

                ImRect grip_r( right_x - hit_half_w, top_y, right_x + hit_half_w, bot_y );
                ImGuiID grip_id = ImGui::GetID( "##md_table_right_resize_grip" );

                ImGui::ItemAdd( grip_r, grip_id, NULL, ImGuiItemFlags_NoNav );
                bool grip_hovered = false, grip_held = false;
                bool grip_pressed = ImGui::ButtonBehavior( grip_r, grip_id, &grip_hovered, &grip_held,
                    ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDoubleClick );

                bool is_resetting = ImGui::GetStateStorage()->GetBool( reset_key, false );
                if( !ImGui::IsMouseDown( 0 ) )
                {
                    is_resetting = false;
                    ImGui::GetStateStorage()->SetBool( reset_key, false );
                }

                float max_grip_w = has_h_scroll ? ImMax( avail_w * 5.0f, total_content_w * 3.0f ) : avail_w;

                if( ( grip_pressed || grip_hovered ) && ImGui::IsMouseDoubleClicked( 0 ) )
                {
                    // Double click: A/B toggle between baseline (preset_norm_w or 0.0f auto) and last manual width!
                    float cur_w = ImGui::GetStateStorage()->GetFloat( width_key, preset_norm_w );
                    float last_w = ImGui::GetStateStorage()->GetFloat( last_width_key, 0.0f );

                    if( std::fabs( cur_w - preset_norm_w ) > 0.1f )
                    {
                        // Currently custom width: remember it and reset to baseline
                        ImGui::GetStateStorage()->SetFloat( last_width_key, cur_w );
                        ImGui::GetStateStorage()->SetFloat( width_key, preset_norm_w );
                    }
                    else if( last_w > 0.0f && std::fabs( last_w - preset_norm_w ) > 0.1f )
                    {
                        // Currently baseline: restore prior custom width!
                        ImGui::GetStateStorage()->SetFloat( width_key, last_w );
                    }

                    ImGui::GetStateStorage()->SetBool( reset_key, true );
                    ImGui::ClearActiveID();
                    fire_state_callback( true );
                }
                else if( grip_held && !is_resetting && ImGui::IsMouseDragging( 0, 1.0f ) )
                {
                    float mouse_x = ImGui::GetIO().MousePos.x;
                    float new_w = mouse_x - table->OuterRect.Min.x;
                    float clamped_w = ImClamp( new_w, min_table_w, max_grip_w );
                    float norm_w = ( scale > 0.0f ) ? ( clamped_w / scale ) : clamped_w;
                    ImGui::GetStateStorage()->SetFloat( width_key, norm_w );
                    ImGui::GetStateStorage()->SetBool( dragging_key, true );
                }

                // Check if drag resize just completed on mouse release
                if( ImGui::GetStateStorage()->GetBool( dragging_key, false ) && !ImGui::IsMouseDown( 0 ) )
                {
                    ImGui::GetStateStorage()->SetBool( dragging_key, false );
                    fire_state_callback( true );
                }

                if( grip_hovered || ( grip_held && !is_resetting ) )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeEW );
                    ImU32 line_col = ImGui::GetColorU32( ( grip_held && !is_resetting ) ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered );
                    ImGui::GetWindowDrawList()->AddLine( ImVec2( right_x, top_y ), ImVec2( right_x, bot_y ), line_col, 2.0f * scale );
                }
            }
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );
    }

    inline void RenderTableBlock(
        const std::vector<TableCell>& headers,
        const std::vector<TableColumnAlign>& alignments,
        const std::vector<std::vector<TableCell>>& rows,
        const MarkdownConfig& mdConfig_,
        size_t table_idx )
    {
        TableDetail::MarkdownTableMetadata dummy_meta;
        RenderTableBlock( headers, alignments, rows, dummy_meta, mdConfig_, table_idx );
    }
    
    // render markdown (low-level rendering pipeline)
    namespace Markdown
    {
        inline void Render( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ )
    {
        static const char* linkHoverStart = NULL; // we need to preserve status of link hovering between frames
        Line        line;
        Link        link;
        Emphasis    em;
        TextRegion  textRegion;
        textRegion.wrapWidth = mdConfig_.wrapWidth;
        size_t      code_block_idx = 0;
        size_t      table_idx = 0;
        bool        lastWasSeparator = false;
        bool        lastWasBlankLine = false;

        char c = 0;
        for( int i=0; i < (int)markdownLength_; ++i )
        {
            c = markdown_[i];               // get the character at index
            if( c == 0 ) { break; }         // shouldn't happen but don't go beyond 0.

            // If we're at the beginning of the line, count any spaces
            if( line.isLeadingSpace )
            {
                if( c == ' ' )
                {
                    ++line.leadSpaceCount;
                    continue;
                }
                else if( c == '\t' )
                {
                    line.leadSpaceCount += ( mdConfig_.tabSize > 0 ? mdConfig_.tabSize : 4 );
                    continue;
                }
                else
                {
                    line.isLeadingSpace = false;
                    line.lastRenderPosition = i - 1;

                    // GitHub Flavored Markdown (GFM) Section 4.10: Tables
                    if( line.leadSpaceCount <= 3 )
                    {
                        std::vector<TableCell> headers;
                        std::vector<TableColumnAlign> alignments;
                        std::vector<std::vector<TableCell>> rows;
                        TableDetail::MarkdownTableMetadata metadata;
                        int table_end = TableDetail::TryParseTable(
                            markdown_, (int)markdownLength_, i,
                            headers, alignments, rows, metadata );
                        if( table_end > i )
                        {
                            RenderTableBlock( headers, alignments, rows, metadata, mdConfig_, table_idx++ );

                            textRegion.ResetIndent();
                            lastWasSeparator = false;
                            lastWasBlankLine = false;

                            int next_pos = TableDetail::SkipNewline( markdown_, table_end, (int)markdownLength_ );

                            line = Line();
                            line.isLeadingSpace = true;
                            line.lineStart = next_pos;
                            line.lastRenderPosition = next_pos - 1;
                            em = Emphasis();
                            link = Link();
                            i = next_pos - 1;
                            continue;
                        }
                    }

                    // CommonMark Section 4.1: Thematic breaks
                    // A line with 0-3 leading spaces, consisting solely of 3 or more matching '-', '*', or '_'
                    // characters (with optional spaces/tabs between them), followed only by whitespace until '\n' or EOF.
                    if( line.leadSpaceCount <= 3 && ( c == '-' || c == '*' || c == '_' ) )
                    {
                        char tb_char = c;
                        int tb_count = 0;
                        bool is_thematic_break = true;
                        int scan = i;
                        while( scan < (int)markdownLength_ && markdown_[scan] != '\n' && markdown_[scan] != '\r' )
                        {
                            char sc = markdown_[scan];
                            if( sc == tb_char )
                            {
                                ++tb_count;
                            }
                            else if( sc == ' ' || sc == '\t' )
                            {
                                // spaces and tabs are allowed between and after characters
                            }
                            else
                            {
                                is_thematic_break = false;
                                break;
                            }
                            ++scan;
                        }
                        if( is_thematic_break && tb_count >= 3 )
                        {
                            ImGui::Separator();
                            textRegion.ResetIndent();
                            lastWasSeparator = true;
                            lastWasBlankLine = false;

                            // Advance past newline (\r\n, \r, or \n)
                            if( scan < (int)markdownLength_ && markdown_[scan] == '\r' && scan + 1 < (int)markdownLength_ && markdown_[scan + 1] == '\n' )
                            {
                                scan += 2;
                            }
                            else if( scan < (int)markdownLength_ && ( markdown_[scan] == '\n' || markdown_[scan] == '\r' ) )
                            {
                                scan += 1;
                            }

                            line = Line();
                            line.isLeadingSpace = true;
                            line.lineStart = scan;
                            line.lastRenderPosition = scan - 1;
                            em = Emphasis();
                            link = Link();
                            i = scan - 1;
                            continue;
                        }
                    }

                    if(( c == '*' || c == '-' || c == '+' ) && 
                       ( (int)markdownLength_ > i + 1 ) && ( markdown_[ i + 1 ] == ' ' ))
                    {
                        line.isUnorderedListStart = true;
                        ++i;
                        ++line.lastRenderPosition;
                        line.lineStart = i + 1;
                        continue;
                    }
                    else if( c >= '0' && c <= '9' )
                    {
                        int d = i;
                        while( d < (int)markdownLength_ && markdown_[d] >= '0' && markdown_[d] <= '9' && (d - i) < 9 )
                        {
                            ++d;
                        }
                        if( d > i && d + 1 < (int)markdownLength_ && ( markdown_[d] == '.' || markdown_[d] == ')' ) && markdown_[d + 1] == ' ' )
                        {
                            line.isOrderedListStart = true;
                            line.orderedListNumStart = i;
                            line.orderedListNumEnd = d + 1; // includes '.' or ')'
                            i = d + 1; // advances past the delimiter
                            line.lastRenderPosition = i;
                            line.lineStart = i + 1;
                            continue;
                        }
                    }
                    else if( c == '#' )
                    {
                        line.headingCount++;
                        bool bContinueChecking = true;
                        int j = i;
                        while( ++j < (int)markdownLength_ && bContinueChecking )
                        {
                            c = markdown_[j];
                            switch( c )
                            {
                            case '#':
                                line.headingCount++;
                                break;
                            case ' ':
                                line.lastRenderPosition = j - 1;
                                i = j;
                                line.isHeading = true;
                                bContinueChecking = false;
                                break;
                            default:
                                line.isHeading = false;
                                bContinueChecking = false;
                                break;
                            }
                        }
                        if( line.isHeading )
                        {
                            em = Emphasis();
                            continue;
                        }
                    }
                    else if( ( c == '`' || c == '~' ) && i + 2 < (int)markdownLength_ &&
                             markdown_[ i + 1 ] == c && markdown_[ i + 2 ] == c )
                    {
                        char fence_char = c;
                        int fence_len = 0;
                        while( i + fence_len < (int)markdownLength_ && markdown_[ i + fence_len ] == fence_char )
                        {
                            ++fence_len;
                        }

                        // Info string (language identifier, e.g. "cpp", "lua")
                        int info_start = i + fence_len;
                        while( info_start < (int)markdownLength_ && ( markdown_[ info_start ] == ' ' || markdown_[ info_start ] == '\t' ) )
                        {
                            ++info_start;
                        }
                        int info_end = info_start;
                        while( info_end < (int)markdownLength_ && markdown_[ info_end ] != '\n' && markdown_[ info_end ] != '\r' )
                        {
                            ++info_end;
                        }
                        int trimmed_info_end = info_end;
                        while( trimmed_info_end > info_start && ( markdown_[ trimmed_info_end - 1 ] == ' ' || markdown_[ trimmed_info_end - 1 ] == '\t' ) )
                        {
                            --trimmed_info_end;
                        }

                        // Start of code block content
                        int code_start = info_end;
                        if( code_start < (int)markdownLength_ && markdown_[ code_start ] == '\r' ) ++code_start;
                        if( code_start < (int)markdownLength_ && markdown_[ code_start ] == '\n' ) ++code_start;

                        int code_end = (int)markdownLength_;
                        int scan = code_start;
                        while( scan < (int)markdownLength_ )
                        {
                            int line_start = scan;
                            while( line_start < (int)markdownLength_ && ( markdown_[ line_start ] == ' ' || markdown_[ line_start ] == '\t' ) )
                            {
                                ++line_start;
                            }
                            if( line_start + fence_len <= (int)markdownLength_ )
                            {
                                bool is_match = true;
                                for( int f = 0; f < fence_len; ++f )
                                {
                                    if( markdown_[ line_start + f ] != fence_char ) { is_match = false; break; }
                                }
                                if( is_match )
                                {
                                    int f_scan = line_start + fence_len;
                                    while( f_scan < (int)markdownLength_ && markdown_[ f_scan ] == fence_char ) ++f_scan;
                                    while( f_scan < (int)markdownLength_ && ( markdown_[ f_scan ] == ' ' || markdown_[ f_scan ] == '\t' ) ) ++f_scan;
                                    if( f_scan >= (int)markdownLength_ || markdown_[ f_scan ] == '\n' || markdown_[ f_scan ] == '\r' )
                                    {
                                        code_end = scan;
                                        if( code_end > code_start && markdown_[ code_end - 1 ] == '\n' ) --code_end;
                                        if( code_end > code_start && markdown_[ code_end - 1 ] == '\r' ) --code_end;

                                        while( f_scan < (int)markdownLength_ && ( markdown_[ f_scan ] == '\r' || markdown_[ f_scan ] == '\n' ) )
                                        {
                                            ++f_scan;
                                        }
                                        i = f_scan - 1;
                                        break;
                                    }
                                }
                            }
                            while( scan < (int)markdownLength_ && markdown_[ scan ] != '\n' ) ++scan;
                            if( scan < (int)markdownLength_ && markdown_[ scan ] == '\n' ) ++scan;
                        }
                        if( scan >= (int)markdownLength_ )
                        {
                            code_end = (int)markdownLength_;
                            i = (int)markdownLength_ - 1;
                        }

                        RenderCodeBlock( markdown_ + code_start, (size_t)(code_end - code_start),
                                         markdown_ + info_start, (size_t)(trimmed_info_end - info_start),
                                         mdConfig_, code_block_idx++, line.leadSpaceCount );

                        textRegion.ResetIndent();
                        lastWasSeparator = false;
                        lastWasBlankLine = false;

                        line = Line();
                        line.isLeadingSpace = true;
                        line.lineStart = i + 1;
                        line.lastRenderPosition = i;
                        em = Emphasis();
                        continue;
                    }
                }
            }

            // Test to see if we have an escaped character via backslash (\* or \# or \<)
            // CommonMark Section 6.1: Backslash escapes do not apply inside inline code spans, links, or emphasis
            if( c == '\\' && em.state == Emphasis::NONE && link.state == Link::NO_LINK &&
                i + 1 < (int)markdownLength_ && IsMarkdownEscapable( markdown_[ i + 1 ] ) )
            {
                // Render any preceding plain text before the backslash
                line.lineEnd = i;
                if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                {
                    RenderLine( markdown_, line, textRegion, mdConfig_ );
                    ImGui::SameLine( 0.0f, 0.0f );
                }
                // Skip the backslash character so it is never displayed
                ++i;
                line.lineStart = i;
                line.lastRenderPosition = i - 1;
                // markdown_[i] is now an escaped literal character; skip normal markup checks for this character
                continue;
            }

            // Test to see if we have a custom styling tag <tag=param>...</tag>
            if( c == '<' && mdConfig_.tagCallback != NULL && link.state == Link::NO_LINK && em.state == Emphasis::NONE )
            {
                int openClose = -1;
                for( int k = i + 1; k < (int)markdownLength_; ++k )
                {
                    if( markdown_[k] == '>' ) { openClose = k; break; }
                    if( markdown_[k] == '\n' || markdown_[k] == '<' ) break;
                }
                if( openClose > i + 1 )
                {
                    int tagHeaderStart = i + 1;
                    int tagHeaderEnd = openClose;
                    int sep = -1;
                    for( int k = tagHeaderStart; k < tagHeaderEnd; ++k )
                    {
                        if( markdown_[k] == '=' || markdown_[k] == ':' || markdown_[k] == ' ' || markdown_[k] == '\t' )
                        {
                            sep = k;
                            break;
                        }
                    }
                    int nameEnd = ( sep != -1 ) ? sep : tagHeaderEnd;
                    int nameLen = nameEnd - tagHeaderStart;

                    char closeTag[64];
                    int closeTagLen = snprintf( closeTag, sizeof(closeTag), "</%.*s>", nameLen, markdown_ + tagHeaderStart );
                    if( closeTagLen > 0 && closeTagLen < (int)sizeof(closeTag) )
                    {
                        const char* closeFound = nullptr;
                        int searchStart = openClose + 1;
                        int maxSearch = (int)markdownLength_ - closeTagLen;
                        for( int k = searchStart; k <= maxSearch; ++k )
                        {
                            if( markdown_[k] == '<' && strncmp( markdown_ + k, closeTag, closeTagLen ) == 0 )
                            {
                                closeFound = markdown_ + k;
                                break;
                            }
                        }
                        if( closeFound != nullptr )
                        {
                            int closeStart = (int)(closeFound - markdown_);
                            int closeEnd = closeStart + closeTagLen;

                            line.lineEnd = i;
                            if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                            {
                                RenderLine( markdown_, line, textRegion, mdConfig_ );
                                line.isUnorderedListStart = false;
                                line.isOrderedListStart = false;
                                ImGui::SameLine( 0.0f, 0.0f );
                            }
                            else if( line.isUnorderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isUnorderedListStart = false;
                            }
                            else if( line.isOrderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isOrderedListStart = false;
                            }

                            MarkdownTagData tagData;
                            tagData.tagName = markdown_ + tagHeaderStart;
                            tagData.tagNameLength = nameLen;
                            tagData.tagParam = ( sep != -1 ) ? ( markdown_ + sep + 1 ) : nullptr;
                            tagData.tagParamLength = ( sep != -1 ) ? ( tagHeaderEnd - sep - 1 ) : 0;
                            while( tagData.tagParamLength > 0 && ( *tagData.tagParam == ' ' || *tagData.tagParam == '\"' || *tagData.tagParam == '\'' ) )
                            {
                                ++tagData.tagParam;
                                --tagData.tagParamLength;
                            }
                            while( tagData.tagParamLength > 0 && ( tagData.tagParam[tagData.tagParamLength - 1] == ' ' || tagData.tagParam[tagData.tagParamLength - 1] == '\"' || tagData.tagParam[tagData.tagParamLength - 1] == '\'' ) )
                            {
                                --tagData.tagParamLength;
                            }
                            tagData.innerText = markdown_ + openClose + 1;
                            tagData.innerTextLength = closeStart - ( openClose + 1 );
                            tagData.config = &mdConfig_;
                            tagData.userData = mdConfig_.userData;

                            mdConfig_.tagCallback( tagData );

                            int peek = closeEnd;
                            while( peek < (int)markdownLength_ && ( markdown_[peek] == ' ' || markdown_[peek] == '\t' ) )
                            {
                                ++peek;
                            }

                            if( peek >= (int)markdownLength_ || markdown_[peek] == '\n' || markdown_[peek] == '\r' )
                            {
                                // Tag was at end of line. The tag widget already advanced ImGui to the next line.
                                // Consume optional \r and single trailing \n so it doesn't double-advance!
                                if( peek < (int)markdownLength_ && markdown_[peek] == '\r' )
                                {
                                    ++peek;
                                }
                                if( peek < (int)markdownLength_ && markdown_[peek] == '\n' )
                                {
                                    ++peek;
                                }
                                textRegion.ResetIndent();
                                line = Line();
                                em = Emphasis();
                                link = Link();
                                line.lineStart = peek;
                                line.lastRenderPosition = peek - 1;
                                i = peek - 1;
                                continue;
                            }
                            else
                            {
                                // Tag was mid-line; flow inline with following text without eating spaces!
                                i = closeEnd - 1;
                                line.lastRenderPosition = closeEnd - 1;
                                line.lineStart = closeEnd;
                                ImGui::SameLine( 0.0f, 0.0f );
                                continue;
                            }
                        }
                    }
                }
            }

            // Test to see if we have a link
            switch( link.state )
            {
            case Link::NO_LINK:
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                if( c == '{' && !line.isHeading )
#else
                if( ( c == '[' || c == '{' ) && !line.isHeading )
#endif
                {
                    link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                    link.openBracketChar = c;
                    link.text.start = i + 1;
                    if( i > 0 && markdown_[i - 1] == '!' )
                    {
                        link.isImage = true;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKET_OPEN:
                {
                    char expectedClose = ( link.openBracketChar == '{' ) ? '}' : ']';
                    if( c == expectedClose )
                    {
                        link.state = Link::HAS_SQUARE_BRACKETS;
                        link.text.stop = i;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKETS:
                if( c == '(' )
                {
                    link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                    link.url.start = i + 1;
                    link.num_brackets_open = 1;
                }
                else
                {
                    // If not immediately followed by '(', reset link state:
                    link = Link();
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                    if( c == '{' && !line.isHeading )
#else
                    if( ( c == '[' || c == '{' ) && !line.isHeading )
#endif
                    {
                        link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                        link.openBracketChar = c;
                        link.text.start = i + 1;
                        if( i > 0 && markdown_[i - 1] == '!' )
                        {
                            link.isImage = true;
                        }
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
                if( c == '(' )
                {
                    ++link.num_brackets_open;
                }
                else if( c == ')' )
                {
                    --link.num_brackets_open;
                }
                if( link.num_brackets_open == 0 )
                {
                    em = Emphasis();
                    line.lineEnd = link.text.start - ( link.isImage ? 2 : 1 );
                    if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                    {
                        RenderLine( markdown_, line, textRegion, mdConfig_ );
                        line.isUnorderedListStart = false;
                        line.isOrderedListStart = false;
                        ImGui::SameLine( 0.0f, 0.0f );
                    }
                    else if( line.isUnorderedListStart )
                    {
                        int numLeadIndents = line.leadSpaceCount / 2;
                        textRegion.ApplyLeadIndent( numLeadIndents );
                        textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                        line.isUnorderedListStart = false;
                    }
                    else if( line.isOrderedListStart )
                    {
                        int numLeadIndents = line.leadSpaceCount / 2;
                        textRegion.ApplyLeadIndent( numLeadIndents );
                        textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                        line.isOrderedListStart = false;
                    }
                    link.url.stop = i;
                    if( link.isImage )
                    {
                        bool drawnImage = false;
                        bool useLinkCallback = false;
                        MarkdownImageCallback* imageCb = mdConfig_.imageCallback ? mdConfig_.imageCallback : GetDefaultMarkdownImageCallback();
                        if( imageCb )
                        {
                            MarkdownImageData imageData = imageCb( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );
                            useLinkCallback = imageData.useLinkCallback;
                            if( imageData.isValid )
                            {
                                ImGui::Image( imageData.user_texture_id, imageData.size, imageData.uv0, imageData.uv1, imageData.tint_col, imageData.border_col );
                                drawnImage = true;
                            }
                        }
                        if( !drawnImage )
                        {
                            ImGui::Text( "( Image %.*s not loaded )", link.url.size(), markdown_ + link.url.start );
                        }
                        if( ImGui::IsItemHovered() )
                        {
                            if( ImGui::IsMouseReleased( 0 ) && mdConfig_.linkCallback && useLinkCallback )
                            {
                                mdConfig_.linkCallback( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );
                            }
                            if( link.text.size() > 0 && mdConfig_.tooltipCallback )
                            {
                                mdConfig_.tooltipCallback( { { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true }, mdConfig_.linkIcon } );
                            }
                        }
                    }
                    else
                    {
                        textRegion.RenderLinkTextWrapped( markdown_ + link.text.start, markdown_ + link.text.start + link.text.size(), link, markdown_, mdConfig_, &linkHoverStart, false );
                    }
                    ImGui::SameLine( 0.0f, 0.0f );
                    link = Link();
                    line.lastRenderPosition = i;
                    break;
                }
            }

            // Test to see if we have emphasis styling
			switch( em.state )
			{
			case Emphasis::NONE:
				if( link.state == Link::NO_LINK && !line.isHeading )
                {
                    int next = i + 1;
                    int prev = i - 1;
					// Check if the character is in the user's registered emphasisChars string
					if( mdConfig_.emphasisChars && strchr(mdConfig_.emphasisChars, c) != NULL
                        // For markers that strictly require at least 2 characters (CriticMarkup '++' underline,
                        // GFM '~~' strikethrough, and ABC legacy '$$' strikethrough), do not trigger on single chars:
                        && ( ( c != '+' && c != '~' && c != '$' ) || ( (int)markdownLength_ > next && markdown_[ next ] == c ) )
                        && ( i == line.lineStart
                            || isspace((unsigned char)markdown_[ prev ])
                            || ( ispunct((unsigned char)markdown_[ prev ]) && markdown_[ prev ] != c ) )
                        && (int)markdownLength_ > next 
                        && markdown_[ next ] != ' '
                        && markdown_[ next ] != '\n'
					    && markdown_[ next ] != '"'
					    && markdown_[ next ] != ')'
					    && markdown_[ next ] != '\''
                        && markdown_[ next ] != '\t' )
                    {
						em.state = Emphasis::LEFT;
						em.sym = c;
                        em.text.start = i;
						line.emphasisCount = 1;
						line.emphasisChar = c;
						continue;
					}
				}
				break;
			case Emphasis::LEFT:
				if( em.sym == c )
                {
					++line.emphasisCount;
					continue;
				}
                else
                {
					em.text.start = i;
					em.state = Emphasis::MIDDLE;
				}
				break;
			case Emphasis::MIDDLE:
				if( em.sym == c )
                {
					em.state = Emphasis::RIGHT;
					em.text.stop = i;
				}
                else
                {
                    break;
                }
			case Emphasis::RIGHT:
				if( em.sym == c )
                {
					if( line.emphasisCount < 3 && ( i - em.text.stop + 1 == line.emphasisCount ) )
                    {
                        int lineEnd = em.text.start - line.emphasisCount;
                        if( lineEnd > line.lineStart && lineEnd > line.lastRenderPosition + 1 )
                        {
                            line.lineEnd = lineEnd;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
						    ImGui::SameLine( 0.0f, 0.0f );
                            line.isUnorderedListStart = false;
                            line.isOrderedListStart = false;
                        }
                        else if( line.isUnorderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isUnorderedListStart = false;
                        }
                        else if( line.isOrderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isOrderedListStart = false;
                        }
						line.isEmphasis = true;
						line.lastRenderPosition = em.text.start - 1;
                        line.lineStart = em.text.start;
					    line.lineEnd = em.text.stop;
					    RenderLine( markdown_, line, textRegion, mdConfig_ );
					    ImGui::SameLine( 0.0f, 0.0f );
					    line.isEmphasis = false;
					    line.lastRenderPosition = i;
					    em = Emphasis();
                    }
                    continue;
				} 
                else
                {
                    em.state = Emphasis::NONE;
                    int start = em.text.start - line.emphasisCount;
                    if( start < line.lineStart )
                    {
                        line.lineEnd = line.lineStart;
                        line.lineStart = start;
                        line.lastRenderPosition = start - 1;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        line.lineStart          = line.lineEnd;
                        line.lastRenderPosition = line.lineStart - 1;
                    }
                }
				break;
			}

            if( c == '\n' )
            {
                line.lineEnd = i;
                line.isPrecededBySeparator = lastWasSeparator;
                line.isPrecededByBlankLine = lastWasBlankLine;
                RenderLine( markdown_, line, textRegion, mdConfig_ );

                lastWasSeparator = line.hadSeparator;
                lastWasBlankLine = line.wasBlank;

				line = Line();
                em = Emphasis();

                line.lineStart = i + 1;
                line.lastRenderPosition = i;
                // Check whether the next line is a continuation line of the current list item
                bool isNextContinuation = false;
                if( ( textRegion.GetIndent() > 0.0f || textRegion.GetLeadIndents() > 0 ) && i + 1 < (int)markdownLength_ )
                {
                    int next = i + 1;
                    int spaceCount = 0;
                    while( next < (int)markdownLength_ && ( markdown_[next] == ' ' || markdown_[next] == '\t' ) )
                    {
                        spaceCount += ( markdown_[next] == '\t' ) ? 4 : 1;
                        ++next;
                    }
                    if( spaceCount >= 2 && next < (int)markdownLength_ )
                    {
                        char nextC = markdown_[next];
                        if( nextC != '*' && nextC != '-' && nextC != '+' && nextC != '#' && !( nextC >= '0' && nextC <= '9' ) && nextC != '\n' && nextC != '\r' )
                        {
                            isNextContinuation = true;
                        }
                    }
                }

                if( !isNextContinuation )
                {
                    textRegion.ResetIndent();
                }
                link = Link();
            }
        }

        if( markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[ line.lineStart ] != 0 )
        {
            line.lineEnd = (int)markdownLength_;
            if( 0 == markdown_[ line.lineEnd - 1 ] )
            {
                --line.lineEnd;
            }
            line.isPrecededBySeparator = lastWasSeparator;
            line.isPrecededByBlankLine = lastWasBlankLine;
            RenderLine( markdown_, line, textRegion, mdConfig_ );
        }
    }
    } // namespace Markdown

    inline bool TextRegion::RenderLinkText( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ )
    {
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.type = MarkdownFormatType::LINK;
        mdConfig_.formatCallback( formatInfo, true );
        ImGui::PushTextWrapPos( -1.0f );
        ImGui::TextUnformatted( text_, text_end_ );
        ImGui::PopTextWrapPos();

        bool bThisItemHovered = ImGui::IsItemHovered();
        if(bThisItemHovered)
        {
            *linkHoverStart_ = markdown_ + link_.text.start;
        }
        bool bHovered = bThisItemHovered || ( *linkHoverStart_ == ( markdown_ + link_.text.start ) );

        formatInfo.itemHovered = bHovered;
        mdConfig_.formatCallback( formatInfo, false );

        if(bHovered)
        {
            if( ImGui::IsMouseReleased( 0 ) && mdConfig_.linkCallback )
            {
                mdConfig_.linkCallback( { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false } );
            }
            if( mdConfig_.tooltipCallback )
            {
                mdConfig_.tooltipCallback( { { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false }, mdConfig_.linkIcon } );
            }
        }
        return bThisItemHovered;
    }

    inline void TextRegion::RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ )
        {
            if( wrapWidth < 0.0f )
            {
                bool bHovered = RenderLinkText( text_, text_end_, link_, markdown_, mdConfig_, linkHoverStart_ );
                if( !bHovered && *linkHoverStart_ == markdown_ + link_.text.start )
                {
                    *linkHoverStart_ = NULL;
                }
                return;
            }
#ifdef ORIGINAL
		float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
		float		scale = ImGui::GetFontScale(false);	  // use parent's scale
#endif
            float       lineStartX = ImGui::GetCurrentWindow()->WorkRect.Min.x;
            if( ImGuiTable* cur_table = ImGui::GetCurrentTable() )
            {
                if( cur_table->CurrentColumn != -1 && cur_table->CurrentColumn < cur_table->ColumnsCount )
                {
                    lineStartX = cur_table->Columns[cur_table->CurrentColumn].WorkMinX;
                }
            }
            float       usedOnLine = ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
            float       widthLeft = ( wrapWidth > 0.0f ) ? ImMax( 10.0f, wrapWidth - usedOnLine ) : GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // If we are continuing on an existing line with prior content (usedOnLine > 0),
            // and the first word cannot cleanly fit in widthLeft, push the word to the next line.
            // When wrapping to a new line, strip any leading spaces that served only as inter-item separation on the previous line.
            if( usedOnLine > 0.0f )
            {
                const char* first_non_space = text_;
                while( first_non_space < text_end_ && *first_non_space == ' ' ) ++first_non_space;

                bool breaks_first_word = false;
                if( endLine <= first_non_space && first_non_space < text_end_ )
                {
                    breaks_first_word = true;
                }
                else if( endLine > first_non_space && endLine < text_end_ )
                {
                    const char* word_end = first_non_space;
                    while( word_end < text_end_ && IsCharInsideWord( *word_end ) ) ++word_end;
                    if( endLine < word_end )
                    {
                        breaks_first_word = true;
                    }
                }

                if( breaks_first_word )
                {
                    // Push to next line and suppress inter-item leading space!
                    ImGui::NewLine();
                    text_ = first_non_space;
                    widthLeft = ( wrapWidth > 0.0f ) ? wrapWidth : ImGui::GetContentRegionAvail().x;
                    endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                    endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                }
            }
            
            bool bHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
            if( bIndentToHere_ )
            {
                float indentNeeded = GetContentRegionAvail().x - widthLeft;
                if( indentNeeded )
                {
                    ImGui::Indent( indentNeeded );
                    indentX += indentNeeded;
                }
            }
            widthLeft = ( wrapWidth > 0.0f ) ? wrapWidth : GetContentRegionAvail().x;
            while( endLine < text_end_ )
            {
                text_ = endLine;
                while( text_ < text_end_ && *text_ == ' ' ) { ++text_; }    // skip spaces at start of line
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine && text_ < text_end_ ) 
                {
                    endLine++;
                }
                bool bThisLineHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
                bHovered = bHovered || bThisLineHovered;
            }
            if( !bHovered && *linkHoverStart_ == markdown_ + link_.text.start )
            {
                *linkHoverStart_ = NULL;
            }
        }

    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ )
    {
        switch( markdownFormatInfo_.type )
        {
        case MarkdownFormatType::NORMAL_TEXT:
        case MarkdownFormatType::UNORDERED_LIST:
        case MarkdownFormatType::ORDERED_LIST:
            if( markdownFormatInfo_.config && markdownFormatInfo_.config->regularFont )
            {
                if( start_ ) ImGui::PushFont( markdownFormatInfo_.config->regularFont );
                else         ImGui::PopFont();
            }
            break;
		case MarkdownFormatType::EMPHASIS:
        {
            if( markdownFormatInfo_.emphasisChar == '`' )
            {
                // Monospace code span (`code`):
                ImFont* mono_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->monoFont : nullptr;
                if( mono_font )
                {
                    if( start_ ) ImGui::PushFont( mono_font );
                    else         ImGui::PopFont();
                }
                break;
            }

            if( ( markdownFormatInfo_.emphasisChar == '~' || markdownFormatInfo_.emphasisChar == '$' ) && markdownFormatInfo_.level >= 2 )
            {
                // GFM Strikethrough (~~text~~) or ABC legacy strikethrough ($$text$$):
                if( !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), true );
                }
                break;
            }

            if( markdownFormatInfo_.emphasisChar == '_' && markdownFormatInfo_.config && markdownFormatInfo_.config->underlineWithUnderscores && markdownFormatInfo_.level == 2 )
            {
                // Underline via __text__ when underlineWithUnderscores is enabled:
                if( !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), false );
                }
                break;
            }

            if( markdownFormatInfo_.emphasisChar == '+' )
            {
                // Underline via ++text++ (CriticMarkup / Markdown Extended):
                if( markdownFormatInfo_.level >= 2 && !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), false );
                }
                break;
            }

            if( markdownFormatInfo_.level == 1 )
            {
                // Level 1: Italic (*italic* or _italic_)
                ImFont* italic_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->italicFont : nullptr;
                if( italic_font )
                {
                    if( start_ ) ImGui::PushFont( italic_font );
                    else         ImGui::PopFont();
                }
                else
                {
                    if( start_ ) ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ ImGuiCol_TextDisabled ] );
                    else         ImGui::PopStyleColor();
                }
            }
            else
            {
                // Level 2: Bold (**bold** or __bold__)
                ImFont* bold_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->boldFont : nullptr;
                if( !bold_font && markdownFormatInfo_.config )
                {
                    bold_font = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
                }
			    if( start_ )
			    {
				    if( bold_font )
				    {
					    ImGui::PushFont( bold_font );
				    }
			    }
                else
			    {
				    if( bold_font )
				    {
					    ImGui::PopFont();
				    }
			    }
            }
            break;
        }
        case MarkdownFormatType::HEADING:
        {
            MarkdownHeadingFormat fmt;
            if( markdownFormatInfo_.level > MarkdownConfig::NUMHEADINGS )
            {
                fmt = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
            }
            else
            {
                fmt = markdownFormatInfo_.config->headingFormats[ markdownFormatInfo_.level - 1 ];
            }

            // Semantic font overrides:
            if( markdownFormatInfo_.config )
            {
                if( markdownFormatInfo_.level == 1 && markdownFormatInfo_.config->h1Font )
                    fmt.font = markdownFormatInfo_.config->h1Font;
                else if( markdownFormatInfo_.level == 2 && markdownFormatInfo_.config->h2Font )
                    fmt.font = markdownFormatInfo_.config->h2Font;
                else if( markdownFormatInfo_.level == 3 && markdownFormatInfo_.config->h3Font )
                    fmt.font = markdownFormatInfo_.config->h3Font;
            }

            if( start_ )
            {
                if( fmt.font  )
                {
                    ImGui::PushFont( fmt.font );
                }
                if( !markdownFormatInfo_.isStartOfDoc &&
                    !markdownFormatInfo_.isPrecededBySeparator &&
                    !markdownFormatInfo_.isPrecededByBlankLine )
                {
                    float font_sz = ImGui::GetFontSize();
                    float top_margin = ( markdownFormatInfo_.level <= 2 ) ? ( font_sz * 0.45f ) : ( font_sz * 0.30f );
                    ImGui::Dummy( ImVec2( 0.0f, top_margin ) );
                }
            }
            else
            {
                if( fmt.separator )
                {
                    ImGui::Separator();
                }
                else
                {
                    float font_sz = ImGui::GetFontSize();
                    float bottom_margin = font_sz * 0.20f;
                    ImGui::Dummy( ImVec2( 0.0f, bottom_margin ) );
                }
                if( fmt.font )
                {
                    ImGui::PopFont();
                }
            }
            break;
        }
        case MarkdownFormatType::LINK:
            if( start_ )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ ImGuiCol_ButtonHovered ] );
            }
            else
            {
                ImGui::PopStyleColor();
                if( markdownFormatInfo_.itemHovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_ButtonHovered ] );
                }
                else
                {
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_Button ] );
                }
            }
            break;
        case MarkdownFormatType::CODE_BLOCK:
            break;
        }
    }

    // -------------------------------------------------------------------------
    // Markdown Table Layout & Sort State Helper
    // -------------------------------------------------------------------------
    // Updates or inserts an HTML comment directive (<!-- table-state: ... -->)
    // immediately following the table rows in the provided markdown text.
    // If the table is reset to its baseline/default state, any existing
    // <!-- table-state: ... --> comment is cleanly removed.
    // Author baseline presets (<!-- table-preset: ... -->) are always preserved intact.
    inline std::string ApplyTableStateToMarkdown( const std::string& markdownText, const MarkdownTableState& state )
    {
        // 1. Determine target directive contents
        bool is_default_width = ( state.totalWidth <= 0.0f ) ||
                                ( state.hasPreset && std::fabs( state.totalWidth - state.presetWidth ) < 0.5f );
        bool is_default_sort  = ( state.sortColumn < 0 ) || ( state.sortDirection == 0 );

        std::string new_directive;
        if( !is_default_width || !is_default_sort )
        {
            new_directive = "<!-- table-state:";
            if( !is_default_width )
            {
                char buf[32];
                if( std::fabs( state.totalWidth - std::round( state.totalWidth ) ) < 0.05f )
                    snprintf( buf, sizeof(buf), " width=%.0f", state.totalWidth );
                else
                    snprintf( buf, sizeof(buf), " width=%.1f", state.totalWidth );
                new_directive += buf;
            }
            if( !is_default_sort )
            {
                new_directive += " sort=";
                if( !state.sortColumnName.empty() )
                {
                    new_directive += "\"";
                    new_directive += state.sortColumnName;
                    new_directive += "\"";
                }
                else
                {
                    new_directive += std::to_string( state.sortColumn );
                }
                new_directive += ( state.sortDirection == 2 ) ? " desc" : " asc";
            }
            new_directive += " -->";
        }

        int text_len = (int)markdownText.size();
        int t_start = state.textStart;
        int t_end = state.textEnd;
        int r_end = state.rowsEnd;

        // Locate existing table-state directive attached to this table
        int search_start = ( r_end > 0 && r_end <= text_len ) ? r_end : t_start;
        int search_end = ImMin( t_end + 128, text_len );

        int existing_comment_start = -1;
        int existing_comment_end = -1;

        if( search_end > search_start )
        {
            std::string search_slice = markdownText.substr( search_start, search_end - search_start );
            size_t found_pos = search_slice.find( "<!-- table-state:" );
            if( found_pos == std::string::npos )
            {
                found_pos = search_slice.find( "<!-- table: user" );
            }
            if( found_pos == std::string::npos )
            {
                found_pos = search_slice.find( "<!-- table: state" );
            }
            if( found_pos == std::string::npos )
            {
                size_t p = search_slice.find( "<!-- table:" );
                if( p != std::string::npos )
                {
                    std::string sub = search_slice.substr( p );
                    if( sub.find( "default" ) == std::string::npos && sub.find( "preset" ) == std::string::npos )
                    {
                        found_pos = p;
                    }
                }
            }

            if( found_pos != std::string::npos )
            {
                size_t close_pos = search_slice.find( "-->", found_pos );
                if( close_pos != std::string::npos )
                {
                    existing_comment_start = (int)( search_start + found_pos );
                    existing_comment_end = (int)( search_start + close_pos + 3 );

                    // Include surrounding line delimiters
                    int line_start = existing_comment_start;
                    while( line_start > 0 && markdownText[line_start - 1] != '\n' && markdownText[line_start - 1] != '\r' )
                    {
                        if( markdownText[line_start - 1] == ' ' || markdownText[line_start - 1] == '\t' )
                            --line_start;
                        else
                            break;
                    }
                    int line_end = existing_comment_end;
                    if( line_end < text_len && markdownText[line_end] == '\r' ) ++line_end;
                    if( line_end < text_len && markdownText[line_end] == '\n' ) ++line_end;

                    existing_comment_start = line_start;
                    existing_comment_end = line_end;
                }
            }
        }

        std::string result = markdownText;

        if( existing_comment_start >= 0 )
        {
            if( new_directive.empty() )
            {
                result.erase( existing_comment_start, existing_comment_end - existing_comment_start );
            }
            else
            {
                std::string rep = new_directive + "\n";
                result.replace( existing_comment_start, existing_comment_end - existing_comment_start, rep );
            }
        }
        else if( !new_directive.empty() )
        {
            int ins_pos = ( t_end > 0 && t_end <= text_len ) ? t_end : r_end;
            while( ins_pos < text_len && markdownText[ins_pos] != '\n' && markdownText[ins_pos] != '\r' )
                ++ins_pos;
            if( ins_pos < text_len && markdownText[ins_pos] == '\r' ) ++ins_pos;
            if( ins_pos < text_len && markdownText[ins_pos] == '\n' ) ++ins_pos;

            std::string ins = new_directive + "\n";
            result.insert( ins_pos, ins );
        }

        return result;
    }

}
