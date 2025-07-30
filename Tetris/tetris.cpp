#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <chrono>
#include <random>

const int nScreenWidth = 120;
const int nScreenHeight = 35;
const int SHAPE_H = 3;
const int SHAPE_W = 3;
const int GRID_WIDTH = 14;
const int GRID_HEIGHT = 14;

int randomShape()
{
    static std::random_device rd;                      // Non-deterministic seed source
    static std::mt19937 gen(rd());                     // Mersenne Twister RNG
    static std::uniform_int_distribution<> dist(1, 7); // Range [1, 7]
    return dist(gen);
}

WORD randomColor()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Color codes: 1–15 (avoid 0 = black)
    std::uniform_int_distribution<> dist(1, 6);

    int code = dist(gen);

    // Map to Windows console colors (foreground only)
    WORD color = 0;
    if (code & 1)
        color |= FOREGROUND_BLUE;
    if (code & 2)
        color |= FOREGROUND_GREEN;
    if (code & 4)
        color |= FOREGROUND_RED;
    // if (code & 8) color |= FOREGROUND_INTENSITY;

    return color;
}

class Shape
{
private:
    int shapeType;
    int x;
    int y;
    int height;
    int width;
    WORD color;

    int I_shape[SHAPE_W * SHAPE_H] = {
        1, 0, 0,
        1, 0, 0,
        1, 0, 0};
    int O_shape[SHAPE_W * SHAPE_H] = {
        0, 0, 0,
        1, 1, 0,
        1, 1, 0};
    int L_shape[SHAPE_W * SHAPE_H] = {
        1, 0, 0,
        1, 0, 0,
        1, 1, 0};
    int S_shape[SHAPE_W * SHAPE_H] = {
        0, 0, 0,
        0, 1, 1,
        1, 1, 0};
    int J_shape[SHAPE_W * SHAPE_H] = {
        0, 0, 1,
        0, 0, 1,
        0, 1, 1};
    int T_shape[SHAPE_W * SHAPE_H] = {
        0, 0, 0,
        0, 1, 0,
        1, 1, 1};
    int Z_shape[SHAPE_W * SHAPE_H] = {
        0, 0, 0,
        1, 1, 0,
        0, 1, 1};

    void setShape(int _shape[])
    {
        for (int i = 0; i < SHAPE_H * SHAPE_W; i++)
        {
            blocks[i] = _shape[i];
        }
    }

    void createShape(int shapeType)
    {
        switch (shapeType)
        {
        case 1:
            setShape(I_shape);
            break;
        case 2:
            setShape(O_shape);
            break;
        case 3:
            setShape(L_shape);
            break;
        case 4:
            setShape(S_shape);
            break;
        case 5:
            setShape(J_shape);
            break;
        case 6:
            setShape(T_shape);
            break;
        case 7:
            setShape(Z_shape);
            break;

        default:
            break;
        }
    }

public:
    int blocks[SHAPE_H * SHAPE_W] = {};

    Shape() = default;

    Shape(int _shapeType, int _haight, int _width) : shapeType(_shapeType), height(_haight), width(_width)
    {
        createShape(shapeType);
        x = 0;
        y = 0;
        color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    void setPosition(int _x, int _y)
    {
        x = _x;
        y = _y;
    }

    void setColor(WORD _color)
    {
        color = _color;
    }

    int getShapeType()
    {
        return shapeType;
    }

    int getX()
    {
        return x;
    }

    int getY()
    {
        return y;
    }

    int getHeight()
    {
        return height;
    }

    int getWidth()
    {
        return width;
    }

    WORD getColor()
    {
        return color;
    }

    void move(int _x, int _y)
    {
        x = x + _x;
        y = y + _y;
    }
};

class GridSystem
{
private:
    int height;
    int width;

    void emptyGrid()
    {
        for (int i = 0; i < height * width; i++)
        {
            grid[i] = 0;
            color_grid[i] = 0;
        }
    }

    void drawBounds()
    {
        // left bound
        for (int i = 0; i < GRID_HEIGHT; i++)
        {
            grid[i * GRID_WIDTH] = 1;
            color_grid[i * GRID_WIDTH] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        }
        // draw bottom bound
        for (int i = 0; i < GRID_WIDTH; i++)
        {
            grid[(GRID_HEIGHT - 1) * GRID_WIDTH + i] = 1;
            color_grid[(GRID_HEIGHT - 1) * GRID_WIDTH + i] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        }
        // draw righ bound
        for (int i = 0; i < GRID_HEIGHT; i++)
        {
            grid[i * GRID_WIDTH + (GRID_WIDTH - 1)] = 1;
            color_grid[i * GRID_WIDTH + (GRID_WIDTH - 1)] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        }
    }

public:
    int *grid;
    WORD *color_grid;

