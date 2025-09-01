#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

struct position
{
    int x;
    int y;
};

struct Rect
{
    int _x, _y;
    int _width, _height;

    int left() const { return _x; }
    int right() const { return _x + _width; }
    int top() const { return _y; }
    int bottom() const { return _y + _height; }

    bool intersects(const Rect &other) const
    {
        return _x < other._x + other._width &&
               _x + _width > other._x &&
               _y < other._y + other._height &&
               _y + _height > other._y;
    }
};

class Window
{
private:
    int width;
    int height;
    int _pixelSize;
    CHAR_INFO *buffer;
    HANDLE hConsole;
    COORD bufferSize;
    COORD bufferCoord;
    SMALL_RECT writeRegion;
    CONSOLE_FONT_INFOEX cfi;

    int checkWidthBound(int x)
    {
        return (x >= width) ? (width - 1) : ((x < 0) ? 0 : x);
    }
    int checkHeightBound(int y)
    {
        return (y >= height) ? (height - 1) : ((y < 0) ? 0 : y);
    }

    void allocateBuffers()
    {
        buffer = new CHAR_INFO[width * height];
    }

    void freeBuffers()
    {
        delete[] buffer;
    }

public:
    Window(int windowWidth, int windowHeight, int pixelSize)
        : width(windowWidth), height(windowHeight), _pixelSize(pixelSize)
    {
        hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(hConsole);

        // Set font size
        cfi = {sizeof(CONSOLE_FONT_INFOEX)};
        GetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
        cfi.dwFontSize.X = pixelSize;
        cfi.dwFontSize.Y = pixelSize;
        SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);

        // Set buffer size parameters accordingly
        bufferSize = {(SHORT)width, (SHORT)height};
        bufferCoord = {0, 0};
        writeRegion = {0, 0, (SHORT)(width - 1), (SHORT)(height - 1)};

