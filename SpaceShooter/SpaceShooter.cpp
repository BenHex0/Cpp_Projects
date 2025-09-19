#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "ConsoleGameEnigne/Window.h"
#include "ConsoleGameEnigne/InputHandler.h"
#include <chrono>

// Global Space
int g_globalWidth = 200;
int g_globalHeight = 30;

// Colors
#define FG_BLACK 0x0000
#define FG_BLUE 0x0001
#define FG_GREEN 0x0002
#define FG_RED 0x0004
#define FG_WHITE 0x0007
#define FG_YELLOW (FG_RED | FG_GREEN)
#define FG_CYAN (FG_BLUE | FG_GREEN)
#define FG_MAGENTA (FG_RED | FG_BLUE)
#define FG_INTENSITY 0x0008

struct Position
{
    int x;
    int y;
    bool isOutOfBounds()
    {
        return x < 0 || x >= g_globalWidth ||
               y < 0 || y >= g_globalHeight;
    }
};

bool isColliding(const Position &aPos, int aWidth, int aHeight,
                 const Position &bPos, int bWidth, int bHeight)
{
    return !(aPos.x + aWidth <= bPos.x ||  // A is left of B
             aPos.x >= bPos.x + bWidth ||  // A is right of B
             aPos.y + aHeight <= bPos.y || // A is above B
             aPos.y >= bPos.y + bHeight);  // A is below B
}

class Sprite : public Drawable
{
private:
    int m_width = 0;
    int m_height = 0;
    short *m_Glyphs = nullptr;
    short *m_Colours = nullptr;
    Position m_position;
    WORD m_color = FG_WHITE;

public:
    Sprite() {}
    Sprite(int w, int h) { Create(w, h); }
    Sprite(Position pos) : m_position(pos) {}
    ~Sprite()
    {
        delete[] m_Glyphs;
        delete[] m_Colours;
    }

    void Create(int w, int h)
    {
        m_width = w;
        m_height = h;
        delete[] m_Glyphs;
        delete[] m_Colours;
        m_Glyphs = new short[w * h];
        m_Colours = new short[w * h];
        for (int i = 0; i < w * h; i++)
        {
            m_Glyphs[i] = L' ';
            m_Colours[i] = FG_BLACK;
        }
    }

    void SetGlyph(int x, int y, short c)
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;
        m_Glyphs[y * m_width + x] = c;
    }

    void SetColour(WORD color)
    {
        m_color = color;
    }

    void setPosition(Position pos)
    {
        m_position.x = pos.x;
        m_position.y = pos.y;
    }

    short GetGlyph(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return L' ';
        return m_Glyphs[y * m_width + x];
    }

    int getWidth()
    {
        return m_width;
    }

    int getHeight()
    {
        return m_height;
    }

    // Load from plain text ASCII file
    bool LoadFromText(const std::string &sFile)
    {
        std::wifstream fin(sFile);
        if (!fin.is_open())
            return false;

        std::vector<std::wstring> lines;
        std::wstring line;
        size_t maxw = 0;

        while (std::getline(fin, line))
        {
            if (!line.empty() && line.back() == L'\r')
                line.pop_back(); // strip CR if CRLF
            lines.push_back(line);
            if (line.size() > maxw)
                maxw = line.size();
        }
        fin.close();

        if (lines.empty())
            return false;

        Create((int)maxw, (int)lines.size());

        for (int y = 0; y < (int)lines.size(); ++y)
        {
            const std::wstring &row = lines[y];
            for (int x = 0; x < (int)maxw; ++x)
            {
                wchar_t g = (x < (int)row.size()) ? row[x] : L' ';
                SetGlyph(x, y, g);
            }
        }

        return true;
    }

    virtual void draw(CHAR_INFO buffer[]) const override
    {
        for (int sy = 0; sy < m_height; sy++)
        {
            for (int sx = 0; sx < m_width; sx++)
            {
                int dy = m_position.y + sy;
                int dx = m_position.x + sx;

                short g = GetGlyph(sx, sy);
                if (g == L' ')
                    continue; // skip spaces

                buffer[dy * g_globalWidth + dx].Char.UnicodeChar = g;
                buffer[dy * g_globalWidth + dx].Attributes = m_color;
            }
        }
    }
};

