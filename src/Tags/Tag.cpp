#include "Tag.hpp"
#include "TagProfileManager.hpp"
#include "Logger.hpp"
#include <cmath>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

namespace
{
constexpr int TAG_PADDING           = 2;
constexpr int TAG_BOX_LINE_WIDTH    = 1;
constexpr int CONNECTION_LINE_WIDTH = 1;

constexpr COLORREF DEFAULT_TAG_BG_COLOR   = RGB(33, 115, 196);  // Blue
constexpr COLORREF DEFAULT_TAG_TEXT_COLOR = RGB(255, 255, 255); // White
constexpr COLORREF DEFAULT_BORDER_COLOR   = RGB(255, 255, 255); // White

constexpr const char * FONT_NAME = "Arial";
constexpr int FONT_HEIGHT        = 22;
constexpr int FONT_WEIGHT        = FW_BOLD;

/**
 * Structure to hold measured tag dimensions
 */
struct TagDimensions
{
    int maxLineWidth;
    int totalHeight;
    int lineHeight;
    std::vector<int> lineWidths;
};

/**
 * Create and select the standard tag font
 * @return Handle to created font (must be deleted by caller)
 */
HFONT CreateTagFont(HDC hDC)
{
    HFONT hFont = CreateFontA(FONT_HEIGHT, 0, 0, 0, FONT_WEIGHT, FALSE, FALSE,
                              FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, FONT_NAME);
    return hFont;
}

/**
 * Liang-Barsky line clipping algorithm to clip a line to a rectangle
 */
bool LiangBarskyClip(const RECT & rect,
                     POINT p1,
                     POINT p2,
                     POINT & clippedStart,
                     POINT & clippedEnd)
{
    double x0 = static_cast<double>(p1.x);
    double y0 = static_cast<double>(p1.y);
    double x1 = static_cast<double>(p2.x);
    double y1 = static_cast<double>(p2.y);

    double dx = x1 - x0;
    double dy = y1 - y0;

    double t0 = 0.0;
    double t1 = 1.0;

    double xmin = static_cast<double>(rect.left);
    double xmax = static_cast<double>(rect.right);
    double ymin = static_cast<double>(rect.top);
    double ymax = static_cast<double>(rect.bottom);

    double p[4] = {-dx, dx, -dy, dy};
    double q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};

    for (int i = 0; i < 4; ++i)
    {
        if (std::abs(p[i]) < 1e-10)
        {
            if (q[i] < 0.0) return false;
        }
        else
        {
            double r = q[i] / p[i];
            if (p[i] < 0.0) { t0 = (t0 > r) ? t0 : r; }
            else { t1 = (t1 < r) ? t1 : r; }

            if (t0 > t1) return false;
        }
    }

    clippedStart.x = static_cast<LONG>(x0 + t0 * dx);
    clippedStart.y = static_cast<LONG>(y0 + t0 * dy);
    clippedEnd.x   = static_cast<LONG>(x0 + t1 * dx);
    clippedEnd.y   = static_cast<LONG>(y0 + t1 * dy);

    return true;
}

/**
 * Draw a leader line from aircraft to tag edge using Liang-Barsky clipping
 */
void DrawLeaderLine(HDC hDC,
                    POINT aircraftScreenPos,
                    POINT tagCenter,
                    const RECT & tagRect)
{
    POINT lineStart, lineEnd;
    if (!LiangBarskyClip(tagRect, aircraftScreenPos, tagCenter, lineStart,
                         lineEnd))
    {
        return;
    }

    HPEN linePen =
        CreatePen(PS_SOLID, CONNECTION_LINE_WIDTH, RGB(255, 255, 255));
    HPEN oldPen = (HPEN)SelectObject(hDC, linePen);

    MoveToEx(hDC, aircraftScreenPos.x, aircraftScreenPos.y, nullptr);
    LineTo(hDC, lineStart.x, lineStart.y);

    SelectObject(hDC, oldPen);
    DeleteObject(linePen);
}

/**
 * Draw the tag background rectangle using GDI+
 */
void DrawTagBackground(Gdiplus::Graphics & graphics, const RECT & rect, Gdiplus::Color backgroundColor)
{
    Gdiplus::SolidBrush bgBrush(backgroundColor);
    graphics.FillRectangle(&bgBrush, 
                           static_cast<Gdiplus::REAL>(rect.left),
                           static_cast<Gdiplus::REAL>(rect.top),
                           static_cast<Gdiplus::REAL>(rect.right - rect.left),
                           static_cast<Gdiplus::REAL>(rect.bottom - rect.top));
}

/**
 * Measure dimensions for multi-line tag content
 */
