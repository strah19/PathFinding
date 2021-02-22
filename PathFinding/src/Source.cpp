#include "StartUp/Application.h"
#include "Assets/Font.h"
#include "Assets/Texture.h"

#include <list>

class Sandbox : public Ember::Application {
public:
	void OnClosure() { 
		delete[] nodes;
	}

	void OnCreate() {
		nodes = new Node[256];
		for (int i = 0; i < grid_size.x; i++) {
			for (int j = 0; j < grid_size.y; j++) {
				nodes[j * grid_size.x + i].position.x = i;
				nodes[j * grid_size.x + i].position.y = j;
				nodes[j * grid_size.x + i].parent = nullptr;
				nodes[j * grid_size.x + i].is_obstacle = false;
				nodes[j * grid_size.x + i].visited = false;
			}
		}

		for (int i = 0; i < grid_size.x; i++) {
			for (int j = 0; j < grid_size.y; j++) {
				if (j > 0)
					nodes[j * grid_size.x + i].neighbors.push_back(&nodes[(j - 1) * grid_size.x + (i + 0)]);
				if (j < grid_size.y - 1)
					nodes[j * grid_size.x + i].neighbors.push_back(&nodes[(j + 1) * grid_size.x + (i + 0)]);

				if (i > 0)
					nodes[j * grid_size.x + i].neighbors.push_back(&nodes[(j + 0) * grid_size.x + (i - 1)]);
				if (i < grid_size.x - 1)
					nodes[j * grid_size.x + i].neighbors.push_back(&nodes[(j + 0) * grid_size.x + (i + 1)]);

			}
		}

		start_node = &nodes[(grid_size.y / 2) * grid_size.x + 1];
		end_node = &nodes[(grid_size.y / 2) * grid_size.x + grid_size.x - 2];
	}
	 
	void OnUserUpdate() {
		window->Update();
		
		renderer->Clear(background_color);

		int node_size = 40;
		int node_border = 10;

		for (int i = 0; i < grid_size.x; i++) {
			for (int j = 0; j < grid_size.y; j++) {
				for (auto& n : nodes[j * grid_size.x + i].neighbors) {
					renderer->Line({ (i * node_size) + node_size / 2, (j * node_size) + node_size / 2 },
						{ (n->position.x * node_size) + node_size / 2, (n->position.y * node_size) + node_size / 2 }, { 0, 0, 255, 255 });
				}
			}
		}

		for (int i = 0; i < grid_size.x; i++) {
			for (int j = 0; j < grid_size.y; j++) {		
				renderer->Rectangle(Ember::Rect({ i * node_size + node_border, j * node_size + node_border, 20, 20 }),
					nodes[j * grid_size.x + i].is_obstacle ? Ember::Color({ 255, 255, 255, 255 }) : Ember::Color({ 0, 0, 255, 255 })); 
				if (nodes[j * grid_size.x + i].visited) 
					renderer->Rectangle(Ember::Rect({ i * node_size + node_border, j * node_size + node_border, 20, 20 }), { 0, 0, 150, 255 });
				if (&nodes[j * grid_size.x + i] == start_node) 
					renderer->Rectangle(Ember::Rect({ i * node_size + node_border, j * node_size + node_border, 20, 20 }), { 0, 255, 0, 255 });
				else if (&nodes[j * grid_size.x + i] == end_node) 
					renderer->Rectangle(Ember::Rect({ i * node_size + node_border, j * node_size + node_border, 20, 20 }), { 255, 0, 0, 255 });
			}
		}

		if (end_node != nullptr) {
			Node* p = end_node;
			while (p->parent != nullptr) {
				renderer->Line({ (p->position.x * node_size) + node_size / 2, (p->position.y * node_size) + node_size / 2 },
					{ (p->parent->position.x * node_size) + node_size / 2, (p->parent->position.y * node_size) + node_size / 2 }, { 255, 255, 0, 255 });

				p = p->parent;
			}
		}

		renderer->Show();
	}

