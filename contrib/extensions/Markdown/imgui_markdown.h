#pragma once

// License: zlib
// Copyright (c) 2019 Juliette Foucaut & Doug Binks

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "imgui_internal.h"

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
}
#endif

// Configure whether or not to use "[]" for links, or the alternative "{}":
#define USE_CURLY_BRACKETS	 /* comment out to use standard [] for links */
#ifdef USE_CURLY_BRACKETS
#define MD_SQUARE_BRACKET_OPEN '{'
#define MD_SQUARE_BRACKET_CLOSE '}'
#else /* USE SQUARE BRACKETS: */
#define MD_SQUARE_BRACKET_OPEN '['
#define MD_SQUARE_BRACKET_CLOSE ']'
#endif

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
         LINK,
         EMPHASIS,
    };

    struct MarkdownFormatInfo
    {
        MarkdownFormatType      type    = MarkdownFormatType::NORMAL_TEXT;
        int32_t                 level   = 0;                               // Set for headings: 1 for H1, 2 for H2 etc.
		char                    emphasisChar  = '\0';                      // The character used for emphasis (e.g. *, _, ^, >)
        bool                    itemHovered = false;                       // Currently only set for links when mouse hovered, only valid when start_ == false
        const MarkdownConfig*   config  = NULL;
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

    struct MarkdownConfig
    {
        static const int        NUMHEADINGS = 4;

        MarkdownLinkCallback*   linkCallback = NULL;
        MarkdownTooltipCallback* tooltipCallback = NULL;
        MarkdownImageCallback*  imageCallback = NULL;
        MarkdownTagCallback*    tagCallback = NULL;
        const char*             linkIcon = "";                      // icon displayd in link tooltip
        const char*             emphasisChars = "*_`";              // string of characters recognized as emphasis markers
        MarkdownHeadingFormat   headingFormats[ NUMHEADINGS ] = { { NULL, true }, { NULL, true }, { NULL, true }, { NULL, true } };
        void*                   userData = NULL;
        MarkdownFormalCallback* formatCallback = defaultMarkdownFormatCallback;
        float                   bulletSpacing = 0.0f;               // 0.0f = default (4.0f); > 0.0f = custom gap between bullet and text

        // Semantic font aliases (if set, takes priority over headingFormats):
        ImFont*                 h1Font = nullptr;                   // If nullptr, falls back to headingFormats[0].font
        ImFont*                 h2Font = nullptr;                   // If nullptr, falls back to headingFormats[1].font
        ImFont*                 h3Font = nullptr;                   // If nullptr, falls back to headingFormats[2].font
        ImFont*                 boldFont = nullptr;                 // If nullptr, falls back to headingFormats[3].font
        ImFont*                 italicFont = nullptr;               // If nullptr, falls back to ImGuiCol_TextDisabled
        ImFont*                 monoFont = nullptr;                 // If nullptr, falls back to ambient window font
    };

    //-----------------------------------------------------------------------------
    // External interface
    //-----------------------------------------------------------------------------

    inline void Markdown( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ );

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
        TextRegion() : indentX( 0.0f ), leadIndents( 0 )
        {
        }
        ~TextRegion()
        {
            ResetIndent();
        }

        void RenderTextWrapped( const char* text_, const char* text_end_, bool bIndentToHere_ = false )
        {
#ifdef ORIGINAL
            float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
			float		scale = ImGui::GetFontScale(false);// use parent's scale
#endif
            float       widthLeft = GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // BRAT'S FIX: If the chunk doesn't fit on this line, but it WILL fit completely on the NEXT line,
            // push it down! This prevents mid-word breaks for emphasized text!
            if( endLine > text_ && endLine < text_end_ )
            {
                // Is the character at the wrap boundary inside a word?
                char c_ = *endLine;
                if( IsCharInsideWord( c_ ) )
                {
                    float lineStartX = ImGui::GetCurrentWindow()->Pos.x + ImGui::GetCurrentWindow()->DC.Indent.x;
                    float widthNextLine = widthLeft + ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
                    const char* endNextLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthNextLine );
                    if( endNextLine == text_end_ )
                    {
                        // It fits perfectly on the next line! Force a new line!
                        endLine = text_;
                        ImGui::NewLine();
                        widthLeft = ImGui::GetContentRegionAvail().x;
                        endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                        endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                    }
                }
            }
            
            ImGui::TextUnformatted( text_, endLine );
            widthLeft = GetContentRegionAvail().x;
            while( endLine < text_end_ )
            {
                text_ = endLine;
                if( *text_ == ' ' ) { ++text_; }    // skip a space at start of line
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine ) 
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

    private:
        float       indentX;
        int         leadIndents;
    };

    struct Line {
        bool isHeading = false;
        bool isEmphasis = false;
        bool isUnorderedListStart = false;
        bool isLeadingSpace = true;     // spaces at start of line
        int  leadSpaceCount = 0;
        int  headingCount = 0;
        int  emphasisCount = 0;
		char emphasisChar  = '\0';
        int  lineStart = 0;
        int  lineEnd   = 0;
        int  lastRenderPosition = 0;     // lines may get rendered in multiple pieces
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

    inline void UnderLine( ImColor col_ )
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine( min, max, col_, 1.0f );
    }

    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ )
    {
        // Apply line-level leading space indentation (nested list depth)
        int numLeadIndents = line_.leadSpaceCount / 2;
        textRegion_.ApplyLeadIndent( numLeadIndents );

        // render
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
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
        else if( line_.isHeading )          // render heading
        {
            formatInfo.level = line_.headingCount;
            formatInfo.type = MarkdownFormatType::HEADING;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            textRegion_.RenderTextWrapped( text, text + textSize - 1 );
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
            formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped( text, text + textSize );
        }
        mdConfig_.formatCallback( formatInfo, false );
    }
    
    // render markdown
    inline void Markdown( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ )
    {
        static const char* linkHoverStart = NULL; // we need to preserve status of link hovering between frames
        Line        line;
        Link        link;
        Emphasis    em;
        TextRegion  textRegion;

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
                else
                {
                    line.isLeadingSpace = false;
                    line.lastRenderPosition = i - 1;
                    if(( c == '*' || c == '-' || c == '+' ) && 
                       ( (int)markdownLength_ > i + 1 ) && ( markdown_[ i + 1 ] == ' ' ))
                    {
                        line.isUnorderedListStart = true;
                        ++i;
                        ++line.lastRenderPosition;
                        continue;
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
                }
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
                                ImGui::SameLine( 0.0f, 0.0f );
                            }
                            else if( line.isUnorderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isUnorderedListStart = false;
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
                if( c == MD_SQUARE_BRACKET_OPEN && !line.isHeading )
                {
                    link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                    link.text.start = i + 1;
                    if( i > 0 && markdown_[i - 1] == '!' )
                    {
                        link.isImage = true;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKET_OPEN:
                if( c == MD_SQUARE_BRACKET_CLOSE )
                {
                    link.state = Link::HAS_SQUARE_BRACKETS;
                    link.text.stop = i;
                }
                break;
            case Link::HAS_SQUARE_BRACKETS:
                if( c == '(' )
                {
                    link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                    link.url.start = i + 1;
                    link.num_brackets_open = 1;
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
                    if( line.lineEnd > line.lineStart )
                    {
                        RenderLine( markdown_, line, textRegion, mdConfig_ );
                        line.isUnorderedListStart = false;
                        ImGui::SameLine( 0.0f, 0.0f );
                    }
                    else if( line.isUnorderedListStart )
                    {
                        int numLeadIndents = line.leadSpaceCount / 2;
                        textRegion.ApplyLeadIndent( numLeadIndents );
                        textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                        line.isUnorderedListStart = false;
                    }
                    link.url.stop = i;
                    if( link.isImage )
                    {
                        bool drawnImage = false;
                        bool useLinkCallback = false;
                        if( mdConfig_.imageCallback )
                        {
                            MarkdownImageData imageData = mdConfig_.imageCallback( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );
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
                        if( lineEnd > line.lineStart )
                        {
                            line.lineEnd = lineEnd;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
						    ImGui::SameLine( 0.0f, 0.0f );
                            line.isUnorderedListStart = false;
                        }
                        else if( line.isUnorderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isUnorderedListStart = false;
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
                if( em.state == Emphasis::MIDDLE && line.emphasisCount >=3 &&
                    ( line.lineStart + line.emphasisCount ) == i )
                {
                    ImGui::Separator();
                }
                else
                {
                    RenderLine( markdown_, line, textRegion, mdConfig_ );
                }

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
                        if( nextC != '*' && nextC != '-' && nextC != '+' && nextC != '#' && nextC != '\n' && nextC != '\r' )
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

        if( em.state == Emphasis::LEFT && line.emphasisCount >= 3 )
        {
            ImGui::Separator();
        }
        else
        {
            if( markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[ line.lineStart ] != 0 )
            {
                line.lineEnd = (int)markdownLength_;
                if( 0 == markdown_[ line.lineEnd - 1 ] )
                {
                    --line.lineEnd;
                }
                RenderLine( markdown_, line, textRegion, mdConfig_ );
            }
        }
    }

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
#ifdef ORIGINAL
		float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
		float		scale = ImGui::GetFontScale(false);	  // use parent's scale
#endif
            float       widthLeft = GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // BRAT'S FIX FOR LINKS:
            if( endLine > text_ && endLine < text_end_ )
            {
                char c_ = *endLine;
                if( IsCharInsideWord( c_ ) )
                {
                    float lineStartX = ImGui::GetCurrentWindow()->Pos.x + ImGui::GetCurrentWindow()->DC.Indent.x;
                    float widthNextLine = widthLeft + ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
                    const char* endNextLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthNextLine );
                    if( endNextLine == text_end_ )
                    {
                        endLine = text_;
                        ImGui::NewLine();
                        widthLeft = ImGui::GetContentRegionAvail().x;
                        endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                        endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                    }
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
            widthLeft = GetContentRegionAvail().x;
            while( endLine < text_end_ )
            {
                text_ = endLine;
                if( *text_ == ' ' ) { ++text_; }
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine ) 
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
                ImGui::NewLine();
            }
            else
            {
                if( fmt.separator )
                {
                    ImGui::Separator();
                    ImGui::NewLine();
                }
                else
                {
                    ImGui::NewLine();
                }
                if( fmt.font )
                {
                    ImGui::PopFont();
                }
            }
            break;
        }
        case MarkdownFormatType::UNORDERED_LIST:
            break;
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
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_ButtonHovered ] );
                }
                else
                {
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_Button ] );
                }
            }
            break;
        }
    }

}