TagDimensions MeasureTagLines(HDC hDC,
                              const TagData & tagData,
                              const std::vector<TagLine> & tagLines,
                              int spaceWidth)
{
    TagDimensions dims = {0, 0, 0, {}};

    for (const auto & line : tagLines)
    {
        int lineWidth    = 0;
        bool hasContent  = false;
        int elementCount = 0;

        for (const auto & itemName : line)
        {
            TagItemType itemType = Tag::ParseTagItemType(itemName);
            auto it              = tagData.items.find(itemType);
            std::string text = (it != tagData.items.end()) ? it->second : "";

            if (!text.empty())
            {
                hasContent = true;
                SIZE textSize;
                GetTextExtentPoint32A(hDC, text.c_str(),
                                      static_cast<int>(text.length()),
                                      &textSize);
                lineWidth += textSize.cx;
                dims.lineHeight = textSize.cy;

                if (elementCount < static_cast<int>(line.size()) - 1)
                {
                    lineWidth += spaceWidth;
                }
            }
            elementCount++;
        }

        dims.lineWidths.push_back(lineWidth);
        if (hasContent)
        {
            dims.maxLineWidth =
                (dims.maxLineWidth > lineWidth) ? dims.maxLineWidth : lineWidth;
            dims.totalHeight += dims.lineHeight;
            // Add line spacing between lines
            if (dims.totalHeight > dims.lineHeight) { dims.totalHeight += 2; }
        }
    }

    return dims;
}

/**
 * Draw multi-line tag text content using GDI+
 */
void DrawMultiLineTagText(Gdiplus::Graphics & graphics,
                          const RECT & tagRect,
                          const TagData & tagData,
                          const std::vector<TagLine> & tagLines,
                          int lineHeight,
                          int spaceWidth,
                          Gdiplus::Color textColor,
                          Gdiplus::Font & font)
{
    Gdiplus::SolidBrush textBrush(textColor);
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentNear);
    format.SetLineAlignment(Gdiplus::StringAlignmentNear);

    Gdiplus::REAL currentY = static_cast<Gdiplus::REAL>(tagRect.top + TAG_PADDING);

    for (size_t lineIdx = 0; lineIdx < tagLines.size(); ++lineIdx)
    {
        const auto & line = tagLines[lineIdx];
        bool hasContent   = false;
        Gdiplus::REAL currentX     = static_cast<Gdiplus::REAL>(tagRect.left + TAG_PADDING);

        // Check if line has content
        for (const auto & itemName : line)
        {
            TagItemType itemType = Tag::ParseTagItemType(itemName);
            auto it              = tagData.items.find(itemType);
            if (it != tagData.items.end() && !it->second.empty())
            {
                hasContent = true;
                break;
            }
        }

        if (!hasContent) continue;

        // Draw each element in the line
        for (size_t elemIdx = 0; elemIdx < line.size(); ++elemIdx)
        {
            const auto & itemName = line[elemIdx];
            TagItemType itemType  = Tag::ParseTagItemType(itemName);
            auto it               = tagData.items.find(itemType);
            std::string text = (it != tagData.items.end()) ? it->second : "";

            if (!text.empty())
            {
                // Convert to wide string for GDI+
                std::wstring wtext(text.begin(), text.end());
                graphics.DrawString(wtext.c_str(), -1, &font,
                                   Gdiplus::PointF(currentX, currentY), &textBrush);

                // Measure text width
                Gdiplus::RectF boundingBox;
                graphics.MeasureString(wtext.c_str(), -1, &font,
                                      Gdiplus::PointF(0, 0), &boundingBox);
                currentX += boundingBox.Width;

                // Check if there's more content after this element
                bool hasNextContent = false;
                for (size_t k = elemIdx + 1; k < line.size(); ++k)
                {
                    TagItemType nextType = Tag::ParseTagItemType(line[k]);
                    auto nextIt          = tagData.items.find(nextType);
                    if (nextIt != tagData.items.end() &&
                        !nextIt->second.empty())
                    {
                        hasNextContent = true;
                        break;
                    }
                }
                if (hasNextContent)
                {
                    Gdiplus::RectF spaceBox;
                    graphics.MeasureString(L" ", 1, &font, Gdiplus::PointF(0, 0),
                                          &spaceBox);
                    currentX += spaceBox.Width;
                }
            }
        }

        currentY += lineHeight;
    }
}

} // namespace

