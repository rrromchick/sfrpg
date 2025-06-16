#include <SFML/Graphics.hpp>
#include <iostream>

int main(int argc, char **argv) {
    sf::RenderWindow window(sf::VideoMode(640, 480), "SFML works!");
    sf::CircleShape circle(10);
    circle.setFillColor(sf::Color::Yellow);
   
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::MouseButtonPressed) {
                std::cout << "click: " << sf::Mouse::getPosition(window).x << ' ' 
                    << sf::Mouse::getPosition().y << std::endl;
            }
            window.clear(sf::Color::Green);
            circle.setPosition(sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2));
            window.draw(circle);
            window.display();
        }
    }

    return 0;
}