	void SolveAStar() {
		for (int i = 0; i < grid_size.x; i++) {
			for (int j = 0; j < grid_size.y; j++) {
				nodes[j * grid_size.x + i].parent = nullptr;
				nodes[j * grid_size.x + i].visited = false;
				nodes[j * grid_size.x + i].global = INFINITY;
				nodes[j * grid_size.x + i].local = INFINITY;
			}
		}

		auto distance = [](Node* a, Node* b) {
			return sqrtf((a->position.x - b->position.x) * (a->position.x - b->position.x) + (a->position.y - b->position.y) * (a->position.y - b->position.y));
		};

		auto heuristic = [distance](Node* a, Node* b) {
			return distance(a, b);
		};

		Node* current = start_node;
		start_node->local = 0.0f;
		start_node->global = heuristic(start_node, end_node);

		std::list<Node*> list_of_not_tested;
		list_of_not_tested.push_back(start_node);

		while (!list_of_not_tested.empty()) {
			list_of_not_tested.sort([](const Node* lhs, const Node* rhs) { return lhs->global < rhs->global; });

			while (!list_of_not_tested.empty() && list_of_not_tested.front()->visited)
				list_of_not_tested.pop_front();

			if (list_of_not_tested.empty())
				break;

			current = list_of_not_tested.front();
			current->visited = true;

			for (auto n : current->neighbors) {
				if (!n->visited && n->is_obstacle == false)
					list_of_not_tested.push_back(n);
				float lowest = current->local + distance(current, n);

				if (lowest < n->local) {
					n->parent = current;
					n->local = lowest;
					n->global = n->local + heuristic(n, end_node);
				}
			}

		}
	}

	bool MouseDown(Ember::MouseButtonEvents& mouse) {
		int node_size = 40;

		int node_select_x = events->MousePosition().x / node_size;
		int node_select_y = events->MousePosition().y / node_size;

		if (mouse.down && mouse.button_id == Ember::ButtonIds::LeftMouseButton) {
			if (node_select_x >= 0 && node_select_x < grid_size.x) {
				if (node_select_y >= 0 && node_select_y < grid_size.y) {
						nodes[node_select_y * grid_size.x + node_select_x].is_obstacle = !nodes[node_select_y * grid_size.x + node_select_x].is_obstacle;
						SolveAStar();
				}
			}
		}

		if (mouse.down && mouse.button_id == Ember::ButtonIds::RightMouseButton) {
			if (node_select_x >= 0 && node_select_x < grid_size.x) {
				if (node_select_y >= 0 && node_select_y < grid_size.y) {
					start_node = &nodes[node_select_y * grid_size.x + node_select_x];
					SolveAStar();
				}
			}
		}

		if (mouse.down && mouse.button_id == Ember::ButtonIds::MiddleMouseButton) {
			if (node_select_x >= 0 && node_select_x < grid_size.x) {
				if (node_select_y >= 0 && node_select_y < grid_size.y) {
					end_node = &nodes[node_select_y * grid_size.x + node_select_x];
					SolveAStar();
				}
			}
		}

		return false;
	}

	void UserDefEvent(Ember::Event& event) {
		Ember::EventDispatcher dispath(&event);

		dispath.Dispatch<Ember::MouseButtonEvents>(EMBER_BIND_FUNC(MouseDown));
	}
private:
	struct Node {
		Node* parent = nullptr;
		float global;
		float local;

		bool is_obstacle = false;
		bool visited = false;

		Ember::IVec2 position;
		std::vector<Node*> neighbors;
	};

	Ember::Color background_color = { 0, 0,0, 255 };
	Node* nodes;
	Node* start_node = nullptr;
	Node* end_node = nullptr;
	Ember::IVec2 grid_size = { 16, 16 };
};

int main(int argc, char** argv) {
	Sandbox sandbox;
	sandbox.Initialize("Path Finding", false, 644, 644);

	sandbox.Run();
	return 0;
}