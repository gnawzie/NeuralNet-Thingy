#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
typedef struct Node Node;
typedef struct Conn Conn;
typedef struct NeuralNet NeuralNet;
typedef enum NodeType NodeType;
typedef enum ConnType ConnType;
//Design Net -> Nodes -> ptr
//			-> Conns ->
struct NeuralNet {
	int node_count;
	int conn_count;
	Node *nodes;
	Conn *conns;
};
enum NodeType { NODE_INTERNAL, NODE_OUTPUT, NODE_INPUT, NODE_INACTIVE };
enum ConnType { CONN_ACTIVE, CONN_INACTIVE };
struct Conn {
	Node *to_node;
	Node *from_node;
	float value;
	float gain;
};

struct Node {
	int input_count;
	Conn **inputs;
	int output_count;
	Conn **outputs;
	float value;
	int x;
	int y;
};

Node NeuralNet_new_node(void){
	Node new;
	new.value = 0;
	new.input_count = 0;
	new.inputs = NULL;
	new.output_count = 0;
	new.outputs = NULL;
	new.x = 0;
	new.y = 0;
	return new;
};
Conn NeuralNet_new_conn(void){
	Conn new;
	new.to_node = NULL;
	new.from_node = NULL;
	new.gain = 0;
	new.value = 0;
	return new;
}
NeuralNet NeuralNet_new(int conns, int nodes){
	NeuralNet new;
	int i;
	new.conn_count = conns;
	new.conns = malloc(sizeof(Conn) * conns);
	for(i=0;i<conns;i++){
		new.conns[i] = NeuralNet_new_conn();
	}
	new.node_count = nodes;
	new.nodes = malloc(sizeof(Node) * nodes);
	for(i=0;i<nodes;i++){
		new.nodes[i] = NeuralNet_new_node();
	}
	return new;
}
int NeuralNet_connect_random_nodes(NeuralNet *net){
	Node *node = net->nodes; //Holds pointer to the node list we will randomly index
	Conn *conns = net->conns; //Points to the connection list we will index through
	int conn_count = net->conn_count; //Holds the count to iterate to
	int node_count = net->node_count; //Holds node count so we can select random in that range
	int random_node_idx;
	int i; //Iterator
	
	for(i=0;i<conn_count;i++){
		random_node_idx = rand() % node_count;
		conns[i].from_node = node+random_node_idx;
		conns[i].from_node->output_count++; //Increment input count, we will malloc later during output list gen
		random_node_idx = rand() % node_count;
		conns[i].to_node = node+random_node_idx;
		conns[i].to_node->input_count++; //Increment output count, we will malloc later during output list gen
		conns[i].gain = 1.0/(rand() % 1000)+1; //Random between 1/~-499.5 and 1/~500 while avoiding division by zero with a +1
		conns[i].gain = (rand() %2) == 0 ? conns[i].gain : -conns[i].gain;
	}
	return 0;
}
int NeuralNet_generate_node_input_outputs(NeuralNet *net){
	Node *node_list = net->nodes; //Holdes node list
	Conn *conn_list = net->conns; //Holds connection list
	Node *from_node; //Holds from node pointer
	Node *to_node; //Holds to node pointer
	int conn_count = net->conn_count; //etc etc
	int node_count = net->node_count;
	int i;
	//Scan through nodes, allocating space indicated by their input/output count if they have one
	for(i=0;i<node_count;i++){
		if(node_list[i].input_count > 0){
			node_list[i].inputs = malloc(sizeof(Conn*) * node_list[i].input_count); //Allocate space to hold pointers
			node_list[i].input_count = 0; //Zeroing, we will use as index and add back up
		}
		if(node_list[i].output_count > 0){
			node_list[i].outputs = malloc(sizeof(Conn*) * node_list[i].output_count); //Same for outputs
			node_list[i].output_count = 0; //Zeroing we will use as index and count back up
		}
	}
	//Scan through connections, get their nodes, and add a pointer to their connections to their input/output lists
	for(i=0;i<conn_count;i++){
		to_node = conn_list[i].to_node;
		from_node = conn_list[i].from_node;
		if((to_node != NULL) && (from_node != NULL)){ //Ensure the connection is set
			to_node->inputs[to_node->input_count++] = conn_list+i; //Write the pointer of the connection to the list and increment
			from_node->outputs[from_node->output_count++] = conn_list+i;
		}
	}
	return 0;
}
int NeuralNet_randomize_node_pos(NeuralNet *net){
	Node *node_list = net->nodes;
	Node *node;
	int node_count = net->node_count;
	int i;
	for(i=0;i<node_count;i++){
		node = node_list+i;
		node->x = rand() % WINDOW_WIDTH;
		node->y = rand() % WINDOW_HEIGHT;
	}
	return 0;
}
int NeuralNet_update(NeuralNet *net){
	Node *node_list = net->nodes;
	Node *node;
	Conn *conn;
	int node_input_count;
	int node_output_count;
	int node_count = net->node_count;
	int i,j;
	//Scan through node list
	for(i=0;i<node_count;i++){
		node = node_list+i;
		node_input_count = node->input_count;
		node_output_count = node->output_count;
		//printf("Now updating node %d\n",i);
		//Scan through the node's inputs
		node->value = 0;
			for(j=0;j<node_input_count;j++){
				conn = node->inputs[j];
				node->value += conn->value * conn->gain;
				//printf("Node %d's value is now %f using input %d\n",i, node->value, j);
			}
			for(j=0;j<node_output_count;j++){
				conn = node->outputs[j];
				conn->value = node->value;
				//printf("Node %d's value transferred to output %d with value %f\n",i,j,conn->value);
			}
		}
		return 0;
	}
