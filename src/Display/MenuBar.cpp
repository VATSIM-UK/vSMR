#include "Display/MenuBar.hpp"

MenuBar::MenuBar()
    : backgroundColor(RGB(127, 122, 122)), textColor(RGB(0, 0, 0)),
      borderColor(RGB(100, 100, 100)), separatorColor(RGB(150, 150, 150))
{
    // Match the legacy top toolbar menu order/text
    menuItems = {"Display", "Target", "Colours", "Alerts", "/"};

    // Legacy toolbar is single-line
    secondLineSections.clear();
}

MenuBar::~MenuBar()
{
}

void MenuBar::Draw(HDC hDC, RECT displayArea)
{
    // Legacy toolbar is a single line at the top
    RECT menuBarArea   = displayArea;
    menuBarArea.bottom = menuBarArea.top + LINE_HEIGHT;

    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(backgroundColor);
    FillRect(hDC, &menuBarArea, bgBrush);
    DeleteObject(bgBrush);

    // Draw the single toolbar line
    DrawFirstLine(hDC, menuBarArea);
}

void MenuBar::DrawFirstLine(HDC hDC, RECT area)
{
    // Set text properties
    SetTextColor(hDC, textColor);
    SetBkMode(hDC, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

    menuItemRects.clear();

    RECT textRect;
    textRect.top    = area.top + 4;
    textRect.bottom = textRect.top + LINE_HEIGHT;
    textRect.left   = area.left + 2;

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
        clickRect.right += 4;
        menuItemRects.push_back(clickRect);

        currentX = textRect.right + 10;
    }

    SelectObject(hDC, oldFont);
}

void MenuBar::DrawSecondLine(HDC hDC, RECT area)
{
    (void)hDC;
    (void)area;
}

int MenuBar::OnClick(POINT pt, RECT displayArea)
{
    if (!IsPointInMenuBar(pt, displayArea)) { return -1; }

    // Check if click is in first line (menu items)
    int firstLineBottom = displayArea.top + LINE_HEIGHT;
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
    return LINE_HEIGHT;
}

bool MenuBar::IsPointInMenuBar(POINT pt, RECT displayArea) const
{
    return pt.y >= displayArea.top && pt.y <= displayArea.top + LINE_HEIGHT;
}

void MenuBar::CalculateMenuItemRects(RECT area)
{
    // This is done during drawing
}
