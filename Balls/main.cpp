#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>

struct Ball
{
    sf::CircleShape shape;
    sf::Vector2f velocity;

    Ball(float radius, sf::Vector2f pos, sf::Vector2f vel, sf::Color color)
    {
        shape.setRadius(radius);
        shape.setPosition(pos);
        shape.setFillColor(color);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::Black);
        velocity = vel;
    }

    void update(const sf::Vector2u &windowSize)
    {
        sf::Vector2f pos = shape.getPosition();
        float diameter = shape.getRadius() * 2;

        // Bounce off edges
        if (pos.x <= 0 || pos.x + diameter >= windowSize.x)
            velocity.x = -velocity.x;
        if (pos.y <= 0 || pos.y + diameter >= windowSize.y)
            velocity.y = -velocity.y;

        shape.move(velocity);
    }
};

Ball createBall(float radius, sf::Vector2i pos, sf::Vector2f vel, sf::Color color)
{
    sf::Vector2f convert = {(int)pos.x, (int)pos.y};
    Ball ball(radius, convert, vel, color);
    return ball;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Multiple Circles");
    window.setFramerateLimit(180);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::vector<Ball> balls;

    sf::Font font("Roboto-Italic-VariableFont_wdth,wght.ttf");
    bool isCreateBall = false;

    while (window.isOpen())
    {
        sf::Text text(font, std::to_string(balls.size()));
        text.setPosition({0, 0});

        float radius = 20 + std::rand() % 30; // radius 20–50
        sf::Vector2f pos(std::rand() % 700, std::rand() % 500);
        sf::Vector2f vel((std::rand() % 5 + 1) * ((std::rand() % 2 == 0) ? 1 : -1),
                         (std::rand() % 5 + 1) * ((std::rand() % 2 == 0) ? 1 : -1));
        sf::Color color(rand() % 256, rand() % 256, rand() % 256);

        while (const std::optional event = window.pollEvent())
        {
            if (event->getIf<sf::Event::MouseButtonPressed>())
            {
                isCreateBall = true;
            }

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        if (isCreateBall)
        {
            Ball ball = createBall(radius, sf::Mouse::getPosition(window), vel, color);
            balls.push_back(ball);
            std::cout << "x: " << sf::Mouse::getPosition().x << " y: " << sf::Mouse::getPosition().y << '\n';
            isCreateBall = false;
        }

        window.clear(sf::Color::Black);

        for (auto &ball : balls)
        {
            ball.update(window.getSize());
            window.draw(ball.shape);
        }

        window.draw(text);
        window.display();
    }
}