int NeuralNet_render_nodes(SDL_Renderer *renderer, NeuralNet *net){
	Node *node_list = net->nodes;
	Node *node;
	int node_count = net->node_count;
	SDL_Rect node_rect = {.w = 10, .h = 10, .x = 0, .y = 0 };
	int i;
	for(i=0;i<node_count;i++){
		node = node_list+i;
		node_rect.x = node->x;
		node_rect.y = node->y;
		SDL_SetRenderDrawColor(renderer, node->value > 0 ? node->value : 0, node->value <0?-node->value:0,128,255);
		SDL_RenderFillRect(renderer, &node_rect);
	}
	return 0;
}
int NeuralNet_render_conns(SDL_Renderer *renderer, NeuralNet *net){
	Conn *conn_list = net->conns;
	Node *to_node;
	Node *from_node;
	int conn_count = net->conn_count;
	int i;
	int red;
	int blue;
	for(i=0;i<conn_count;i++){
		to_node = conn_list[i].to_node;
		from_node = conn_list[i].from_node;
		if((to_node != NULL) && (from_node != NULL)){
			red = conn_list[i].value > 0 ? conn_list[i].value : 0;
			blue = conn_list[i].value < 0 ? -conn_list[i].value : 0;
			SDL_SetRenderDrawColor(renderer, red,blue,255,255);
			SDL_RenderDrawLine(renderer,from_node->x, from_node->y, to_node->x, to_node->y);
		}
	}
	return 0;
}
int NeuralNet_print_net(NeuralNet *net){
	Node *node_list = net->nodes;
	Node *node;
	Conn *conn;
	int node_count = net->node_count;
	int conn_count = net->conn_count;
	int node_input_count;
	int node_output_count;
	int i,j;
	printf("Network data: %d nodes, %d connections\n",node_count, conn_count);
	printf("Node data as follows:\n");
	for(i=0;i<node_count;i++){
		node = node_list+i;
		node_input_count = node->input_count;
		node_output_count = node->output_count;
		printf("--->Node %d has %d inputs and %d outputs, verifying inputs\n",i,node_input_count,node_output_count);
		if(node_input_count != 0){
			for(j=0;j<node_input_count;j++){
				conn = node->inputs[j];
				if(conn->to_node == node){
					printf("----->Connection %d does point to this node\n",j);
				}else{
					printf("----->Connection %d doesn't point to this node, bug\n",j);
				}
			}
		}else{
			printf("---->This node has no inputs\n");
		}
		printf("--->Now verifying outputs\n");
		if(node_output_count != 0){
			for(j=0;j<node_output_count;j++){
				conn = node->outputs[j];
				if(conn->from_node == node){
					printf("----->Connection %d points from this node\n",j);
				}else{
					printf("----->Connection %d doesn't point from this node, bug\n",j);
				}
			}
		}else{
			printf("--->This node has no outputs\n");
		}
	}
}
int NeuralNet_connect_all_nodes(NeuralNet *net){
	Node *node_list = net->nodes;
	Conn *connection;
	Conn *conn_list = net->conns;
	Node *from_node;
	Node *to_node;	
	int node_count = net->node_count;
	int conn_iter = 0;
	int i,j;
	for(i=0;i<node_count;i++){
		printf("Now on %d\n",i);
		from_node = node_list+i;
		for(j=0;j<node_count;j++){
			if(j==i){
				printf("Skipping same one\n");
				continue;
			}
			connection = conn_list+conn_iter++;
			to_node = node_list+j;
			
			connection->from_node = from_node;
			connection->to_node = to_node;
			printf("from = %d, to = %d ,conns=%d\n",i,j,conn_iter);
		}
		printf("%d away from completion\n",node_count-i);
	}
	printf("Done!");
	return 0;
}
NeuralNet NeuralNet_generate_layered_net(int layers, int node_per_layer){
	NeuralNet net = NeuralNet_new(layers * node_per_layer * node_per_layer, node_per_layer * layers);
	Node *node_list = net.nodes;
	Conn *conn_list = net.conns;
	Conn *conn;
	Node *node;
	Node *from_node;
	Node *to_node;
	int i,j,k;
	int x= 0,y = 0;
	int node_iter = 0;
	int conn_iter = 0;
	int x_step = WINDOW_WIDTH/layers;
	int y_step = WINDOW_HEIGHT/node_per_layer;
	for(i=0;i<layers;i++){
		for(j=0;j<node_per_layer;j++){
			node = node_list+node_iter++;
			node->x = x;
			node->y = y += y_step;
		}
		y=0;
		x+=x_step;
	}
	//Starting from first layer up to last layer (no forward connections after last layer)
	for(i=1;i<layers;i++){
		//For each node on the layer
		for(j=0;j<node_per_layer;j++){
			//The from node will be each node on the layer
			from_node = node_list+(j+(i-1)*node_per_layer);
			//The next node will be each of the node of the next layer
			for(k=0;k<node_per_layer;k++){
				//Here we multiply by i to jump over to the next layer, then add k to select the node of that layer
				to_node = node_list+(k+(i*node_per_layer));
				//Do the connection
				conn = conn_list+conn_iter++;
				conn->from_node = from_node;
				conn->to_node = to_node;
				conn->gain = 0.01;
				from_node->output_count++;
				to_node->input_count++;
			}
		}
	}
	return net;
}
int main(int argc, char **argv)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
	SDL_RenderClear(renderer);
	
	NeuralNet new_net = NeuralNet_generate_layered_net(10, 10);
	//NeuralNet_print_net(&new_net);
	NeuralNet_generate_node_input_outputs(&new_net);
	NeuralNet_print_net(&new_net);
	int i;
	long error = 0;
	int time = 0;
	while(time < 200){
		new_net.nodes[0].value = sin(M_PI*2/200 * time)*255;
		printf("%f\n",new_net.nodes[99].value);
		NeuralNet_update(&new_net);
		NeuralNet_render_nodes(renderer, &new_net);
		NeuralNet_render_conns(renderer, &new_net);
		SDL_RenderPresent(renderer);
		SDL_Delay(50);
		time++;
	}
	return 0;
}

