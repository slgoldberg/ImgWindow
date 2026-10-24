#!/usr/bin/env python3
# ==============================================================================
# ImgWindow User Manual PDF Generator (tools/generate_manual.py)
# ------------------------------------------------------------------------------
# Author: Steven L. Goldberg (@slgoldberg)
# Adapted from Sir Lance's ReportLab manual generator pattern for FlyWithLua.
#
# Ingests contrib/ Markdown documentation and compiles a clean, publication-grade
# PDF user manual with running headers, "Page X of Y" dynamic two-pass footers,
# callout alert boxes (> [!NOTE]), styled code blocks, and zebra-striped tables.
# ==============================================================================

import os
import sys
import re
import argparse
from typing import List, Tuple

try:
    from reportlab.lib.pagesizes import letter
    from reportlab.lib import colors
    from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
    from reportlab.platypus import (
        SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable, Preformatted
    )
    from reportlab.pdfgen import canvas
except ImportError:
    print("Error: ReportLab is required. Install via: pip install reportlab", file=sys.stderr)
    sys.exit(1)


# ------------------------------------------------------------------------------
# 1. Two-Pass Numbered Canvas (Running Header & "Page X of Y" Footer)
# ------------------------------------------------------------------------------
class NumberedCanvas(canvas.Canvas):
    """
    Two-pass canvas that accumulates total page count before drawing
    running headers, rules, and dynamic 'Page X of Y' footers.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            canvas.Canvas.showPage(self)
        canvas.Canvas.save(self)

    def draw_page_decorations(self, page_count):
        if self._pageNumber == 1:
            # Suppress header and footer on cover / title page
            return

        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#64748B"))

        # Running Header
        header_text = "ImgWindow Contrib Library — Developer & User Manual"
        self.drawString(54, 750, header_text)
        self.setStrokeColor(colors.HexColor("#CBD5E1"))
        self.setLineWidth(0.5)
        self.line(54, 744, 558, 744)

        # Running Footer
        self.line(54, 48, 558, 48)
        footer_left = "ImgWindow for X-Plane • Author: Steven L. Goldberg (@slgoldberg)"
        footer_right = f"Page {self._pageNumber} of {page_count}"
        self.drawString(54, 36, footer_left)
        self.drawRightString(558, 36, footer_right)

        self.restoreState()


# ------------------------------------------------------------------------------
# 2. Markdown Inline Parser & HTML Sanitizer
# ------------------------------------------------------------------------------
def sanitize_inline_markdown(text: str) -> str:
    """
    Converts Markdown inline syntax (**bold**, *italic*, `code`, ~~strike~~, ++ins++)
    to ReportLab-compatible HTML markup safely.
    """
    # 1. Extract and protect backtick inline code spans with unique placeholders
    code_spans = []
    def save_code(m):
        code_content = m.group(1).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        idx = len(code_spans)
        code_spans.append(f'<font name="Courier-Bold" color="#B91C1C"><b>{code_content}</b></font>')
        return f"___CODE_SPAN_{idx}___"

    text = re.sub(r'`([^`]+)`', save_code, text)

    # 2. Escape raw XML ampersands
    text = text.replace("&", "&amp;")

    # 3. Bold: **bold**
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)

    # 4. Italic: *italic* (avoid matching solitary asterisks or bullet starts)
    text = re.sub(r'(?<!\*)\*([^\*\s][^\*]*?[^\*\s]|\S)\*(?!\*)', r'<i>\1</i>', text)

    # 5. Italic via underscores: only when bounded by whitespace/punctuation (never intra-word like FOO_BAR_BAZ)
    text = re.sub(r'(?<=\s)_([^_]+?)_(?=\s|[.,;:!?\)]|$)', r'<i>\1</i>', text)

    # 6. Underline: ++text++
    text = re.sub(r'\+\+([^\+]+?)\+\+', r'<u>\1</u>', text)

    # 7. Strikethrough: ~~text~~ or $$strike$$
    text = re.sub(r'~~([^~]+?)~~', r'<font color="#94A3B8">\1</font>', text)
    text = re.sub(r'\$\$([^\$]+?)\$\$', r'<font color="#94A3B8">\1</font>', text)

    # 8. Links: [label](url)
    def link_repl(m):
        label, url = m.group(1), m.group(2)
        return f'<link href="{url}" color="#2563EB"><u>{label}</u></link>'
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link_repl, text)

    # 9. Transform or sanitize custom tags (<badge>, <pill>, <kbd>)
    valid_tags = {'b', '/b', 'i', '/i', 'u', '/u', 'font', '/font', 'link', '/link', 'a', '/a'}
    def tag_filter(m):
        raw = m.group(0)
        tag_name = m.group(1).split()[0].lower()
        if tag_name in valid_tags or tag_name.startswith('font ') or tag_name.startswith('link '):
            return raw
        if any(tag_name.startswith(k) for k in ('badge', 'pill', 'kbd', 'tag', 'btn', 'key', 'code', 'mono')):
            return f'<font name="Helvetica-Bold" color="#1E293B">[{raw[1:-1]}]</font>'
        return raw.replace("<", "&lt;").replace(">", "&gt;")

    text = re.sub(r'</?([a-zA-Z0-9_\-=:#, ]+)>', tag_filter, text)

    # 10. Escape any remaining stray angle brackets
    text = re.sub(r'<(?!(?:b|/b|i|/i|u|/u|font|/font|link|/link|a|/a)(?:\s|>))', '&lt;', text)

    # 11. Restore code spans
    for idx, span in enumerate(code_spans):
        text = text.replace(f"___CODE_SPAN_{idx}___", span)

    return text


def safe_paragraph(raw_text: str, style) -> Paragraph:
    """
    Safely creates a Paragraph flowable with graceful plain-text fallback
    if malformed markup is encountered.
    """
    formatted = sanitize_inline_markdown(raw_text)
    try:
        return Paragraph(formatted, style)
    except Exception:
        plain = re.sub(r'<[^>]+>', '', raw_text)
        plain = plain.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        return Paragraph(plain, style)


# ------------------------------------------------------------------------------
# 3. Document Parser: Markdown Blocks to ReportLab Flowables
# ------------------------------------------------------------------------------
def build_styles():
    styles = getSampleStyleSheet()

    navy = colors.HexColor("#1E3A8A")
    slate_body = colors.HexColor("#334155")

    styles.add(ParagraphStyle(
        name='ManualTitle',
        fontName='Helvetica-Bold',
        fontSize=24,
        leading=30,
        textColor=navy,
        spaceAfter=6
    ))

    styles.add(ParagraphStyle(
        name='ManualSubtitle',
        fontName='Helvetica',
        fontSize=11,
        leading=15,
        textColor=colors.HexColor("#475569"),
        spaceAfter=16
    ))

    styles['Heading1'].fontName = 'Helvetica-Bold'
    styles['Heading1'].fontSize = 17
    styles['Heading1'].leading = 21
    styles['Heading1'].textColor = navy
    styles['Heading1'].spaceBefore = 14
    styles['Heading1'].spaceAfter = 8
    styles['Heading1'].keepWithNext = True

    styles['Heading2'].fontName = 'Helvetica-Bold'
    styles['Heading2'].fontSize = 12.5
    styles['Heading2'].leading = 16
    styles['Heading2'].textColor = colors.HexColor("#1E293B")
    styles['Heading2'].spaceBefore = 11
    styles['Heading2'].spaceAfter = 6
    styles['Heading2'].keepWithNext = True

    styles['Heading3'].fontName = 'Helvetica-Bold'
    styles['Heading3'].fontSize = 10.5
    styles['Heading3'].leading = 14
    styles['Heading3'].textColor = colors.HexColor("#334155")
    styles['Heading3'].spaceBefore = 8
    styles['Heading3'].spaceAfter = 4
    styles['Heading3'].keepWithNext = True

    styles['BodyText'].fontName = 'Helvetica'
    styles['BodyText'].fontSize = 9.0
    styles['BodyText'].leading = 13.0
    styles['BodyText'].textColor = slate_body
    styles['BodyText'].spaceAfter = 5

    styles.add(ParagraphStyle(
        name='BulletItem',
        fontName='Helvetica',
        fontSize=9.0,
        leading=13.0,
        textColor=slate_body,
        leftIndent=14,
        firstLineIndent=-10,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='NumberedItem',
        fontName='Helvetica',
        fontSize=9.0,
        leading=13.0,
        textColor=slate_body,
        leftIndent=18,
        firstLineIndent=-14,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='CodeSnippet',
        fontName='Courier',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.HexColor("#F8FAFC")
    ))

    styles.add(ParagraphStyle(
        name='AlertTitle',
        fontName='Helvetica-Bold',
        fontSize=9.0,
        leading=11.5,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='AlertBody',
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=slate_body
    ))

    styles.add(ParagraphStyle(
        name='TableHeader',
        fontName='Helvetica-Bold',
        fontSize=8.0,
        leading=10.5,
        textColor=colors.white
    ))

    styles.add(ParagraphStyle(
        name='TableCell',
        fontName='Helvetica',
        fontSize=7.5,
        leading=10.0,
        textColor=slate_body
    ))

    return styles


def parse_markdown_to_flowables(md_content: str, styles) -> List:
    flowables = []
    lines = md_content.splitlines()
    i = 0
    total_lines = len(lines)

    in_code_block = False
    code_lines = []
    code_lang = ""

    in_alert = False
    alert_type = ""
    alert_lines = []

    in_table = False
    table_rows = []

    CONTENT_WIDTH = 490.0

    def flush_alert():
        nonlocal in_alert, alert_type, alert_lines
        if not alert_lines:
            in_alert = False
            return

        theme_colors = {
            "NOTE": (colors.HexColor("#2563EB"), colors.HexColor("#EFF6FF")),
            "TIP": (colors.HexColor("#059669"), colors.HexColor("#ECFDF5")),
            "IMPORTANT": (colors.HexColor("#7C3AED"), colors.HexColor("#F5F3FF")),
            "WARNING": (colors.HexColor("#D97706"), colors.HexColor("#FFFBEB")),
            "CAUTION": (colors.HexColor("#DC2626"), colors.HexColor("#FEF2F2")),
        }
        accent, bg = theme_colors.get(alert_type, (colors.HexColor("#475569"), colors.HexColor("#F1F5F9")))

        alert_text = " ".join(alert_lines).strip()
        body_p = safe_paragraph(alert_text, styles['AlertBody'])

        title_style = ParagraphStyle(
            'AlertHeaderSpecific',
            parent=styles['AlertTitle'],
            textColor=accent
        )
        title_p = Paragraph(f"<b>{alert_type}</b>", title_style)

        table_data = [[title_p], [body_p]]
        tbl = Table(table_data, colWidths=[CONTENT_WIDTH])
        tbl.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, -1), bg),
            ('LINELEFT', (0, 0), (0, -1), 3.0, accent),
            ('TOPPADDING', (0, 0), (-1, -1), 4),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
            ('LEFTPADDING', (0, 0), (-1, -1), 10),
            ('RIGHTPADDING', (0, 0), (-1, -1), 8),
        ]))
        flowables.append(Spacer(1, 4))
        flowables.append(tbl)
        flowables.append(Spacer(1, 6))

        in_alert = False
        alert_type = ""
        alert_lines = []

    def flush_table():
        nonlocal in_table, table_rows
        if not table_rows:
            in_table = False
            return

        filtered_rows = []
        for r in table_rows:
            if all(re.match(r'^\s*[:\-]+\s*$', cell) for cell in r if cell):
                continue
            filtered_rows.append(r)

        if not filtered_rows:
            in_table = False
            table_rows = []
            return

        num_cols = max(len(r) for r in filtered_rows)
        col_width = CONTENT_WIDTH / num_cols if num_cols > 0 else CONTENT_WIDTH
        col_widths = [col_width] * num_cols

        formatted_data = []
        for row_idx, row in enumerate(filtered_rows):
            padded_row = row + [""] * (num_cols - len(row))
            row_cells = []
            for cell in padded_row:
                style_to_use = styles['TableHeader'] if row_idx == 0 else styles['TableCell']
                row_cells.append(safe_paragraph(cell, style_to_use))
            formatted_data.append(row_cells)

        tbl = Table(formatted_data, colWidths=col_widths, repeatRows=1)
        t_style = [
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#1E3A8A")),
            ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
            ('VALIGN', (0, 0), (-1, -1), 'TOP'),
            ('GRID', (0, 0), (-1, -1), 0.5, colors.HexColor("#CBD5E1")),
            ('TOPPADDING', (0, 0), (-1, -1), 3),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
            ('LEFTPADDING', (0, 0), (-1, -1), 4),
            ('RIGHTPADDING', (0, 0), (-1, -1), 4),
        ]
        for r_i in range(1, len(formatted_data)):
            if r_i % 2 == 0:
                t_style.append(('BACKGROUND', (0, r_i), (-1, r_i), colors.HexColor("#F8FAFC")))
        tbl.setStyle(TableStyle(t_style))

        flowables.append(Spacer(1, 4))
        flowables.append(tbl)
        flowables.append(Spacer(1, 6))

        in_table = False
        table_rows = []

    while i < total_lines:
        line = lines[i]

        # Fenced code block (``` or ~~~)
        if line.strip().startswith("```") or line.strip().startswith("~~~"):
            if in_code_block:
                # Chunk code lines into rows of max 20 lines so multi-page code blocks break cleanly
                CHUNK_SIZE = 20
                table_data = []
                for chunk_idx in range(0, len(code_lines), CHUNK_SIZE):
                    chunk_slice = code_lines[chunk_idx:chunk_idx + CHUNK_SIZE]
                    raw_code = "\n".join(chunk_slice)
                    escaped_code = raw_code.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

                    cell_elements = []
                    if chunk_idx == 0 and code_lang:
                        chip_style = ParagraphStyle(
                            'LangChip',
                            fontName='Helvetica-Bold',
                            fontSize=7.0,
                            textColor=colors.HexColor("#94A3B8"),
                            spaceAfter=2
                        )
                        cell_elements.append(Paragraph(f"<b>[{code_lang.upper()}]</b>", chip_style))
                    cell_elements.append(Preformatted(escaped_code, styles['CodeSnippet']))
                    table_data.append([cell_elements])

                if not table_data:
                    table_data = [[[Preformatted("", styles['CodeSnippet'])]]]

                tbl = Table(table_data, colWidths=[CONTENT_WIDTH])
                tbl.setStyle(TableStyle([
                    ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#0F172A")),
                    ('BOX', (0, 0), (-1, -1), 0.5, colors.HexColor("#334155")),
                    ('LEFTPADDING', (0, 0), (-1, -1), 8),
                    ('RIGHTPADDING', (0, 0), (-1, -1), 8),
                    ('TOPPADDING', (0, 0), (-1, -1), 3),
                    ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
                ]))
                flowables.append(Spacer(1, 4))
                flowables.append(tbl)
                flowables.append(Spacer(1, 6))

                in_code_block = False
                code_lines = []
                code_lang = ""
            else:
                if in_alert: flush_alert()
                if in_table: flush_table()
                in_code_block = True
                code_lang = line.strip()[3:].strip()
                code_lines = []
            i += 1
            continue

        if in_code_block:
            code_lines.append(line)
            i += 1
            continue

        # Alert Callouts (> [!NOTE])
        alert_match = re.match(r'^\s*>\s*\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*(.*)$', line, re.IGNORECASE)
        if alert_match:
            if in_alert: flush_alert()
            if in_table: flush_table()
            in_alert = True
            alert_type = alert_match.group(1).upper()
            initial_text = alert_match.group(2).strip()
            alert_lines = [initial_text] if initial_text else []
            i += 1
            continue

        if in_alert:
            if line.strip().startswith(">"):
                alert_lines.append(line.strip().lstrip(">").strip())
                i += 1
                continue
            else:
                flush_alert()

        # Tables (| cell | cell |)
        if line.strip().startswith("|") and line.strip().endswith("|"):
            if not in_table:
                in_table = True
                table_rows = []
            cells = [c.strip() for c in line.strip().split("|")[1:-1]]
            table_rows.append(cells)
            i += 1
            continue
        elif in_table:
            flush_table()

        if not line.strip():
            i += 1
            continue

        # Horizontal Rule
        if re.match(r'^\s*[-*_]{3,}\s*$', line):
            flowables.append(Spacer(1, 4))
            flowables.append(HRFlowable(width="100%", thickness=0.5, color=colors.HexColor("#CBD5E1"), spaceAfter=8, spaceBefore=4))
            i += 1
            continue

        # Headings
        if line.startswith("# "):
            flowables.append(safe_paragraph(line[2:].strip(), styles['Heading1']))
            i += 1
            continue
        elif line.startswith("## "):
            flowables.append(safe_paragraph(line[3:].strip(), styles['Heading2']))
            i += 1
            continue
        elif line.startswith("### "):
            flowables.append(safe_paragraph(line[4:].strip(), styles['Heading3']))
            i += 1
            continue
        elif line.startswith("#### "):
            flowables.append(safe_paragraph(f"<b>{line[5:].strip()}</b>", styles['BodyText']))
            i += 1
            continue

        # Bullet List Items
        bullet_match = re.match(r'^\s*[\*\-\+]\s+(.*)$', line)
        if bullet_match:
            flowables.append(safe_paragraph(f"• {bullet_match.group(1).strip()}", styles['BulletItem']))
            i += 1
            continue

        # Numbered List Items
        num_match = re.match(r'^\s*(\d+)[\.\)]\s+(.*)$', line)
        if num_match:
            item_num = num_match.group(1)
            flowables.append(safe_paragraph(f"{item_num}. {num_match.group(2).strip()}", styles['NumberedItem']))
            i += 1
            continue

        # Standard Paragraph
        flowables.append(safe_paragraph(line.strip(), styles['BodyText']))
        i += 1

    if in_alert: flush_alert()
    if in_table: flush_table()

    return flowables


# ------------------------------------------------------------------------------
# 4. Main Document Orchestrator
# ------------------------------------------------------------------------------
def generate_pdf(output_path: str, source_files: List[str]):
    print(f"[PDF Generator] Preparing manual: {output_path}")
    styles = build_styles()
    story = []

    # Title Page Banner
    story.append(Paragraph("ImgWindow Contrib Library", styles['ManualTitle']))
    story.append(Paragraph("Modern UI Widgets & Extensions for Dear ImGui in X-Plane Plugins", styles['ManualSubtitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#1E3A8A"), spaceAfter=14))

    for idx, filepath in enumerate(source_files):
        if not os.path.exists(filepath):
            print(f"[Warning] File not found: {filepath}", file=sys.stderr)
            continue

        print(f"[PDF Generator] Ingesting: {filepath}")
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()

        if idx > 0:
            story.append(PageBreak())

        flowables = parse_markdown_to_flowables(content, styles)
        story.extend(flowables)

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    doc.build(story, canvasmaker=NumberedCanvas)
    file_size_kb = os.path.getsize(output_path) / 1024.0
    print(f"[PDF Generator] Manual created successfully: {output_path} ({file_size_kb:.1f} KB)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate publication-grade PDF manual from ImgWindow contrib markdown documentation.")
    parser.add_argument("--output", "-o", default="docs/ImgWindow_Contrib_Manual.pdf", help="Output PDF file path")
    parser.add_argument("inputs", nargs="*", help="Markdown files to include (defaults to all contrib docs)")

    args = parser.parse_args()

    if not args.inputs:
        repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        default_docs = [
            os.path.join(repo_root, "contrib", "README.md"),
            os.path.join(repo_root, "contrib", "widgets", "README.md"),
            os.path.join(repo_root, "contrib", "extensions", "Markdown", "README.md"),
            os.path.join(repo_root, "contrib", "extensions", "TimedTooltip", "README.md"),
        ]
        args.inputs = [f for f in default_docs if os.path.exists(f)]

    generate_pdf(args.output, args.inputs)