struct Star
{
    Position position;
    int speed; // 1 = slow, 2 = fast
};

class Space
{
private:
    int m_width{};
    int m_height{};
    std::vector<Star> m_stars;
    int m_stars_number{};

    void createStars(std::vector<Star> &stars)
    {
        for (int i = 0; i < m_stars_number; i++)
        {
            stars.push_back({rand() % m_width, rand() % m_height, 1 + rand() % 1});
        }
    }

public:
    Space(int width, int height, int stars) : m_width(width), m_height(height), m_stars_number(stars)
    {
        m_stars.reserve(stars);
        createStars(m_stars);
    }

    void updateStars()
    {
        for (auto &s : m_stars)
        {
            s.position.x -= s.speed; // move left

            if (s.position.x < 0) // reset to right side
            {
                s.position.x = m_width - 1;
                s.position.y = rand() % m_height;
                s.speed = 1 + rand() % 3;
            }
        }
    }

    void drawStars(CHAR_INFO buffer[])
    {
        // draw stars
        for (auto &s : m_stars)
        {
            int idx = s.position.y * m_width + s.position.x;
            buffer[idx].Char.UnicodeChar = (s.speed == 1 ? L'.' : (s.speed == 2 ? L'+' : L'*'));
            buffer[idx].Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED; // white
        }
    }
};

class Bullet
{
private:
    Position m_position;

public:
    Sprite sprite;

    Bullet() : sprite(m_position) {}

    void setPosition(int x, int y)
    {
        m_position.x = x;
        m_position.y = y;
        sprite.setPosition(m_position);
    }

    Position getPosition() const
    {
        return m_position;
    }

    void move(int x, int y)
    {
        m_position.x += x;
        m_position.y += y;
        sprite.setPosition(m_position);
    }

    void update(int x)
    {
        move(x, 0);
    }
};

class Enemy
{
private:
    Position m_position;
    std::chrono::time_point<std::chrono::steady_clock> lastTimeShot;

public:
    Sprite sprite;
    std::vector<Bullet *> bullets;

    Enemy() : sprite(m_position)
    {
        lastTimeShot = std::chrono::steady_clock::now();
    }

    ~Enemy()
    {
        for (size_t i = 0; i < bullets.size(); i++)
        {
            delete bullets[i];
        }
        bullets.clear();
    }

    void setPosition(int x, int y)
    {
        m_position.x = x;
        m_position.y = y;
        sprite.setPosition(m_position);
    }

    Position getPosition()
    {
        return m_position;
    }

    std::vector<Bullet *> &getBullets()
    {
        return bullets;
    }

    void move(int x, int y)
    {
        m_position.x += x;
        m_position.y += y;
        sprite.setPosition(m_position);
    }

    bool outOfBound()
    {
        return m_position.isOutOfBounds();
    }

    void shoot()
    {
        Bullet *bullet = new Bullet;
        int centerX = m_position.x + sprite.getWidth() / 2;
        int centerY = m_position.y + sprite.getHeight() / 2;
        bullet->setPosition(centerX, centerY);
        bullet->sprite.SetColour(FG_RED);
        bullet->sprite.LoadFromText("Bullet.txt");
        bullets.push_back(bullet);
    }

    void update()
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastTimeShot;

        if (elapsed.count() >= 2.f)
        {
            lastTimeShot = now;
            shoot();
        }

        move(-1, 0);

        for (size_t i = 0; i < bullets.size(); i++)
        {
            bullets[i]->update(-2);
        }
    }
};

class Player
{
private:
    Position m_position;
    int m_health;