TagItemType Tag::ParseTagItemType(const std::string & itemName)
{
    static const std::map<std::string, TagItemType> typeMap = {
        {"callsign", TagItemType::Callsign},
        {"actype", TagItemType::AcType},
        {"sctype", TagItemType::ScType},
        {"sqerror", TagItemType::SqError},
        {"deprwy", TagItemType::DepRwy},
        {"seprwy", TagItemType::SepRwy},
        {"arvrwy", TagItemType::ArvRwy},
        {"srvrwy", TagItemType::SrvRwy},
        {"gate", TagItemType::Gate},
        {"sate", TagItemType::Sate},
        {"flightlevel", TagItemType::FlightLevel},
        {"gs", TagItemType::GroundSpeed},
        {"tendency", TagItemType::Tendency},
        {"wake", TagItemType::Wake},
        {"groundstatus", TagItemType::GroundStatus},
        {"tssr", TagItemType::SSR},
        {"ssr", TagItemType::SSR},
        {"asid", TagItemType::SID},
        {"ssid", TagItemType::ShortSID},
        {"origin", TagItemType::Origin},
        {"dep", TagItemType::Origin},
        {"dest", TagItemType::Dest},
        {"systemid", TagItemType::SystemId},
        {"uk_stand", TagItemType::UkStand}};

    auto it = typeMap.find(itemName);
    if (it != typeMap.end()) { return it->second; }

    // Default to callsign if unknown
    return TagItemType::Callsign;
}

RECT Tag::DrawTagForAircraft(HDC hDC,
                             POINT aircraftScreenPos,
                             const TagData & tagData,
                             const TagProfileManager & profileManager,
                             bool isCorrelated,
                             bool isDeparture,
                             bool isArrival,
                             double groundSpeed,
                             int tagOffsetX,
                             int tagOffsetY)
{
    int tagType =
        DetermineTagType(isCorrelated, isDeparture, isArrival, groundSpeed);
    return DrawProfileTag(hDC, aircraftScreenPos, tagData, profileManager,
                          tagType, tagOffsetX, tagOffsetY);
}

RECT Tag::DrawProfileTag(HDC hDC,
                         POINT aircraftScreenPos,
                         const TagData & tagData,
                         const TagProfileManager & profileManager,
                         int tagTypeValue,
                         int tagOffsetX,
                         int tagOffsetY)
{
    // Return empty rect if no tag should be displayed (-1)
    if (tagTypeValue == -1)
    {
        return {0, 0, 0, 0};
    }

    TagType tagType = static_cast<TagType>(tagTypeValue);
    const TagDefinition * tagDef = profileManager.GetTagDefinition(tagType);

    if (!tagDef)
    {
        // Fallback to airborne tag if type not found
        tagDef = profileManager.GetTagDefinition(TagType::Airborne);
        if (!tagDef)
        {
            return {0, 0, 0, 0};
        }
    }

    // Build tag lines from definition
    std::vector<TagLine> tagLines = tagDef->lines;

    const TagFormattingSettings & formatting =
        profileManager.GetFormattingSettings();

    return DrawMultiLineTag(hDC,
                            aircraftScreenPos,
                            tagData,
                            tagLines,
                            tagOffsetX,
                            tagOffsetY,
                            tagDef->backgroundColor.ToCOLORREF(),
                            tagDef->textColor.ToCOLORREF(),
                            formatting.borderColor.ToCOLORREF());
}

RECT Tag::DrawMultiLineTag(HDC hDC,
                           POINT aircraftScreenPos,
                           const TagData & tagData,
                           const std::vector<TagLine> & tagLines,
                           int tagOffsetX,
                           int tagOffsetY,
                           COLORREF backgroundColor,
                           COLORREF textColor,
                           COLORREF borderColor)
{
    Logger::getInstance().debug("DrawMultiLineTag: Starting tag rendering");
    
    POINT tagCenter;
    tagCenter.x = aircraftScreenPos.x + tagOffsetX;
    tagCenter.y = aircraftScreenPos.y + tagOffsetY;

    // Create GDI+ Graphics object from HDC
    Gdiplus::Graphics graphics(hDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

    // Create font for GDI+
    Gdiplus::Font gdiPlusFont(L"Arial", 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

    // Measure all tag lines using GDI (for accurate dimensions)
    HFONT hFont   = CreateTagFont(hDC);
    HFONT oldFont = (HFONT)SelectObject(hDC, hFont);

    SIZE spaceSize;
    GetTextExtentPoint32A(hDC, " ", 1, &spaceSize);
    int spaceWidth = spaceSize.cx;

    TagDimensions dims = MeasureTagLines(hDC, tagData, tagLines, spaceWidth);

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);

    if (dims.totalHeight == 0)
    {
        Logger::getInstance().debug("DrawMultiLineTag: No content to render");
        return {0, 0, 0, 0};
    }

    // Calculate tag rectangle
    int boxWidth  = dims.maxLineWidth + (TAG_PADDING * 2);
    int boxHeight = dims.totalHeight + (TAG_PADDING * 2);

    int left   = tagCenter.x - (boxWidth / 2);
    int top    = tagCenter.y - (boxHeight / 2);
    int right  = left + boxWidth;
    int bottom = top + boxHeight;

    RECT boxRect = {left, top, right, bottom};

    Logger::getInstance().debug("DrawMultiLineTag: Drawing background at (" + 
                                std::to_string(left) + "," + std::to_string(top) + ")");

    // Draw leader line (GDI)
    DrawLeaderLine(hDC, aircraftScreenPos, tagCenter, boxRect);

    // Draw background (GDI+)
    Gdiplus::Color bgColor(GetRValue(backgroundColor), GetGValue(backgroundColor),
                  GetBValue(backgroundColor));
    DrawTagBackground(graphics, boxRect, bgColor);

    Logger::getInstance().debug("DrawMultiLineTag: Drawing text content");

    // Draw text content (GDI+)
    Gdiplus::Color txtColor(GetRValue(textColor), GetGValue(textColor),
                   GetBValue(textColor));
    DrawMultiLineTagText(graphics, boxRect, tagData, tagLines,
                        dims.lineHeight, spaceWidth, txtColor, gdiPlusFont);

    Logger::getInstance().debug("DrawMultiLineTag: Tag rendering complete");

    return boxRect;

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);

    return boxRect;
}

