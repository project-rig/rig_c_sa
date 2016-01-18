/**
 * Simulated annealing algorithm for SpiNNaker application placement for use
 * with the Rig toolsuite.
 */

#ifndef SA_H
#define SA_H

////////////////////////////////////////////////////////////////////////////////
// Bools, for Windows support...
////////////////////////////////////////////////////////////////////////////////

typedef int sa_bool_t;
static const sa_bool_t sa_true = 1;
static const sa_bool_t sa_false = 0;

////////////////////////////////////////////////////////////////////////////////
// Data structures
////////////////////////////////////////////////////////////////////////////////

typedef struct sa_net sa_net_t;
typedef struct sa_vertex sa_vertex_t;

// Information associated with an individual net
struct sa_net {
	double weight;
	
	// The number of vertices in the net (and thus the length of the array below)
	size_t num_vertices;
	
	// Has this net been counted when computing net weight? (Used by
	// sa_get_swap_cost).
	sa_bool_t counted;
	
	// The set of vertices which belong to this net
	sa_vertex_t *vertices[];
};


// The state of a particular vertex
struct sa_vertex {
	// The coordinates of the chip this vertex is placed on
	int x;
	int y;
	
	// A pointer to the array of resources this vertex consumes...
	int *vertex_resources;
	
	// Pointer to the next vertex on the same chip as this one (or NULL if no
	// more). Note that non-movable vertices simply never appear in these linked
	// lists.
	sa_vertex_t *next;
	
	size_t num_nets;
	
	// The array of nets this vertex is a member of
	sa_net_t *nets[];
};


// The state of the whole algorithm
typedef struct sa_state {
	// The dimensions of the system under simulation
	size_t width;
	size_t height;
	
	// Is the network a torus?
	sa_bool_t has_wrap_around_links;
	
	// The number of resource types in existance
	size_t num_resource_types;
	
	// The amount of resources currently free on each chip. Set to a negative
	// quantity to indicate a dead chip.
	// An array [width][height][num_resource_types].
	int *chip_resources;
	
	// An array [width][height] giving a pointer to the first (movable) vertex on
	// the specified chip (or NULL if none on the chip).
	sa_vertex_t **chip_vertices;
	
	// An array of all the nets in the graph (for ease-of-freeing)
	size_t num_nets;
	sa_net_t **nets;
	
	// An array of all the vertices in the graph. The first num_movable_vertices
	// vertices are the ones which are movable, the others may not be moved.
	size_t num_vertices;
	size_t num_movable_vertices;
	sa_vertex_t **vertices;
	
} sa_state_t;


////////////////////////////////////////////////////////////////////////////////
// Constructors
////////////////////////////////////////////////////////////////////////////////

/**
 * Initialise a new SA algorithm state ready to hold a graph of a given size.
 *
 * Note that to avoid the need for special-case code the following "trivial"
 * scenarios are not allowed:
 *  - Systems must contain at least two chips (i.e. be larger than 1x1)
 *  - A least one type of chip-wide resource should be defined
 *  - There must be at least 1 movable vertex to place.
 *
 * After creating an sa_state_t struct with this function, you should then take
 * the following steps to completely initialise it:
 *  - state->has_wrap_around_links should be set to true or false depending on
 *    whether the system is configured as a torus (true) or not (false).
 *  - the state->chip_resources array should be initialised (see
 *    sa_set_chip_resources) to give the resources available on all chips.
 *    Dead chips should be given negative resource quantities.
 *  - state->num_movable_vertices should be set to indicate how many of the
 *    num_vertices in the system can be moved.
 *  - state->vertices[] should have *all* of its elements set to pointers
 *    to vertices to be placed. The first state->num_movable_vertices must be
 *    the vertices which may be moved and the remaining vertices must not be
 *    movable.
 *  - state->nets[] should have *all* of its elements set to pointers to nets
 *    that connect vertices defined in state->vertices[].
 *  - sa_add_vertex_to_net() should be used to associate all related nets and
 *    vertices.
 *  - sa_add_vertex_to_chip() should be used to specifiy the initial positions
 *    of every movable and non-movable vertex. The initial placement should be
 *    valid (i.e. not over-allocate resources).
 *
 * @param width The width of the hexagonal network network in chips.
 * @param height The height of the hexagonal network network in chips.
 * @param num_resource_types The number of types of chip-wide resource types.
 * @param num_vertices The exact number of vertices to be placed.
 * @param num_nets The exact number of uniqe nets between vertices.
 *
 * @returns A pointer to a new sa_state_t or NULL if memory allocation failed.
 *          Must be freed by sa_free()
 */
