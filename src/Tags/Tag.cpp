#include "Tag.hpp"
#include <cmath>

namespace
{
constexpr int TAG_PADDING           = 2;
constexpr int TAG_BOX_LINE_WIDTH    = 1;
constexpr int CONNECTION_LINE_WIDTH = 1;

constexpr COLORREF DEFAULT_TAG_BG_COLOR   = RGB(33, 115, 196);  // Blue
constexpr COLORREF DEFAULT_TAG_TEXT_COLOR = RGB(255, 255, 255); // White
constexpr COLORREF DEFAULT_BORDER_COLOR   = RGB(255, 255, 255); // White

constexpr const char * FONT_NAME = "EuroScope";
constexpr int FONT_HEIGHT        = 14;
constexpr int FONT_WEIGHT        = FW_NORMAL;

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
    POINT tagCenter;
    tagCenter.x = aircraftScreenPos.x + tagOffsetX;
    tagCenter.y = aircraftScreenPos.y + tagOffsetY;

    HFONT hFont = CreateFontA(FONT_HEIGHT, 0, 0, 0, FONT_WEIGHT, FALSE, FALSE,
                              FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, FONT_NAME);

    HFONT oldFont = (HFONT)SelectObject(hDC, hFont);

    // Measure space width
    SIZE spaceSize;
    GetTextExtentPoint32A(hDC, " ", 1, &spaceSize);
    int spaceWidth = spaceSize.cx;

    int maxLineWidth = 0;
    int totalHeight  = 0;
    int lineHeight   = 0;
    std::vector<int> lineWidths;

    for (const auto & line : tagLines)
    {
        int lineWidth    = 0;
        bool hasContent  = false;
        int elementCount = 0;

        for (const auto & itemName : line)
        {
            TagItemType itemType = ParseTagItemType(itemName);
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
                lineHeight = textSize.cy; // All lines should have same height

                // Add space between elements (but not after last element)
                if (elementCount < static_cast<int>(line.size()) - 1)
                {
                    lineWidth += spaceWidth;
                }
            }
            elementCount++;
        }

        lineWidths.push_back(lineWidth);
        if (hasContent)
        {
            maxLineWidth =
                (maxLineWidth > lineWidth) ? maxLineWidth : lineWidth;
            totalHeight += lineHeight;
        }
    }

    if (totalHeight == 0)
    {
        RECT emptyRect = {0, 0, 0, 0};
        return emptyRect;
    }

    int boxWidth  = maxLineWidth + (TAG_PADDING * 2);
    int boxHeight = totalHeight + (TAG_PADDING * 2);

    int left   = tagCenter.x - (boxWidth / 2);
    int top    = tagCenter.y - (boxHeight / 2);
    int right  = left + boxWidth;
    int bottom = top + boxHeight;

    RECT boxRect = {left, top, right, bottom};

    POINT lineStart, lineEnd;
    if (LiangBarskyClip(boxRect, aircraftScreenPos, tagCenter, lineStart,
                        lineEnd))
    {
        HPEN linePen =
            CreatePen(PS_SOLID, CONNECTION_LINE_WIDTH, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hDC, linePen);

        MoveToEx(hDC, aircraftScreenPos.x, aircraftScreenPos.y, nullptr);
        LineTo(hDC, lineStart.x, lineStart.y);

        SelectObject(hDC, oldPen);
        DeleteObject(linePen);
    }

    HBRUSH bgBrush  = CreateSolidBrush(backgroundColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, bgBrush);
    HPEN oldPen     = (HPEN)SelectObject(hDC, GetStockObject(NULL_PEN));

    Rectangle(hDC, left, top, right + 1, bottom + 1);

    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldBrush);
    DeleteObject(bgBrush);

    SetTextColor(hDC, textColor);
    SetBkMode(hDC, TRANSPARENT);

    // Draw each line of text
    int currentY = top + TAG_PADDING;

    for (size_t lineIdx = 0; lineIdx < tagLines.size(); ++lineIdx)
    {
        const auto & line = tagLines[lineIdx];
        bool hasContent   = false;
        int currentX      = left + TAG_PADDING;

        // Check if line has content
        for (const auto & itemName : line)
        {
            TagItemType itemType = ParseTagItemType(itemName);
            auto it              = tagData.items.find(itemType);
            if (it != tagData.items.end() && !it->second.empty())
            {
                hasContent = true;
                break;
            }
        }

        if (!hasContent) continue;

        for (size_t elemIdx = 0; elemIdx < line.size(); ++elemIdx)
        {
            const auto & itemName = line[elemIdx];
            TagItemType itemType  = ParseTagItemType(itemName);
            auto it               = tagData.items.find(itemType);
            std::string text = (it != tagData.items.end()) ? it->second : "";

            if (!text.empty())
            {
                TextOutA(hDC, currentX, currentY, text.c_str(),
                         static_cast<int>(text.length()));

                SIZE textSize;
                GetTextExtentPoint32A(hDC, text.c_str(),
                                      static_cast<int>(text.length()),
                                      &textSize);
                currentX += textSize.cx;

                bool hasNextContent = false;
                for (size_t k = elemIdx + 1; k < line.size(); ++k)
                {
                    TagItemType nextType = ParseTagItemType(line[k]);
                    auto nextIt          = tagData.items.find(nextType);
                    if (nextIt != tagData.items.end() &&
                        !nextIt->second.empty())
                    {
                        hasNextContent = true;
                        break;
                    }
                }
                if (hasNextContent) { currentX += spaceWidth; }
            }
        }

        currentY += lineHeight;
    }

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);

    return boxRect;
}