        allocateBuffers();
        clearScreen();
    }

    ~Window()
    {
        freeBuffers();
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }

    void updateSizeIfChanged()
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
            return;

        int newWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        if (newWidth != width || newHeight != height)
        {
            width = newWidth;
            height = newHeight;

            bufferSize = {(SHORT)width, (SHORT)height};
            writeRegion = {0, 0, (SHORT)(width - 1), (SHORT)(height - 1)};

            freeBuffers();
            allocateBuffers();
            clearScreen();
        }
    }

    void clearScreen()
    {
        for (int i = 0; i < height * width; i++)
        {
            buffer[i].Char.UnicodeChar = L' ';
            buffer[i].Attributes = 0;
        }
    }

    void render(bool autoClear = true)
    {
        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);

        if (autoClear)
            clearScreen();
    }

    void drawText(int x, int y, const std::wstring &text, WORD color = 7)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);

        for (int i = 0; i < text.size(); i++)
        {
            buffer[y * width + (x + i)].Char.UnicodeChar = text[i];
            buffer[y * width + (x + i)].Attributes = color;
        }
    }

    void drawChar(int x, int y, wchar_t ch, WORD color = 7)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);

        buffer[y * width + x].Char.UnicodeChar = ch;
        buffer[y * width + x].Attributes = color;
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
                    int index = (y + i) * width + (x + j);
                    buffer[index].Char.UnicodeChar = border;
                    buffer[index].Attributes = color;
                }
            }
        }
        else
        {
            for (int i = 0; i < w; ++i)
            {
                int upperBound = y * width + (x + i);
                buffer[upperBound].Char.UnicodeChar = border;
                buffer[upperBound].Attributes = color;

                int lowerBound = (y + h - 1) * width + (x + i);
                buffer[lowerBound].Char.UnicodeChar = border;
                buffer[lowerBound].Attributes = color;
            }

            for (int i = 0; i < h; ++i)
            {
                int leftBound = (y + i) * width + x;
                buffer[leftBound].Char.UnicodeChar = border;
                buffer[leftBound].Attributes = color;

                int rightBound = (y + i) * width + (x + w - 1);
                buffer[rightBound].Char.UnicodeChar = border;
                buffer[rightBound].Attributes = color;
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
                    if (px >= 0 && px < width && py >= 0 && py < height)
                    {
                        buffer[py * width + px].Char.UnicodeChar = ch;
                        buffer[py * width + px].Attributes = color;
                    }
                }
            }
        }
    }

    void drawObject(int arr[], int w, int h, int x, int y, wchar_t ch = L'*', WORD color = 7)
    {
        x = checkWidthBound(x);
        y = checkHeightBound(y);
        w = checkWidthBound(w);
        h = checkHeightBound(h);

        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                int index = (y + i) * width + (x + j);
                if (arr[i * w + j] != 0)
                {
                    buffer[index].Char.UnicodeChar = ch;
                    buffer[index].Attributes = color;
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
};

class Paddle
{
private:
    position pos;
    int _width;
    int _height;
    int *_shape;

    void createPaddle()
    {
        for (int i = 0; i < _width * _height; i++)
        {
            _shape[i] = 1;
        }
    }

public:
    Paddle(int width, int height) : _width(width), _height(height)
    {
        _shape = new int[width * height];

        std::fill(_shape, _shape + (width * height), 0);
        createPaddle();
    }

    ~Paddle()
    {
        delete[] _shape;
    }

    void setPosition(int x, int y)
    {
        pos.x = x;
        pos.y = y;
    }

    int getX() const { return pos.x; }
    int getY() const { return pos.y; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }

    position getPosition() const { return pos; }

    int *getShape() { return _shape; }

    Rect getRect() const
    {
        return {pos.x, pos.y, _width, _height};
    }

    void move(int dx, int dy)
    {
        pos.x += dx;
        pos.y += dy;
    }
};

class Ball
{
private:
    position pos;

public:
    Ball() = default;

    wchar_t shape = L'O';

    void setPosition(int x, int y)
    {
        pos.x = x;
        pos.y = y;
    }

    int getX()
    {
        return pos.x;
    }

    int getY()
    {
        return pos.y;
    }

    position getPosition()
    {
        return pos;
    }

    Rect getRect() const
    {
        return {pos.x, pos.y, 1, 1};
    }

    void move(int x, int y)
    {
        pos.x += x;
        pos.y += y;
    }
};

class Brick
{
private:
    position pos;
    int width, height;
    bool destroyed = false;

public:
    Brick(int x, int y, int w = 1, int h = 1)
        : width(w), height(h)
    {
        pos.x = x;
        pos.y = y;
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }

    Rect getRect() const
    {
        return {pos.x, pos.y, width, height};
    }

    void draw(Window &win, wchar_t ch = L'.', WORD color = 7)
    {
        if (!destroyed)
            win.drawBox(pos.x, pos.y, width, height, ch, color, true);
    }

    void destroy()
    {
        destroyed = true;
    }

    bool isDestroyed() const
    {
        return destroyed;
    }
};

class InputHandler
{
private:
    std::unordered_map<int, bool> currentKeys;
    std::unordered_map<int, bool> previousKeys;

public:
    void update()
    {
        previousKeys = currentKeys; // Save previous state

        // List of keys you want to monitor
        int keys[] = {VK_LEFT, VK_RIGHT, VK_SPACE, 'A', 'D', VK_ESCAPE};

        for (int key : keys)
        {
            currentKeys[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        }
    }

    bool isKeyDown(int key) const
    {
        auto it = currentKeys.find(key);
        return it != currentKeys.end() ? it->second : false;
    }

    bool isKeyPressed(int key) const
    {
        return isKeyDown(key) && !wasKeyDown(key);
    }

    bool wasKeyDown(int key) const
    {
        auto it = previousKeys.find(key);
        return it != previousKeys.end() ? it->second : false;
    }

    bool isKeyReleased(int key) const
    {
        return !isKeyDown(key) && wasKeyDown(key);
    }
};

bool checkXBound(Window &window, int bound)
{
    if (bound >= window.getWidth() || bound <= 0)
        return true;
    return false;
}

bool checkYBound(Window &window, int bound)
{
    if (bound >= window.getHeight() || bound <= 0)
        return true;
    return false;
}

class GameManager
{
private:
    Window *_window;
    Paddle *_paddle;
    Ball ball;
    std::vector<Brick> bricks;
    InputHandler input;
    int ball_x_dir, ball_y_dir;
    int paddle_x_dir;
    int speed;
    bool ballDestroy;
    int lives;

    void createBlockOfBricks()
    {
        bricks.erase(bricks.begin(), bricks.end());

        // Create a grid of bricks
        int brickRows = 13;
        int brickCols = 50;
        int brickW = 1;
        int brickH = 1;
        for (int row = 0; row < brickRows; row++)
        {
            for (int col = 0; col < brickCols; col++)
            {
                int x = col * (brickW) + (_window->getWidth() - brickCols) / 2;
                int y = row * (brickH) + 2;
                bricks.emplace_back(x, y, brickW, brickH);
            }
        }
    }
    void createCircleOfBricks()
    {
        int centerX = _window->getWidth() / 2;
        int centerY = 5;
        int radius = 5;      // distance from center
        int brickCount = 60; // number of bricks in the circle
        int brickW = 1;
        int brickH = 1;

        for (int i = 0; i < brickCount; ++i)
        {
            float angle = (2.0f * 3.14159f * i) / brickCount; // convert index to angle in radians

            int x = static_cast<int>(centerX + radius * std::cos(angle));
            int y = static_cast<int>(centerY + radius * sin(angle));

            bricks.emplace_back(x, y, brickW, brickH);
        }
    }

    void start()
    {
        ball_x_dir = 1, ball_y_dir = 1;
        paddle_x_dir = 1;
        speed = 3;
        lives = 3;
        _paddle->setPosition(_window->getWidth() / 2, _window->getHeight() - 1);
        ball.setPosition((_window->getWidth() / 2) - 5, _window->getHeight() / 2);
    }

public:
    GameManager(Window *window, Paddle *paddle)
    {
        _window = window;
        _paddle = paddle;
        createBlockOfBricks();
    }

    ~GameManager()
    {
        delete _window;
        delete _paddle;
    }

    void gameLoop()
    {
        start();
        while (true)
        {
            int timer = 50;
            ballDestroy = false;
            input.update();

            bool collidedLeft = checkXBound(*_window, _paddle->getRect().left());
            bool collidedRight = checkXBound(*_window, _paddle->getRect().right());

            ////////////// control
            if (lives > 0)
            {
                if (input.isKeyDown('D') && !collidedRight)
                {
                    paddle_x_dir = 1;
                    _paddle->move(paddle_x_dir * speed, 0);
                }
                else if (input.isKeyDown('A') && !collidedLeft)
                {
                    paddle_x_dir = -1;
                    _paddle->move(paddle_x_dir * speed, 0);
                }
            }
            ///////////////////////////////////////////////////////

            ////////////// collision detection
            if (checkXBound(*_window, ball.getRect().left()) || checkXBound(*_window, ball.getRect().left()))
            {
                ball_x_dir *= -1;
            }

            Rect nextBallRect = ball.getRect();
            nextBallRect._x += ball_x_dir;
            nextBallRect._y += ball_y_dir;

            if (checkYBound(*_window, ball.getRect().top()))
            {
                ball_y_dir *= -1;
            }

            if (nextBallRect.intersects(_paddle->getRect()))
            {
                ball_y_dir *= -1;

                // Determine reflection direction
                int ballCenterX = ball.getX();
                int paddleCenter = _paddle->getX() + _paddle->getWidth() / 2;
                int diff = ballCenterX - paddleCenter;

                if (diff < 0)
                    ball_x_dir = -1; // Bounce left
                else if (diff > 0)
                    ball_x_dir = 1; // Bounce right
                else
                    ball_x_dir = 0; // Go straight up
            }
            else if (checkYBound(*_window, ball.getRect().bottom()))
            {
                ballDestroy = true;
                lives--;
            }

            // brick collision
            for (auto &brick : bricks)
            {
                if (!brick.isDestroyed() && ball.getRect().intersects(brick.getRect()))
                {
                    brick.destroy();
                    ball_y_dir *= -1;
                    break;
                }
            }
            ///////////////////////////////////////////////////////

            ////////////// game logic
            if (lives <= 0)
            {
                _window->drawChar(ball.getX(), ball.getY(), L'X');
                std::wstring text = L"Game Over Press Space to play again";
                _window->drawText((_window->getWidth() - text.length()) / 2, _window->getHeight() / 2, text, 4);
                if (input.isKeyPressed(VK_SPACE))
                {
                    start();
                    createBlockOfBricks();
                }
            }
            else if (bricks.empty())
            {
                std::wstring text = L"You won Press Space to play again";
                _window->drawText((_window->getWidth() - text.length()) / 2, _window->getHeight() / 2, text, 2);
                if (input.isKeyPressed(VK_SPACE))
                {
                    start();
                    createBlockOfBricks();
                }
            }
            else if (!ballDestroy)
            {
                _window->drawChar(ball.getX(), ball.getY(), L'O');
                ball.move(ball_x_dir, ball_y_dir);
            }
            ///////////////////////////////////////////////////////

            ////////////// rendering
            std::wstring lives_bar = L"Lives: ";
            lives_bar += std::to_wstring(lives);
            _window->drawText(0, 0, lives_bar);
            for (auto &brick : bricks)
            {
                brick.draw(*_window);
            }

            if (ballDestroy)
            {
                _window->drawChar(ball.getX(), ball.getY(), L'X');
                ball.setPosition((_window->getWidth() / 2) - 5, _window->getHeight() / 2);
                timer = 500;
            }
            
            _window->drawObject(_paddle->getShape(), _paddle->getWidth(), _paddle->getHeight(), _paddle->getX(), _paddle->getY(), L'=');
            _window->render();
            _window->updateSizeIfChanged();
            ///////////////////////////////////////////////////////
            Sleep(timer);
        }
    }
};

int main()
{
    Window window(120, 30, 16);
    Paddle paddle(10, 1);
    GameManager arkanoid(&window, &paddle);
    arkanoid.gameLoop();
    return 0;
}