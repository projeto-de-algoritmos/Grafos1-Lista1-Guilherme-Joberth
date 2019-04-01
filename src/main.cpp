#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <X11/Xlib.h>

#include <cstdlib>
#include <iostream>
#include <ctime>
#include <queue>
#include <map>
#include <list>


#include "Tools.h"
#include "Node.h"


Node **nodes;

Node *color_node;
int current_tool = Tools::PENCIL;
std::queue<sf::Color> colors;

sf::Mutex node_mutex;
sf::Mutex bucket_mutex;
bool skip_render = false;

bool generation_running = false;
bool generated = false;

int lin = 30;// 55;
int col = 30;// 75;
int max_elements = lin * col;

float rectangle_W = 20.f;
float rectangle_H = 20.f;

int ms_wait = 0;
const int ms_step = 10;

struct BUCKET_ARGS
{
	Node **nodes;
	Node *n;
	sf::Color color;
};



Node *get_node(Node** nodes, int i, int j){


	if(i >= lin || j >= col){
		return NULL;
	}

	return nodes[i * col + j];
}


void generateNodes() {

	// accessing nodes in matrix order is Node[i * col + j]

	skip_render = true;

	node_mutex.lock(); //lock to initialize mutexes

	if (nodes != NULL) { 
	
		for (int i = 0; i < max_elements; i++) {
			delete nodes[i];
		}

		delete color_node;
		free(nodes);
	}

	nodes = (Node **)calloc(max_elements, sizeof(Node *));

	color_node = new Node(lin + 5, 1, rectangle_W, rectangle_H, 0, 0);
	color_node->set_color(sf::Color::Red);

	int k = 0;
	for (int i = 0; i < lin; i++) {
		for (int j = 0; j < col; j++) {

			Node *n = new Node(j, i, rectangle_W, rectangle_H, lin, col);

			nodes[k++] = n;
		}

	}

	node_mutex.unlock();

	skip_render = false;
}


void renderingThread(sf::RenderWindow* window) {

	unsigned seed = (unsigned)std::time(0);

	std::cout << "[RENDERING] Seed: " << seed << "\n";

	std::srand(seed);

	// activate the window's context
	window->setActive(true);
	
	generateNodes();

	// the rendering loop
	while (window->isOpen()) {

		window->clear();

		for (int i = 0; i < max_elements; i++) {
			
			if (skip_render) continue;

			for (int j = nodes[i]->shape_count - 1; j >= 0; j--) {

				if (skip_render) continue;
		
				window->draw(*nodes[i]->shapes[j]);
				
			}
		}

		if (skip_render == false) window->draw(*color_node->shapes[4]);

		// end the current frame -- this is a rendering function (it requires the context to be active)
		window->display();

	}

}

void breadth_first(Node *start, Node *end) {
	
	std::queue<Node *> node_queue;
	std::map<Node *, Node *> path_map; // <current node, parent node> 


	node_queue.push(start);

	while (node_queue.empty() == false)	{

		sf::sleep(sf::milliseconds(ms_wait));

		Node *current = node_queue.front();
		node_queue.pop();

		if (current == end) break;

		for (int i = 0; i < 4; i++) {
		
			Node *directed_node = current->directions[i];

			if (directed_node != NULL ){//&& directed_node->path_check == false) {
			
				node_queue.push(directed_node);

				// insert on map with current as parent, this is for the backtrace of the path
				path_map.insert({ directed_node, current });
			}
		}

	}


	// since 'end' is the same as the last 'current' that was inserted into the map,
	// we can use it to find the last element parent
	Node *current = end;
	while (current != start) {

		sf::sleep(sf::milliseconds(ms_wait));

		Node *parent = path_map[current];
		
		current = parent;

	}

}

void reset_canvas(){

	generateNodes();
}

void set_tool(int tool){

	current_tool = tool;
	std::cout << "[TOOL] Current tool: " << tool << "\n";

}

void initialize_colors(){

	colors.push(sf::Color::Red);
	colors.push(sf::Color::Green);
	colors.push(sf::Color::Blue);
	colors.push(sf::Color::Yellow);
	colors.push(sf::Color::White);

}

void change_color(){

	node_mutex.lock();
	skip_render = true;

	// the current color is the first on the queue
	
	// to change the color, we just send it to the end
	
	sf::Color old_color = colors.front();
	colors.pop();
	colors.push(old_color);

	sf::Color new_color = colors.front();

	color_node->set_color(new_color);

	node_mutex.unlock();
	skip_render = false;

	std::cout << "[COlOR] New color: " << new_color.toInteger() << "\n";
}

void do_pencil(Node** nodes, Node* n, sf::Color current_color){

	n->set_color(current_color);

    Node **neighbors = (Node**)calloc(4, sizeof(Node *));

    neighbors[0] = get_node(nodes, n->position_y - 1, n->position_x);
    neighbors[1] = get_node(nodes, n->position_y + 1, n->position_x);
    neighbors[2] = get_node(nodes, n->position_y, n->position_x - 1);
    neighbors[3] = get_node(nodes, n->position_y, n->position_x + 1);
    
    for (int i = 0; i < 4; i++){

		if (neighbors[i] == NULL) continue;

        if (neighbors[i]->get_color() == current_color){
            n->registerDirection(i, neighbors[i]);
        }else{
			n->removeDirection(i);
		}
    }



    free(neighbors);
}