RECT Tag::DrawTag(HDC hDC,
                  POINT aircraftScreenPos,
                  const std::string & callsign,
                  int tagOffsetX,
                  int tagOffsetY)
{
    POINT tagCenter;
    tagCenter.x = aircraftScreenPos.x + tagOffsetX;
    tagCenter.y = aircraftScreenPos.y + tagOffsetY;

    HFONT hFont = CreateFontA(FONT_HEIGHT, 0, 0, 0, FONT_WEIGHT, FALSE, FALSE,
                              FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, FONT_NAME);

    HFONT oldFont = (HFONT)SelectObject(hDC, hFont);

    SIZE textSize;
    GetTextExtentPoint32A(hDC, callsign.c_str(),
                          static_cast<int>(callsign.length()), &textSize);

    int boxWidth  = textSize.cx + (TAG_PADDING * 2);
    int boxHeight = textSize.cy + (TAG_PADDING * 2);

    int left   = tagCenter.x - (boxWidth / 2);
    int top    = tagCenter.y - (boxHeight / 2);
    int right  = left + boxWidth;
    int bottom = top + boxHeight;

    RECT boxRect = {left, top, right, bottom};

    // Draw leader line using Liang-Barsky clipping
    POINT lineStart, lineEnd;
    if (LiangBarskyClip(boxRect, aircraftScreenPos, tagCenter, lineStart,
                        lineEnd))
    {
        HPEN linePen =
            CreatePen(PS_SOLID, CONNECTION_LINE_WIDTH, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hDC, linePen);

        MoveToEx(hDC, aircraftScreenPos.x, aircraftScreenPos.y, nullptr);
        LineTo(hDC, lineStart.x, lineStart.y);

        SelectObject(hDC, oldPen);
        DeleteObject(linePen);
    }

    // Draw tag background
    HBRUSH bgBrush  = CreateSolidBrush(DEFAULT_TAG_BG_COLOR);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, bgBrush);
    HPEN oldPen     = (HPEN)SelectObject(hDC, GetStockObject(NULL_PEN));

    Rectangle(hDC, left, top, right + 1, bottom + 1);

    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldBrush);
    DeleteObject(bgBrush);

    // Draw text in tag
    SetTextColor(hDC, DEFAULT_TAG_TEXT_COLOR);
    SetBkMode(hDC, TRANSPARENT);

    int textX = left + TAG_PADDING;
    int textY = top + TAG_PADDING;
    TextOutA(hDC, textX, textY, callsign.c_str(),
             static_cast<int>(callsign.length()));

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);

    return boxRect;
}

bool Tag::GetTextRect(HDC hDC, const std::string & text, RECT & outRect)
{
    HFONT hFont =
        CreateFontA(FONT_HEIGHT, 0, 0, 0, FONT_WEIGHT, FALSE, FALSE, FALSE,
                    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH, FONT_NAME);

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