    bool checkLeft()
    {
        return m_position.x > 0;
    }
    bool checkRight()
    {
        return (m_position.x + sprite.getWidth()) < 120;
    }
    bool checkUp()
    {
        return m_position.y > 1;
    }
    bool checkDown()
    {
        return (m_position.y + sprite.getHeight()) < g_globalHeight;
    }

public:
    Sprite sprite;
    std::vector<Bullet *> bullets;

    Player() : sprite(m_position), m_health(5) {}

    ~Player()
    {
        for (size_t i = 0; i < bullets.size(); i++)
        {
            delete bullets[i];
        }
        bullets.clear();
    }

    void setPosition(int x, int y)
    {
        m_position.x = x;
        m_position.y = y;
        sprite.setPosition(m_position);
    }

    void setHealth(int health)
    {
        m_health = health;
    }

    Position getPosition()
    {
        return m_position;
    }

    int getHealth()
    {
        return m_health;
    }

    std::vector<Bullet *> &getBullets()
    {
        return bullets;
    }

    std::wstring currentHealth()
    {
        std::wstring health = L"Health: ";
        health += std::to_wstring(m_health);
        return health;
    }

    void takeDamage()
    {
        m_health--;
    }

    bool isDeath()
    {
        if (m_health <= 0)
        {
            m_health = 0;
            return true;
        }
        return false;
    }

    void move(int x, int y)
    {
        m_position.x += x;
        m_position.y += y;
        sprite.setPosition(m_position);
    }

    void shoot()
    {
        Bullet *bullet = new Bullet;
        int centerX = m_position.x + sprite.getWidth() / 2;
        int centerY = m_position.y + sprite.getHeight() / 2;
        bullet->setPosition(centerX, centerY);
        bullet->sprite.SetColour(FG_RED);
        bullet->sprite.LoadFromText("PBullet.txt");
        bullets.push_back(bullet);
    }

    void update(InputHandler &input)
    {
        if (input.isKeyDown('S') && checkDown())
            move(0, 1);
        if (input.isKeyDown('W') && checkUp())
            move(0, -1);
        if (input.isKeyDown('D') && checkRight())
            move(1, 0);
        if (input.isKeyDown('A') && checkLeft())
            move(-1, 0);
        if (input.isKeyPressed(VK_SPACE))
            shoot();

        for (size_t i = 0; i < bullets.size(); i++)
        {
            bullets[i]->update(5);
        }
    }
};

class GameManager
{
    Window window;
    Space space;
    CHAR_INFO *buffer;
    Player player;
    std::vector<Enemy *> enemies;
    InputHandler input;
    int score = 0;

    void start()
    {
        for (int i = 0; i < g_globalWidth * g_globalHeight; i++)
        {
            buffer[i].Char.UnicodeChar = L' ';
            buffer[i].Attributes = 0;
        }

        deleteEnemies();
        player.setHealth(5);
        player.setPosition(5, 5);
        score = 0;
    }

    void spawnEnemies()
    {
        while (enemies.size() < 10)
        {
            Enemy *e = new Enemy;
            e->sprite.LoadFromText("Enemy.txt");
            int x = 130 + rand() % (140 - 130 + 1);
            int y = 1 + rand() % (g_globalHeight - 5);
            e->setPosition(x, y);
            e->sprite.SetColour(FG_YELLOW);
            enemies.push_back(e);
        }
    }

    void deleteEnemies()
    {
        for (auto& enemy : enemies)
        {
            delete enemy;
        }
        enemies.clear();
    }

    void checkCollisions()
    {
        // Check enemy bounds
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();)
        {
            Enemy *e = *enemyIt;

            if (e->getPosition().isOutOfBounds())
            {
                // Destroy enemy
                delete e;
                enemyIt = enemies.erase(enemyIt);
            }
            else
            {
                ++enemyIt;
            }
        }

        // check bullet bounds
        for (auto bulletIt = player.getBullets().begin(); bulletIt != player.getBullets().end();)
        {
            Bullet *bullet = *bulletIt;

            if (bullet->getPosition().isOutOfBounds())
            {
                // Destroy bullet
                delete bullet;
                bulletIt = player.getBullets().erase(bulletIt);
            }
            else
            {
                ++bulletIt;
            }
        }