sa_state_t *sa_new(size_t width, size_t height, size_t num_resource_types,
                   size_t num_vertices, size_t num_nets);

/**
 * Free all memory associated with a SA algorithm run (including all nets and
 * vertices).
 */
void sa_free(sa_state_t *state);

/**
 * Initialise a new vertex which is connected to the specified number of nets.
 *
 * After calling this function the following initialisation steps are required:
 *  - A pointer to the new vertex should be added to state->vertices[] (see
 *    sa_new()).
 *  - All resources consumed by the vertex must be specified in
 *    vertex->vertex_resources[]. These values must not be changed once the
 *    vertex has been added to a chip.
 *  - The vertex should be added to a specific chip using
 *    sa_add_vertex_to_chip() *after* setting all vertex resource requirements.
 *  - All nets this vertex is connected to must be created using
 *    sa_add_vertex_to_net().
 *
 * @param state The SA algorithm state associated with the vertex.
 * @param num_nets The exact number of unique nets that this vertex is
 *                 connected to.
 *
 * @returns A pointer to a new sa_vertex_t or NULL if memory allocation failed.
 *          Must be freed by sa_free() (which internally uses sa_free_vertex()).
 */
sa_vertex_t *sa_new_vertex(const sa_state_t *state, size_t num_nets);

/**
 * For internal use only. Free all memory associated with a vertex.
 */
void sa_free_vertex(sa_vertex_t *vertex);

/**
 * Initialise a new net which connects the specified number of vertices.
 *
 * After calling this function the following initialisation steps are required:
 *  - A pointer to the new net should be added to state->nets[] (see sa_new()).
 *  - The net's weight should be set in net->weight to a positive double.
 *  - All vertices the net connects must be specified using
 *    sa_add_vertex_to_net(). Note: Source vertices must be added to the net
 *    like any other vertex. A net should only be added to a vertex exactly
 *    once (even if the net sources and sinks at the vertex).
 *
 * @param state The SA algorithm state associated with the vertex.
 * @param num_nets The exact number of unique nets that this vertex is
 *                 connected to.
 *
 * @returns A pointer to a new sa_net_t or NULL if memory allocation failed.
 *          Must be freed by sa_free() (which internally uses sa_free_net()).
 */
sa_net_t *sa_new_net(const sa_state_t *state, size_t num_vertices);

/**
 * For internal use only. Free all memory associated with a net.
 */
void sa_free_net(sa_net_t *net);

/**
 * Add the specified vertex to the specified chip and decrement the resources
 * available accordingly.
 *
 * @param state The SA algorithm state associated with the vertex.
 * @param vertex The vertex to add to a chip.
 * @param x The X coordinate of the chip the vertex should be added to.
 * @param y The Y coordinate of the chip the vertex should be added to.
 * @param movable Is the vertex a movable vertex or not?
 */
void sa_add_vertex_to_chip(sa_state_t *state, sa_vertex_t *vertex, int x, int y, sa_bool_t movable);

/**
 * Add the specified vertex to a net, updating the datastructures of both.
 *
 * Intended for use during initialisation (not hugely efficient!)
 *
 * @param state The SA algorithm state associated with the net and vertex.
 * @param net The net to which a vertex is to be added.
 * @param vertex The vertex to add to the net.
 */
void sa_add_vertex_to_net(const sa_state_t *state, sa_net_t *net, sa_vertex_t *vertex);


////////////////////////////////////////////////////////////////////////////////
// Accessor utillty functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Get a pointer to the resource array for the given chip.
 */
int *sa_get_chip_resources_ptr(sa_state_t *state, size_t x, size_t y);

/**
 * Get the quantity of a resource available on a given chip.
 */
