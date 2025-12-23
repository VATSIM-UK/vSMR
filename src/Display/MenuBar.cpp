#include "Display/MenuBar.hpp"

MenuBar::MenuBar()
    : backgroundColor(RGB(25, 25, 25)), textColor(RGB(255, 255, 255)),
      borderColor(RGB(100, 100, 100)), separatorColor(RGB(150, 150, 150))
{
    // Initialize first line menu items
    menuItems = {"Display", "Maps",  "Windows", "Colours",
                 "Target",  "Radar", "RIMCAS",  "AFDAS"};

    // Initialize second line with default content
    secondLineSections = {"Section 1", "Section 2", "Section 3"};
}

MenuBar::~MenuBar()
{
}

void MenuBar::Draw(HDC hDC, RECT displayArea)
{
    // Create menu bar area at the top
    RECT menuBarArea   = displayArea;
    menuBarArea.bottom = menuBarArea.top + MENU_BAR_HEIGHT;

    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(backgroundColor);
    FillRect(hDC, &menuBarArea, bgBrush);
    DeleteObject(bgBrush);

    // Draw the two lines
    DrawFirstLine(hDC, menuBarArea);
    DrawSecondLine(hDC, menuBarArea);
}

void MenuBar::DrawFirstLine(HDC hDC, RECT area)
{
    // Set text properties
    SetTextColor(hDC, textColor);
    SetBkMode(hDC, TRANSPARENT);

    // Create font
    HFONT font = CreateFontA(14,                       // Height
                             0,                        // Width
                             0,                        // Escapement
                             0,                        // Orientation
                             FW_NORMAL,                // Weight
                             FALSE,                    // Italic
                             FALSE,                    // Underline
                             FALSE,                    // StrikeOut
                             DEFAULT_CHARSET,          // CharSet
                             OUT_DEFAULT_PRECIS,       // OutputPrecision
                             CLIP_DEFAULT_PRECIS,      // ClipPrecision
                             DEFAULT_QUALITY,          // Quality
                             DEFAULT_PITCH | FF_SWISS, // PitchAndFamily
                             "Arial"                   // FaceName
    );

    HFONT oldFont = (HFONT)SelectObject(hDC, font);

    menuItemRects.clear();

    RECT textRect;
    textRect.top    = area.top + 2;
    textRect.bottom = textRect.top + LINE_HEIGHT;
    textRect.left   = area.left + 5;

    // Draw menu items left-aligned with spacing between them
    int currentX = textRect.left;

    for (size_t i = 0; i < menuItems.size(); i++)
    {
        SIZE textSize;
        GetTextExtentPoint32A(hDC, menuItems[i].c_str(), menuItems[i].length(),
                              &textSize);

        textRect.left  = currentX;
        textRect.right = currentX + textSize.cx;

        DrawTextA(hDC, menuItems[i].c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Store rectangle for click detection
        RECT clickRect = textRect;
        clickRect.right += 10; // Add small clickable area after text
        menuItemRects.push_back(clickRect);

        // Move to next position with spacing
        currentX = textRect.right + 20;
    }

    SelectObject(hDC, oldFont);
    DeleteObject(font);
}

void MenuBar::DrawSecondLine(HDC hDC, RECT area)
{
    // Set text properties
    SetTextColor(hDC, textColor);
    SetBkMode(hDC, TRANSPARENT);

    // Create font (slightly smaller)
    HFONT font = CreateFontA(12,                       // Height
                             0,                        // Width
                             0,                        // Escapement
                             0,                        // Orientation
                             FW_NORMAL,                // Weight
                             FALSE,                    // Italic
                             FALSE,                    // Underline
                             FALSE,                    // StrikeOut
                             DEFAULT_CHARSET,          // CharSet
                             OUT_DEFAULT_PRECIS,       // OutputPrecision
                             CLIP_DEFAULT_PRECIS,      // ClipPrecision
                             DEFAULT_QUALITY,          // Quality
                             DEFAULT_PITCH | FF_SWISS, // PitchAndFamily
                             "Arial"                   // FaceName
    );

    HFONT oldFont = (HFONT)SelectObject(hDC, font);

    // Calculate second line position
    RECT textRect;
    textRect.top    = area.top + LINE_HEIGHT + 4;
    textRect.bottom = textRect.top + LINE_HEIGHT;
    textRect.left   = area.left + 5;
    textRect.right  = area.right - 5;

    // Create pen for vertical separators
    HPEN separatorPen = CreatePen(PS_SOLID, 1, separatorColor);
    HPEN oldPen       = (HPEN)SelectObject(hDC, separatorPen);

    // Draw sections left-aligned with separators
    int currentX = textRect.left;

    for (size_t i = 0; i < secondLineSections.size(); i++)
    {
        // Draw double vertical separator before each section (except the first)
        if (i > 0)
        {
            // First line
            MoveToEx(hDC, currentX - 10, textRect.top, NULL);
            LineTo(hDC, currentX - 10, textRect.bottom);

            // Second line
            MoveToEx(hDC, currentX - 7, textRect.top, NULL);
            LineTo(hDC, currentX - 7, textRect.bottom);
        }

        RECT sectionRect  = textRect;
        sectionRect.left  = currentX;
        sectionRect.right = textRect.right;

        // Draw text
        DrawTextA(hDC, secondLineSections[i].c_str(), -1, &sectionRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Calculate text width for next position
        SIZE textSize;
        GetTextExtentPoint32A(hDC, secondLineSections[i].c_str(),
                              secondLineSections[i].length(), &textSize);
        currentX += textSize.cx + 25; // Add spacing for next section
    }

    SelectObject(hDC, oldPen);
    DeleteObject(separatorPen);

    SelectObject(hDC, oldFont);
    DeleteObject(font);
}

int MenuBar::OnClick(POINT pt, RECT displayArea)
{
    if (!IsPointInMenuBar(pt, displayArea)) { return -1; }

    // Check if click is in first line (menu items)
    int firstLineBottom = displayArea.top + LINE_HEIGHT + 2;
    if (pt.y >= displayArea.top && pt.y <= firstLineBottom)
    {
        // Check each menu item rectangle
        for (size_t i = 0; i < menuItemRects.size(); i++)
        {
            if (PtInRect(&menuItemRects[i], pt)) { return static_cast<int>(i); }
        }
    }

    return -1;
}

void MenuBar::SetSecondLineContent(const std::vector<std::string> & sections)
{
    secondLineSections = sections;
}

int MenuBar::GetHeight() const
{
    return MENU_BAR_HEIGHT;
}

bool MenuBar::IsPointInMenuBar(POINT pt, RECT displayArea) const
{
    return pt.y >= displayArea.top && pt.y <= displayArea.top + MENU_BAR_HEIGHT;
}

void MenuBar::CalculateMenuItemRects(RECT area)
{
    // This is done during drawing
}