    GridSystem(int _girdHeight, int _gridWidth) : height(_girdHeight), width(_gridWidth)
    {
        grid = new int[height * width];
        color_grid = new WORD[height * width];
        emptyGrid();
        drawBounds();
    }

    ~GridSystem()
    {
        delete[] grid;
        delete[] color_grid;
    }

    int getHeight()
    {
        return height;
    }

    int getWidth()
    {
        return width;
    }

    void placeShape(Shape shape)
    {
        for (int r = 0; r < shape.getHeight(); ++r)
        {
            for (int c = 0; c < shape.getWidth(); ++c)
            {
                int grid_index = (shape.getY() + r) * width + (shape.getX() + c);

                if (shape.blocks[r * SHAPE_W + c] != 0)
                {
                    grid[grid_index] = shape.blocks[r * shape.getWidth() + c];
                    color_grid[grid_index] = shape.getColor();
                }
            }
        }
    }

    std::vector<int> collision(Shape shape)
    {
        // recognized by index
        // bottom = 0, right = 1, left = 2, rotate = 3
        std::vector<int> collision(4, 0);

        for (int i = 0; i < shape.getHeight(); i++)
        {
            for (int j = 0; j < shape.getWidth(); j++)
            {
                int gy = shape.getY() + i;
                int gx = shape.getX() + j;

                if (shape.blocks[i * shape.getWidth() + j] != 0)
                {
                    if ((gy + 1) >= height)
                    {
                        // nothing till now
                    }

                    if (grid[gy * width + (gx - 1)] != 0)
                    {
                        collision[2] = 1;
                    }

                    if (grid[gy * width + (gx + 1)] != 0)
                    {
                        collision[1] = 1;
                    }

                    if (grid[(gy + 1) * width + gx] != 0)
                    {
                        collision[0] = 1;
                    }
                }

                if (grid[gy * GRID_WIDTH + gx] != 0)
                {
                    collision[3] = 1;
                }
            }
        }

        return collision;
    }

    void clearLines()
    {
        for (int i = height - 2; i > 0; i--)
        {
            bool fullLine = true;
            for (int j = 1; j < width - 1; j++)
            {
                if (grid[i * width + j] != 2)
                {
                    fullLine = false;
                    break;
                }
            }

            if (fullLine)
            {
                for (int j = i; j > 0; j--)
                {
                    for (int k = 1; k < width - 1; k++)
                    {
                        grid[j * width + k] = grid[(j - 1) * width + k];
                        color_grid[j * width + k] = color_grid[(j - 1) * width + k];
                    }
                }

                // Clear top row after shift
                for (int k = 1; k < width - 1; k++)
                {
                    grid[0 * width + k] = 0;
                    color_grid[0 * width + k] = 0;
                }

                i++; // recheck the same row after shift
            }
        }
    }

    int markLines()
    {
        int anyLineCleared = 0;
        for (int i = height - 2; i >= 0; i--)
        {
            bool fullLine = true;
            for (int j = 1; j < width - 1; j++)
            {
                if (grid[i * width + j] != 1)
                {
                    fullLine = false;
                    break;
                }
            }

            if (fullLine)
            {
                for (int j = 1; j < width - 1; j++)
                {
                    grid[i * width + j] = 2;
                    color_grid[i * width + j] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
                }
                anyLineCleared++;
            }
        }
        return anyLineCleared;
    }
};

class Screen
{
private:
    int width;
    int height;
    wchar_t *screen_buffer;
    WORD *color_buffer;
    HANDLE hConsole;
    DWORD dwBytesWritten;

public:
    Screen(int screenWidth,int screenHeight ) : width(screenWidth), height(screenHeight)
    {
        screen_buffer = new wchar_t[height * width];
        color_buffer = new WORD[height * width];
        hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(hConsole);
        dwBytesWritten = 0;
        clearScreen();
    }

    ~Screen()
    {
        delete[] screen_buffer;
        delete[] color_buffer;
    }