int sa_get_chip_resources(sa_state_t *state, size_t x, size_t y, size_t resource);

/**
 * Set the quantity of a resource available on a given chip.
 */
void sa_set_chip_resources(sa_state_t *state, size_t x, size_t y, size_t resource, int value);

/**
 * Get the head of the linked list of vertices on the specified chip.
 */
sa_vertex_t *sa_get_chip_vertex(sa_state_t *state, size_t x, size_t y);

/**
 * Set the head of the linked list of vertices on the specified chip.
 */
void sa_set_chip_vertex(sa_state_t *state, size_t x, size_t y, sa_vertex_t *vertex);


////////////////////////////////////////////////////////////////////////////////
// Resource array utility functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Subtract the resources b from a, updating a.
 *
 * @param state The SA algorithm state for the resources being subtracted.
 * @param a Pointer to the resource array to be subtracted-from (will be
 *          modified)
 * @param b Pointer to the resource array to be subtracted
 */
void sa_subtract_resources(const sa_state_t *state, int *a, const int *b);

/**
 * Add the resources b to a, updating a.
 *
 * @param state The SA algorithm state for the resources being subtracted.
 * @param a Pointer to the resource array to be added-to (will be modified)
 * @param b Pointer to the resource array to be added
 */
void sa_add_resources(const sa_state_t *state, int *a, const int *b);

/**
 * Return true if all resource quantities are positive or zero.
 *
 * @param state The SA algorithm state for the resources being subtracted.
 * @param a Pointer to the resource array to be added-to (will be modified)
 */
sa_bool_t sa_positive_resources(const sa_state_t *state, const int *a);


////////////////////////////////////////////////////////////////////////////////
// General SA data structure manipulation functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Add a linked-list of movable vertices to the specified chip, subtracting the
 * resources consumed from the chip resources.
 *
 * Vertices are added in reverse order to the chip (so the last vertex in the
 * linked list will become the head of the linked list of the chip).
 *
 * @param state The SA algorithm state for the vertices being moved.
 * @param vertices The head of a linked-list of vertices (linked by the
 *                 vertices next field) which are not currently located on any
 *                 particular chip.
 * @param x The X coordinate of the chip to add the vertices to.
 * @param y The Y coordinate of the chip to add the vertices to.
 */
void sa_add_vertices_to_chip(sa_state_t *state, sa_vertex_t *vertices, int x, int y);

/**
 * Add a linked-list of movable vertices to the specified chip, subtracting the
 * resources consumed from the chip resources, only if the vertices would not
 * consume more resources than were available on the specified chip.
 *
 * Vertices are added in order to the chip (so the first vertex in the linked
 * list will become the head of the linked list of the chip).
 *
 * @param state The SA algorithm state for the vertices being moved.
 * @param vertices The head of a linked-list of vertices (linked by the
 *                 vertices next field) which are not currently located on any
 *                 particular chip.
 * @param x The X coordinate of the chip to add the vertices to.
 * @param y The Y coordinate of the chip to add the vertices to.
 *
 * @returns True if the vertices fit (and have been added to the chip) and
 *          false otherwise (leaving everything unchanged).
 */
sa_bool_t sa_add_vertices_to_chip_if_fit(sa_state_t *state, sa_vertex_t *vertices, int x, int y);

/**
 * Remove the specified movable vertex from its chip, incrementing the resources
 * available on that chip accordingly.
 *
 * @param state The SA algorithm state associated with the vertex.
 * @param vertex The vertex to be removed from its chip.
 */
void sa_remove_vertex_from_chip(sa_state_t *state, sa_vertex_t *vertex);

/**
 * Given a chip, remove vertices from it until the specified quantity of
 * resources are available.
 *
 * If no number of vertices can be used to free up the requested space then the
 * chip (and its vertices) will be left unchanged and false returned.
 *
 * @param state The SA algorithm state associated with the chip.
 * @param x The X coordinate of the chip to remove vertices from.
 * @param y The Y coordinate of the chip to remove vertices from.
 * @param resources_required The required resource quantities to be freed.
 * @param removed_vertices Will be set as the head of a linked-list of
 *                         vertices which were removed.
 *
 * @returns True if required space was freed up, false otherwise.
 */
