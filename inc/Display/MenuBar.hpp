#pragma once

#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include <string>
#include <vector>

/**
 * @brief MenuBar class for handling the settings/menu bar at the top of the SMR
 * display
 *
 * The menu bar consists of two lines:
 * - Line 1: Main menu items
 * - Line 2: Information sections separated by vertical lines
 */
class MenuBar
{
    public:
    MenuBar();
    ~MenuBar();

    /**
     * @brief Draw the menu bar
     * @param hDC Device context for drawing
     * @param displayArea The area of the radar display
     */
    void Draw(HDC hDC, RECT displayArea);

    /**
     * @brief Handle mouse clicks on the menu bar
     * @param pt Point where the mouse was clicked
     * @param displayArea The area of the radar display
     * @return Index of the menu item clicked, or -1 if none
     */
    int OnClick(POINT pt, RECT displayArea);

    /**
     * @brief Set the content for the second line sections
     * @param sections Vector of strings to display in the second line
     */
    void SetSecondLineContent(const std::vector<std::string> & sections);

    /**
     * @brief Get the height of the menu bar in pixels
     * @return Height in pixels
     */
    int GetHeight() const;

    /**
     * @brief Check if a point is within the menu bar
     * @param pt Point to check
     * @param displayArea The area of the radar display
     * @return true if point is within menu bar
     */
    bool IsPointInMenuBar(POINT pt, RECT displayArea) const;

    private:
    // First line menu items
    std::vector<std::string> menuItems;

    // Second line content sections
    std::vector<std::string> secondLineSections;

    // Cached rectangles for each menu item (for click detection)
    std::vector<RECT> menuItemRects;

    // Colors
    COLORREF backgroundColor;
    COLORREF textColor;
    COLORREF borderColor;
    COLORREF separatorColor;

    // Dimensions
    static constexpr int LINE_HEIGHT     = 20;
    static constexpr int PADDING         = 5;
    static constexpr int MENU_BAR_HEIGHT = LINE_HEIGHT * 2 + PADDING * 3;

    /**
     * @brief Draw the first line with menu items
     */
    void DrawFirstLine(HDC hDC, RECT area);

    /**
     * @brief Draw the second line with information sections
     */
    void DrawSecondLine(HDC hDC, RECT area);

    /**
     * @brief Calculate rectangles for menu items
     */
    void CalculateMenuItemRects(RECT area);
};