    void clearScreen()
    {
        for (int i = 0; i < height * width; i++)
        {
            screen_buffer[i] = L' ';
            color_buffer[i] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }

    void draw(GridSystem &field, int target, wchar_t texture = L'*')
    {
        for (int i = 0; i < field.getHeight(); i++)
        {
            for (int j = 0; j < field.getWidth(); j++)
            {
                int grid_index = i * field.getWidth() + j;

                if (field.grid[grid_index] == target)
                {
                    int screen_index = i * width + j;
                    screen_buffer[screen_index] = texture;
                    color_buffer[screen_index] = field.color_grid[grid_index];
                }
            }
        }
    }

    void draw(Shape shape, wchar_t texture = L'*')
    {
        for (int i = 0; i < shape.getHeight(); i++)
        {
            for (int j = 0; j < shape.getWidth(); j++)
            {
                if (shape.blocks[i * shape.getWidth() + j] != 0)
                {
                    int gx = shape.getX() + j;
                    int gy = shape.getY() + i;
                    int index = gy * width + gx;
                    screen_buffer[index] = texture;
                    color_buffer[index] = shape.getColor();
                }
            }
        }
    }

    void print(std::string message, int x, int y)
    {
        y += GRID_HEIGHT;
        for (int i = 0; i < message.size(); i++)
        {
            screen_buffer[y * width + (x + i)] = message[i];
        }
    }

    void render()
    {
        WriteConsoleOutputCharacterW(hConsole, screen_buffer, height * width, {0, 0}, &dwBytesWritten);
        WriteConsoleOutputAttribute(hConsole, color_buffer, width * height, {0, 0}, &dwBytesWritten);
        clearScreen();
    }
};

class GameManger
{
private:
    Shape *shape;
    GridSystem *grid;
    Screen *screen;

    void control(Shape &shape, std::vector<int> collision)
    {
        if ((GetAsyncKeyState('A') & 0b1) && (collision[2] == 0))
        {
            shape.move(-1, 0);
        }
        else if ((GetAsyncKeyState('D') & 0b1) && (collision[1] == 0))
        {
            shape.move(1, 0);
        }
        else if ((GetAsyncKeyState('S') & 0b1))
        {
            shape.move(0, 1);
        }
        else if ((GetAsyncKeyState('W') & 0b1))
        {
            shape.move(0, -1);
        }
        else if (((GetAsyncKeyState)(VK_SPACE) & 0b1) && (collision[3] == 0) && (shape.getShapeType() != 2))
        {
            rotate90Right(shape);
        }
    }

    void rotate90Right(Shape &shape)
    {
        // Assuming square grid
        const int N = SHAPE_W;

        // Step 1: Transpose the matrix
        for (int i = 0; i < N; ++i)
            for (int j = i + 1; j < N; ++j)
                std::swap(shape.blocks[i * N + j], shape.blocks[j * N + i]);

        // Step 2: Reverse each row
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N / 2; ++j)
            {
                std::swap(shape.blocks[i * N + j], shape.blocks[i * N + (N - 1 - j)]);
            }
        }
    }

public:
    GameManger()
    {
        shape = new Shape(randomShape(), SHAPE_H, SHAPE_W);
        grid = new GridSystem(GRID_HEIGHT, GRID_WIDTH);
        screen = new Screen(nScreenWidth, nScreenHeight);
    }

    ~GameManger()
    {
        delete shape;
        delete grid;
        delete screen;
    }

    void run()
    {
        auto lastFallTime = std::chrono::high_resolution_clock::now();
        int x = GRID_WIDTH / 2, y = 0;
        shape->setPosition(x, y);
        shape->setColor(randomColor());
        int score = 0;
        bool gameOver = false;
        int fullLine = -1;

        while (true)
        {

            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = now - lastFallTime;

            screen->print("Score: ", 0, 0);
            screen->print(std::to_string(score), 7, 0);

            if (grid->collision(*shape)[0] != 0)
            {
                if (shape->getY() <= 0)
                {
                    gameOver = true;
                    screen->print("GAMEOVER", 0, 1);
                    screen->draw(*shape, L'\u2588');
                    screen->draw(*grid, 1, L'\u2588');
                    screen->render();
                }
                else
                {
                    grid->placeShape(*shape);
                    delete shape;
                    Shape *new_shape = new Shape(randomShape(), SHAPE_H, SHAPE_W);
                    shape = new_shape;
                    shape->setPosition(x, y);
                    shape->setColor(randomColor());
                }
            }

            if (!gameOver)
            {
                if (fullLine > 0)
                {
                    Sleep(150);
                    grid->clearLines();
                    score += fullLine;
                }
                fullLine = grid->markLines();

                // control shape
                control(*shape, grid->collision(*shape));

                if (elapsed.count() >= 0.5f)
                {
                    lastFallTime = now;
                    shape->move(0, 1);
                }

                screen->draw(*shape, L'#');
                screen->draw(*grid, 1, L'\u2588');
                screen->draw(*grid, 2, L'*');
                screen->render();
            }
        }
    }
};

int main()
{
    GameManger tetris;
    tetris.run();
    std::cin.get();
}