sa_bool_t sa_make_room_on_chip(sa_state_t *state, int x, int y,
                               const int *resources_required,
                               sa_vertex_t **removed_vertices);


////////////////////////////////////////////////////////////////////////////////
// SA data "random" functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Select a movable vertex at random with uniform probability.
 *
 * The rand() function is used to generate random numbers and may be seeded as
 * usual.
 *
 * @param state The SA algorithm state from which to pick a movable vertex.
 * @returns A pointer to a movable vertex.
 */
sa_vertex_t *sa_get_random_movable_vertex(const sa_state_t *state);

/**
 * Select another chip randomly which is within the specified range of the
 * specified chip.
 *
 * @param state The SA algorithm state describing the chips to chose from
 * @param x The X coordinate of the chip near which another will be chosen.
 * @param y The Y coordinate of the chip near which another will be chosen.
 * @param distance_limit The "radius" of the square (on the X and Y axes)
 *                       around chip x,y from which chips may be selected. Must
 *                       be >= 1.
 * @param x_out Pointer to be set to the X coordinate of the selected chip.
 * @param y_out Pointer to be set to the Y coordinate of the selected chip.
 */
void sa_get_random_nearby_chip(const sa_state_t *state, int x, int y,
                               int distance_limit,
                               int *x_out, int *y_out);


////////////////////////////////////////////////////////////////////////////////
// Simulated annealing algorithm functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Compute the current cost of the specified net.
 *
 * Cost is estimated using a simple HPWL heuristic on a square grid...
 */
double sa_get_net_cost(sa_state_t *state, sa_net_t *net);

/**
 * Compute the change in cost which would result from swapping the location of
 * the vertices va and vb.
 *
 * The vertices va must initially have their x and y fields set to ax and ay.
 * Vertices vb must conversely have them set to bx and by. A side-effect of
 * this function is that the x and y values of vertices va will be swapped with
 * those of vb. Since the vertices will be later re-added to some chip
 * depending on the result of this computation, the values will be re-set later
 * regardless.
 *
 * @param state The SA algorithm state associated with the vertices.
 * @param ax The X position of the chip vertices va were removed from.
 * @param ay The Y position of the chip vertices va were removed from.
 * @param va A linked list of vertices which have been removed from a chip.
 * @param bx The X position of the chip vertices vb were removed from.
 * @param by The Y position of the chip vertices vb were removed from.
 * @param vb A linked list of vertices which have been removed from a chip.
 *
 * @returns The change in cost which would result from performing the proposed
 *          swap. -ve is better.
 */
double sa_get_swap_cost(sa_state_t *state,
                        int ax, int ay, sa_vertex_t *va,
                        int bx, int by, sa_vertex_t *vb);

/**
 * Attempt a single random swap operation and accept it according to the rules
 * of the SA.
 *
 * @param state The SA algorithm state to run within.
 * @param distance_limit The maximum rectangular-radius a swap may be made over.
 * @param temperature The current annealing temperature.
 * @param cost The change in cost since calling this function.
 *
 * @returns True if the swap as accepted, false if the swap as rejected either
 *          on cost grounds or because the vertex selected for swapping
 *          could not be swapped with the chosen target chip without running
 *          out of room.
 */
sa_bool_t sa_step(sa_state_t *state, int distance_limit, double temperature, double *cost);

/**
 * Run a predetermined number of random swaps at a given temperature and
 * distance limit and return statistics about the run.
 *
 * @param state The SA algorithm state to run within.
 * @param num_steps The number of steps to attempt.
 * @param distance_limit The maximum rectangular-radius a swap may be made over.
 * @param temperature The current annealing temperature.
 * @param num_accepted Returns the number of swaps of the num_steps made which
 *        were accepted.
 * @param cost_delta Returns the overall change in cost after the run.
 * @param cost_delta_sd Returns the standard deviation of cost changes during
 *                      the run.
 */
void sa_run_steps(sa_state_t *state, size_t num_steps, int distance_limit, double temperature,
                  size_t *num_accepted, double *cost_delta, double *cost_delta_sd);

#endif
