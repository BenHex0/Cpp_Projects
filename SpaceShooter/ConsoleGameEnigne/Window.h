#pragma once

// Console Rendering Engine (Genericized)
#include <windows.h>


class Drawable
{
public:
    virtual void draw(CHAR_INFO buffer[]) const = 0;
    virtual ~Drawable() = default;
};

class Window
{
private:
    int m_width;
    int m_height;
    int m_pixelSize;
    CHAR_INFO *m_buffer;
    HANDLE m_hConsole;
    COORD m_bufferSize;
    COORD m_bufferCoord;
    SMALL_RECT m_writeRegion;
    CONSOLE_FONT_INFOEX m_cfi;

    int checkWidthBound(int x)
    {
        return (x >= m_width) ? (m_width - 1) : ((x < 0) ? 0 : x);
    }
    int checkHeightBound(int y)
    {
        return (y >= m_height) ? (m_height - 1) : ((y < 0) ? 0 : y);
    }

    void allocateBuffers()
    {
        m_buffer = new CHAR_INFO[m_width * m_height];
    }

    void freeBuffers()
    {
        delete[] m_buffer;
    }

public:
    Window(int windowWidth, int windowHeight, int pixelSize)
        : m_width(windowWidth), m_height(windowHeight), m_pixelSize(pixelSize)
    {
        m_hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(m_hConsole);

        // Set font size
        m_cfi = {sizeof(CONSOLE_FONT_INFOEX)};
        GetCurrentConsoleFontEx(m_hConsole, FALSE, &m_cfi);
        m_cfi.dwFontSize.X = pixelSize;
        m_cfi.dwFontSize.Y = pixelSize;
        SetCurrentConsoleFontEx(m_hConsole, FALSE, &m_cfi);

        // Set buffer size parameters accordingly
        m_bufferSize = {(SHORT)m_width, (SHORT)m_height};
        m_bufferCoord = {0, 0};
        m_writeRegion = {0, 0, (SHORT)(m_width - 1), (SHORT)(m_height - 1)};

        allocateBuffers();
        clearScreen();
    }

    ~Window()
    {
        freeBuffers();
    }

    void updateSizeIfChanged()
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(m_hConsole, &csbi))
            return;

        int newWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        if (newWidth != m_width || newHeight != m_height)
        {
            m_width = newWidth;
            m_height = newHeight;

            m_bufferSize = {(SHORT)m_width, (SHORT)m_height};
            m_writeRegion = {0, 0, (SHORT)(m_width - 1), (SHORT)(m_height - 1)};

            freeBuffers();
            allocateBuffers();
            clearScreen();
        }
    }

    void clearScreen()
    {
        for (int i = 0; i < m_height * m_width; i++)
        {
            m_buffer[i].Char.UnicodeChar = L' ';
            m_buffer[i].Attributes = 0;
        }
    }

    void render(bool autoClear = true)
    {
        WriteConsoleOutputW(m_hConsole, m_buffer, m_bufferSize, m_bufferCoord, &m_writeRegion);

        if (autoClear)
            clearScreen();
    }

    void drawText(int x, int y, const std::wstring &text, WORD color = 7)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);

        for (int i = 0; i < text.size(); i++)
        {
            m_buffer[y * m_width + (x + i)].Char.UnicodeChar = text[i];
            m_buffer[y * m_width + (x + i)].Attributes = color;
        }
    }

    void drawChar(int x, int y, wchar_t ch, WORD color = 7)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);

        m_buffer[y * m_width + x].Char.UnicodeChar = ch;
        m_buffer[y * m_width + x].Attributes = color;
    }

    void drawBox(int x, int y, int w, int h, wchar_t border = L'#', WORD color = 7, bool fullBox = true)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);
        w = checkWidthBound(w);
        h = checkHeightBound(h);

        if (fullBox)
        {
            for (int i = 0; i < h; i++)
            {
                for (int j = 0; j < w; j++)
                {
                    int index = (y + i) * m_width + (x + j);
                    m_buffer[index].Char.UnicodeChar = border;
                    m_buffer[index].Attributes = color;
                }
            }
        }
        else
        {
            for (int i = 0; i < w; ++i)
            {
                int upperBound = y * m_width + (x + i);
                m_buffer[upperBound].Char.UnicodeChar = border;
                m_buffer[upperBound].Attributes = color;

                int lowerBound = (y + h - 1) * m_width + (x + i);
                m_buffer[lowerBound].Char.UnicodeChar = border;
                m_buffer[lowerBound].Attributes = color;
            }

            for (int i = 0; i < h; ++i)
            {
                int leftBound = (y + i) * m_width + x;
                m_buffer[leftBound].Char.UnicodeChar = border;
                m_buffer[leftBound].Attributes = color;

                int rightBound = (y + i) * m_width + (x + w - 1);
                m_buffer[rightBound].Char.UnicodeChar = border;
                m_buffer[rightBound].Attributes = color;
            }
        }
    }

    void drawCircle(int cx, int cy, int r, wchar_t ch = L'*', WORD color = 7)
    {
        cx = checkWidthBound(cx);
        cy = checkHeightBound(cy);

        for (int y = -r; y <= r; y++)
        {
            for (int x = -r; x <= r; x++)
            {
                // Apply vertical compression factor for better aspect ratio
                if (x * x + (2 * y) * (2 * y) <= r * r)
                {
                    int px = cx + x;
                    int py = cy + y;
                    if (px >= 0 && px < m_width && py >= 0 && py < m_height)
                    {
                        m_buffer[py * m_width + px].Char.UnicodeChar = ch;
                        m_buffer[py * m_width + px].Attributes = color;
                    }
                }
            }
        }
    }

    void drawLine(int x0, int y0, int x1, int y1, wchar_t ch = L'*', WORD color = 7)
    {
        int dx = abs(x1 - x0);
        int dy = -abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true)
        {
            drawChar(x0, y0, ch, color); // Draw current point

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    void draw(const Drawable &drawable, CHAR_INFO buffer[], int bufferSize)
    {
        drawable.draw(buffer);

        for (int i = 0; i < bufferSize; i++)
        {
            m_buffer[i].Char.UnicodeChar = buffer[i].Char.UnicodeChar;
            m_buffer[i].Attributes = buffer[i].Attributes;
        }
    }

    void draw(CHAR_INFO buffer[], int bufferSize)
    {
        for (int i = 0; i < bufferSize; i++)
        {
            m_buffer[i].Char.UnicodeChar = buffer[i].Char.UnicodeChar;
            m_buffer[i].Attributes = buffer[i].Attributes;
        }

        for (int i = 0; i < bufferSize; i++)
        {
            buffer[i].Char.UnicodeChar = L' ';
            buffer[i].Attributes = 0;
        }
    }
};