        // Player bullets vs enemies
        for (auto bulletIt = player.getBullets().begin(); bulletIt != player.getBullets().end();)
        {
            Bullet *bullet = *bulletIt;
            bool hit = false;

            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();)
            {
                Enemy *e = *enemyIt;

                if (isColliding(bullet->getPosition(), bullet->sprite.getWidth(), bullet->sprite.getHeight(),
                                e->getPosition(), e->sprite.getWidth(), e->sprite.getHeight())) // collision function
                {
                    // Destroy enemy and bullet
                    delete e;
                    enemyIt = enemies.erase(enemyIt);

                    delete bullet;
                    bulletIt = player.getBullets().erase(bulletIt);

                    score++;

                    hit = true;
                    break; // stop checking this bullet
                }
                else
                {
                    ++enemyIt;
                }
            }

            if (!hit)
                ++bulletIt;
        }

        // // Enemy bullets vs player
        for (auto &e : enemies)
        {
            for (auto bulletIt = e->getBullets().begin(); bulletIt != e->getBullets().end();)
            {
                Bullet *bullet = *bulletIt;
                if (isColliding(bullet->getPosition(), bullet->sprite.getWidth(), bullet->sprite.getHeight(),
                 player.getPosition(), player.sprite.getWidth(), player.sprite.getHeight()))
                {
                    // player hit
                    player.takeDamage();

                    delete bullet;
                    bulletIt = e->getBullets().erase(bulletIt);
                }
                else
                {
                    ++bulletIt;
                }
            }
        }

        // Player vs enemies
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();)
        {
            Enemy *e = *enemyIt;
            if (isColliding(player.getPosition(), player.sprite.getWidth(), player.sprite.getHeight(),
                            e->getPosition(), e->sprite.getWidth(), e->sprite.getHeight()))
            {
                player.takeDamage();
                delete e;
                enemyIt = enemies.erase(enemyIt);
            }
            else
            {
                enemyIt++;
            }
        }
    }

public:
    GameManager() : window(g_globalWidth, g_globalHeight, 16),
                    space(g_globalWidth, g_globalHeight, 50)
    {
        buffer = new CHAR_INFO[g_globalWidth * g_globalHeight];
        player.sprite.LoadFromText("Player.txt");
        player.sprite.SetColour(FG_CYAN);
    }

    ~GameManager()
    {
        delete[] buffer;
        deleteEnemies();
    }

    void update()
    {
        start();

        while (true)
        {
            std::wstring scoreText;
            scoreText += L"Score: ";
            scoreText += std::to_wstring(score);
            input.update();

            if (!player.isDeath())
            {
                space.updateStars();
                space.drawStars(buffer);
                player.update(input);

                for (auto &e : enemies)
                {
                    e->update();
                }

                spawnEnemies();

                checkCollisions();

                for (auto &bullet : player.getBullets())
                {
                    window.draw(bullet->sprite, buffer, g_globalWidth * g_globalHeight);
                }

                for (auto &e : enemies)
                {
                    window.draw(e->sprite, buffer, g_globalWidth * g_globalHeight);
                    for (auto &b : e->getBullets())
                    {
                        window.draw(b->sprite, buffer, g_globalWidth * g_globalHeight);
                    }
                }

                window.draw(player.sprite, buffer, g_globalWidth * g_globalHeight);
                window.draw(buffer, g_globalWidth * g_globalHeight);
                window.drawText(0, 0, player.currentHealth());
                window.drawText(105, 0, scoreText);
                window.render();
                Sleep(50);
            }
            else
            {
                std::wstring message = L"Press Space to Play Again";
                window.drawText(0, 0, player.currentHealth());
                window.drawText(105, 0, scoreText);
                window.drawText((120 - message.length()) / 2, 30 / 2,message , FG_RED);
                window.render();
                if (input.isKeyPressed(VK_SPACE))
                {
                    start();
                }
            }
        }
    }
};

int main()
{
    GameManager gameManager;
    gameManager.update();
}