// OLD FUNCTION - Kept for compatibility but not actively used
// Use DrawTagForAircraft instead which handles state-based rendering
RECT Tag::DrawTag(HDC hDC,
                  POINT aircraftScreenPos,
                  const std::string & callsign,
                  int tagOffsetX,
                  int tagOffsetY)
{
    // Create GDI+ Graphics object  
    Gdiplus::Graphics graphics(hDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

    Gdiplus::Font font(L"Arial", 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

    // Measure text
    Gdiplus::RectF boundingBox;
    std::wstring wcallsign(callsign.begin(), callsign.end());
    graphics.MeasureString(wcallsign.c_str(), -1, &font,
                          Gdiplus::PointF(0, 0), &boundingBox);

    POINT tagCenter;
    tagCenter.x = aircraftScreenPos.x + tagOffsetX;
    tagCenter.y = aircraftScreenPos.y + tagOffsetY;

    int boxWidth  = static_cast<int>(boundingBox.Width) + (TAG_PADDING * 2);
    int boxHeight = static_cast<int>(boundingBox.Height) + (TAG_PADDING * 2);

    int left   = tagCenter.x - (boxWidth / 2);
    int top    = tagCenter.y - (boxHeight / 2);
    int right  = left + boxWidth;
    int bottom = top + boxHeight;

    RECT boxRect = {left, top, right, bottom};

    DrawLeaderLine(hDC, aircraftScreenPos, tagCenter, boxRect);

    Gdiplus::Color bgColor(40, 50, 200);
    DrawTagBackground(graphics, boxRect, bgColor);

    graphics.DrawString(wcallsign.c_str(), -1, &font,
                       Gdiplus::PointF(static_cast<Gdiplus::REAL>(left + TAG_PADDING),
                                      static_cast<Gdiplus::REAL>(top + TAG_PADDING)),
                       &textBrush);

    return boxRect;
}

bool Tag::GetTextRect(HDC hDC, const std::string & text, RECT & outRect)
{
    HFONT hFont   = CreateTagFont(hDC);
    HFONT oldFont = (HFONT)SelectObject(hDC, hFont);

    SIZE textSize;
    if (!GetTextExtentPointA(hDC, text.c_str(), static_cast<int>(text.length()),
                             &textSize))
    {
        SelectObject(hDC, oldFont);
        DeleteObject(hFont);
        return false;
    }

    outRect.left   = 0;
    outRect.top    = 0;
    outRect.right  = textSize.cx + (TAG_PADDING * 2);
    outRect.bottom = textSize.cy + (TAG_PADDING * 2);

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);
    return true;
}

int Tag::DetermineTagType(bool isCorrelated,
                          bool isDeparture,
                          bool isArrival,
                          double groundSpeed)
{
    // Uncorrelated aircraft
    if (!isCorrelated)
    {
        // Stationary uncorrelated: no tag
        if (groundSpeed < 5.0)
        {
            return -1; // No tag
        }
        // Moving uncorrelated: show uncorrelated tag
        return static_cast<int>(TagType::Uncorrelated);
    }

    // Correlated aircraft - check ground speed threshold
    if (groundSpeed >= 80.0)
    {
        // High speed: always airborne tag
        return static_cast<int>(TagType::Airborne);
    }

    // Low speed: use departure/arrival status
    if (isDeparture)
    {
        return static_cast<int>(TagType::Departure);
    }

    if (isArrival)
    {
        return static_cast<int>(TagType::Arrival);
    }

    // Default to airborne if no departure/arrival info
    return static_cast<int>(TagType::Airborne);
}