void do_eraser(Node *n){

	n->set_color(sf::Color::Black);

    for (int i = 0; i < 4; i++){

        n->removeDirection(i);
    }
}

int convert_i_direction(int i, int direction){

	if(direction == Node::UP_DIR) return i - 1;
	if(direction == Node::DOWN_DIR) return i + 1;
	return i;

}


int convert_j_direction(int j, int direction){

	if(direction == Node::LEFT_DIR) return j - 1;
	if(direction == Node::RIGHT_DIR) return j + 1;
	return j;

}

void do_bucket(struct BUCKET_ARGS *args){


	std::cout << "[BUCKET] Started bucket thread \n";

	sf::Color new_color = args->color;
	Node **nodes = args->nodes;
	Node *n = args->n;

	bucket_mutex.lock();

	std::queue <Node *> nodesQueue;

	sf::Color old_color = n->get_color();

	if(n->get_color() != new_color){
		nodesQueue.push(n);
	} 

	while(nodesQueue.empty() == false){

		Node* current = nodesQueue.front();
		nodesQueue.pop();

		current->set_color(new_color);

		for (int dir = 0; dir < 4; dir++){

			int i = convert_i_direction(current->position_y, dir);
			int j = convert_j_direction(current->position_x, dir);

			Node* aux = get_node(nodes, i, j);

			if (aux == NULL) continue;

			if (aux->get_color() == old_color) {
				nodesQueue.push(aux);
				current->registerDirection(dir, aux);

			} else if (aux->get_color() == new_color && aux->directions[dir] != current){
				current->registerDirection(dir, aux);

			}
		}
	}

	bucket_mutex.unlock();

}

int main() {
	
	XInitThreads();
	
	unsigned seed = (unsigned)std::time(0);
	
	std::cout << "[MAIN] Seed: " << seed << "\n";
	
	std::srand(seed);

	// create the window (remember: it's safer to create it in the main thread due to OS limitations)
	sf::RenderWindow window(sf::VideoMode(800, 600), "Grafos - Pintura");

	// deactivate its OpenGL context
	window.setActive(false);

	// insert colors in color palete
	initialize_colors();


	// launch the rendering thread
	sf::Thread rend_thread(&renderingThread, &window);
	rend_thread.launch();

	// the event/logic/whatever loop
	while (window.isOpen()) {



		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) {

				rend_thread.terminate();
				
				window.close();
			}

			if (event.type == sf::Event::KeyReleased) {
				
				if (event.key.code == sf::Keyboard::R) {
					// reset canvas
					reset_canvas();
					
				} else if (event.key.code == sf::Keyboard::Num1) {
					// set tool as pencil
					set_tool(Tools::PENCIL);
				
				}else if (event.key.code == sf::Keyboard::Num2){
					// set tool as bucket
					set_tool(Tools::BUCKET);

				}else if (event.key.code == sf::Keyboard::Num3){
					// set tool as eraser
					set_tool(Tools::ERASER);

				
				}else if (event.key.code == sf::Keyboard::Space){
					// Change color
					change_color();
					
				} else if (event.key.code == sf::Keyboard::Q) {

					if (ms_wait - ms_step > 0) ms_wait -= ms_step; else ms_wait = 0;

					std::cout << "[MAIN] ms_wait = " << ms_wait << "\n";

				}
				else if (event.key.code == sf::Keyboard::W) {

					ms_wait += ms_step;

					std::cout << "[MAIN] ms_wait = " << ms_wait << "\n";
				}
			} 
			
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)){

				sf::Vector2i localPosition = sf::Mouse::getPosition(window);
				//std::cout << "Mouse at: " << localPosition.x << ", " << localPosition.y << "\n";;
				//find node correct node
				int node_j = localPosition.x / rectangle_W;
				int node_i = localPosition.y / rectangle_H;

				Node *n = get_node(nodes, node_i, node_j);
				
				if (n){

					if (current_tool == Tools::PENCIL){
							do_pencil(nodes, n, colors.front());
					}else if(current_tool == Tools::ERASER){
							do_eraser(n);
					}else if (current_tool == Tools::BUCKET){

						struct BUCKET_ARGS *bucket_args = (BUCKET_ARGS *)malloc(sizeof(BUCKET_ARGS));
						
						bucket_args->color = colors.front();
						bucket_args->nodes = nodes;
						bucket_args->n = n;

						std::cout << "[MAIN] Creating bucket thread \n";
						sf::Thread bucket_thread(&do_bucket, bucket_args);
						bucket_thread.launch();

					}
				}
			}

			
		}
	}

	free(nodes);

	return 0;
}
