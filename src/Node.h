#pragma once

#include <SFML/Graphics.hpp>

class Node {

public:

	Node(int, int, float, float, int, int);
	~Node();
	
	friend bool operator <(const Node&, const Node&);

	int get_reverse_direction(int direction);
	void registerDirection(int direction, Node * n);
	void removeDirection(int direction);
	void blockDirection(int dir);

	int getRandomUnusedDir();

	void set_color(sf::Color color);
	sf::Color get_color();

	sf::Color default_color = sf::Color::Black;

	sf::Shape **shapes;
	int shape_count = 5;
	// 4 is the main shape
	// 0 to 3 are the borders (same identification as DIR)

	Node **directions;

	static constexpr int UP_DIR = 0;
	static constexpr int DOWN_DIR = 1;
	static constexpr int LEFT_DIR = 2;
	static constexpr int RIGHT_DIR = 3;

	static constexpr float BORDER_FACTOR = 15.75;

	int position_x;
	int position_y;

	int max_lin;
	int max_col;

	float rectangle_W;
	float rectangle_H;
	
};

