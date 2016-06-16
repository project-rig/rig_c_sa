/**
 * Rig-SA: A simple simulated-annealing based placement algorithm implemented
 * in C.
 */

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <assert.h>

#include <math.h>

#include "usort/u1_sort.c"

#include "sa.h"

// Windows support for alloca...
#if defined(_WIN32) || defined(WIN32)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif


////////////////////////////////////////////////////////////////////////////////
// Constructors & Destructors
////////////////////////////////////////////////////////////////////////////////

sa_state_t *sa_new(size_t width, size_t height, size_t num_resource_types,
                   size_t num_vertices, size_t num_nets) {
	size_t x, y, r, i;
	sa_state_t *state;
	
	assert(width > 0);
	assert(height > 0);
	assert(width > 1 || height > 1);
	assert(num_resource_types >= 1);
	assert(num_vertices >= 1);
	
	// Allocate memory for main state object
	state = malloc(sizeof(sa_state_t));
	if (state == NULL)
		return NULL;
	
	state->has_wrap_around_links = sa_false;
	state->num_movable_vertices = 0;
	
	// A simple machine with Cores and SDRAM
	state->width = width;
	state->height = height;
	state->num_resource_types = num_resource_types;
	
	// Allocate memory for chip resource counters
	state->chip_resources = calloc(state->width * state->height * state->num_resource_types, 
	                               sizeof(int));
	if (state->chip_resources == NULL) {
		free(state);
		return NULL;
	}
	
	// Allocate memory for chip vertex LL heads
	state->chip_vertices = calloc(state->width * state->height, sizeof(sa_vertex_t *));
	if (state->chip_vertices == NULL) {
		free(state->chip_resources);
		free(state);
		return NULL;
	}
	
	// Initialise the lookup for chip resources. Resources are -ve on dead chips.
	for (x = 0; x < state->width; x++) {
		for (y = 0; y < state->height; y++) {
			for (r = 0; r < state->num_resource_types; r++) {
				sa_set_chip_resources(state, x, y, r,  -1);
			}
			sa_set_chip_vertex(state, x, y, NULL);
		}
	}
	
	// Allocate memory for vertex pointers
	state->num_vertices = num_vertices;
	state->vertices = calloc(state->num_vertices, sizeof(sa_vertex_t *));
	if (state->vertices == NULL) {
		free(state->chip_resources);
		free(state->chip_vertices);
		free(state);
		return NULL;
	}
	for (i = 0; i < state->num_vertices; i++)
		state->vertices[i] = NULL;
	
	// Allocate memory for net pointers
	state->num_nets = num_nets;
	state->nets = calloc(state->num_nets, sizeof(sa_net_t *));
	if (state->nets == NULL) {
		free(state->vertices);
		free(state->chip_resources);
		free(state->chip_vertices);
		free(state);
		return NULL;
	}
	for (i = 0; i < state->num_nets; i++)
		state->nets[i] = NULL;
	
	return state;
}

void sa_free(sa_state_t *state) {
	size_t n, v;
	if (!state)
		return;
	
	for (n = 0; n < state->num_nets; n++)
		sa_free_net(state->nets[n]);
	
	for (v = 0; v < state->num_vertices; v++)
		sa_free_vertex(state->vertices[v]);
	
	free(state->vertices);
	free(state->nets);
	free(state->chip_vertices);
	free(state->chip_resources);
	free(state);
}

sa_vertex_t *sa_new_vertex(const sa_state_t *state, size_t num_nets) {
	size_t i;
	sa_vertex_t *vertex;
	
	// Allocate the array of resources
	int *resources = calloc(state->num_resource_types, sizeof(int));
	if (resources == NULL)
		return NULL;
	
	// Allocate the vertex itself
	vertex = malloc( sizeof(sa_vertex_t)
	                              + (sizeof(sa_net_t *) * num_nets));
	if (vertex == NULL) {
		free(resources);
		return NULL;
	}
	
	vertex->vertex_resources = resources;
	vertex->num_nets = num_nets;
	
	// Keep valgrind happy...
	vertex->next = NULL;
	for (i = 0; i < num_nets; i++)
		vertex->nets[i] = NULL;
	
	return vertex;
}

void sa_free_vertex(sa_vertex_t *vertex) {
	if (!vertex)
		return;
	
	free(vertex->vertex_resources);
	free(vertex);
}

/**
 * Initialise a new net which is involved with the specified number of
 * vertices.
 */
sa_net_t *sa_new_net(const sa_state_t *state, size_t num_vertices) {
	size_t i;
	sa_net_t *net;
	(void) state;
	
	net = malloc(sizeof(sa_net_t) + (sizeof(sa_vertex_t *) * num_vertices));
	if (net == NULL)
		return NULL;
	
	net->num_vertices = num_vertices;
	net->counted = sa_false;
	
	// Keep valgrind happy
	for (i = 0; i < num_vertices; i++)
		net->vertices[i] = NULL;
	
	return net;
}

void sa_free_net(sa_net_t *net) {
	if (!net)
		return;
	
	free(net);
}

////////////////////////////////////////////////////////////////////////////////
// General data structure manipulation functions
////////////////////////////////////////////////////////////////////////////////

int *sa_get_chip_resources_ptr(sa_state_t *state, size_t x, size_t y) {
	return state->chip_resources + (
		(y * state->width * state->num_resource_types)
		+ (x * state->num_resource_types)
	);
}

int sa_get_chip_resources(sa_state_t *state, size_t x, size_t y, size_t resource) {
	return sa_get_chip_resources_ptr(state, x, y)[resource];
}

void sa_set_chip_resources(sa_state_t *state, size_t x, size_t y, size_t resource, int value) {
	sa_get_chip_resources_ptr(state, x, y)[resource] = value;
}

sa_vertex_t *sa_get_chip_vertex(sa_state_t *state, size_t x, size_t y) {
	return state->chip_vertices[(y * state->width) + x];
}

void sa_set_chip_vertex(sa_state_t *state, size_t x, size_t y, sa_vertex_t *vertex) {
	 state->chip_vertices[(y * state->width) + x] = vertex;
}

void sa_subtract_resources(const sa_state_t *state, int *a, const int *b) {
	size_t i;
	for (i = 0; i < state->num_resource_types; i++)
		a[i] -= b[i];
}

void sa_add_resources(const sa_state_t *state, int *a, const int *b) {
	size_t i;
	for (i = 0; i < state->num_resource_types; i++)
		a[i] += b[i];
}

sa_bool_t sa_positive_resources(const sa_state_t *state, const int *a) {
	size_t i;
	for (i = 0; i < state->num_resource_types; i++)
		if (a[i] < 0)
			return sa_false;
	return sa_true;
}

void sa_add_vertex_to_chip(sa_state_t *state, sa_vertex_t *vertex, int x, int y, sa_bool_t movable) {
	vertex->x = x;
	vertex->y = y;
	
	// Insert the vertex into the LL of movable vertices on the target chip
	if (movable) {
		assert(vertex->next == NULL);
		vertex->next = sa_get_chip_vertex(state, x, y);
		sa_set_chip_vertex(state, x, y, vertex);
	}
	
	// Subtract the resources consumed from those available on the chip
	sa_subtract_resources(state,
	                      sa_get_chip_resources_ptr(state, x, y),
	                      vertex->vertex_resources);
}

void sa_add_vertices_to_chip(sa_state_t *state, sa_vertex_t *vertices, int x, int y) {
	while (vertices != NULL) {
		// Detatch the head of list
		sa_vertex_t *vertex = vertices;
		vertices = vertex->next;
		vertex->next = NULL;
		
		// Add it to the chip
		sa_add_vertex_to_chip(state, vertex, x, y, sa_true);
	}
}

sa_bool_t sa_add_vertices_to_chip_if_fit(sa_state_t *state, sa_vertex_t *vertices, int x, int y) {
	int *resources_available = alloca(sizeof(int) * state->num_resource_types);
	sa_vertex_t *v;
	
	memcpy(resources_available, sa_get_chip_resources_ptr(state, x, y),
	       sizeof(int) * state->num_resource_types);
	
	v = vertices;
	while (v) {
		// Update the coordinates of the vertex as we pass. If we don't actually
		// end up adding the vertex to the chip, this change has no meaningful
		// effect.
		v->x = x;
		v->y = y;
		
		sa_subtract_resources(state, resources_available, v->vertex_resources);
		
		// Leave v pointing at the last vertex
		if (v->next == NULL)
			break;
		else
			v = v->next;
	}
	
	if (sa_positive_resources(state, resources_available)) {
		// The vertices fit, insert them
		if (vertices) {
			v->next = sa_get_chip_vertex(state, x, y);
			sa_set_chip_vertex(state, x, y, vertices);
			
			// And update the resource consumption
			memcpy(sa_get_chip_resources_ptr(state, x, y), resources_available,
			       sizeof(int) * state->num_resource_types);
		}
		return sa_true;
	} else {
		// The vertices didn't fit, just stop now
		return sa_false;
	}
}

void sa_remove_vertex_from_chip(sa_state_t *state, sa_vertex_t *vertex) {
	sa_vertex_t *head = sa_get_chip_vertex(state, vertex->x, vertex->y);
	sa_vertex_t **next_ptr = &head;
	
	// Search through the linked-list for the vertex to remove
	while (*next_ptr != vertex) {
		assert((*next_ptr)->next != NULL);  // The vertex *must* be present!
		next_ptr = &((*next_ptr)->next);
	}
	
	// Remove it
	*next_ptr = vertex->next;
	vertex->next = NULL;
	
	sa_set_chip_vertex(state, vertex->x, vertex->y, head);
	
	// Account for the resources now freed up
	sa_add_resources(state,
	                 sa_get_chip_resources_ptr(state, vertex->x, vertex->y),
	                 vertex->vertex_resources);
}

void sa_add_vertex_to_net(const sa_state_t *state, sa_net_t *net, sa_vertex_t *vertex) {
	size_t i;
	(void) state;
	
	// Add vertex to net's list of vertices
	for (i = 0; i < net->num_vertices; i++) {
		if (net->vertices[i] == NULL) {
			net->vertices[i] = vertex;
			break;
		}
	}
	assert(i < net->num_vertices);
	
	// Add net to vertex's list of nets
	for (i = 0; i < vertex->num_nets; i++) {
		if (vertex->nets[i] == NULL) {
			vertex->nets[i] = net;
			break;
		}
	}
	assert(i < vertex->num_nets);
}

sa_vertex_t *sa_get_random_movable_vertex(const sa_state_t *state) {
	// TODO: Use a propper random scheme...
	return state->vertices[rand() % state->num_movable_vertices];
}

void sa_get_random_nearby_chip(const sa_state_t *state, int x, int y,
                               int distance_limit,
                               int *x_out, int *y_out) {
	int x_min, y_min, x_max, y_max;
	assert(distance_limit >= 1);
	
	*x_out = x;
	*y_out = y;
	
	// Compute the bounds of the region from which candidate chips may be chosen
	x_min = x - distance_limit;
	y_min = y - distance_limit;
	x_max = x + distance_limit;
	y_max = y + distance_limit;
	if (state->has_wrap_around_links) {
		// If the distance limit allows wrapping more than all the way around, just
		// clamp to that range
		if ((x_max - x_min) >= (int)state->width) {
			x_min = 0;
			x_max = (int)state->width - 1;
		}
		if ((y_max - y_min) >= (int)state->height) {
			y_min = 0;
			y_max = (int)state->height - 1;
		}
	} else {
		// No wrap-around links
		if (x_min < 0)
			x_min = 0;
		if (y_min < 0)
			y_min = 0;
		if (x_max > (int)state->width - 1)
			x_max = (int)state->width - 1;
		if (y_max > (int)state->height - 1)
			y_max = (int)state->height - 1;
	}
	
	// Note we must pick a chip which isn't this chip(!)
	while (*x_out == x && *y_out == y) {
		// XXX: TODO: Use a proper random scheme...
		*x_out = x_min + (rand() % ((x_max - x_min) + 1));
		*y_out = y_min + (rand() % ((y_max - y_min) + 1));
		
		// Wrap-around (if required)
		if (*x_out < 0)
			*x_out += (int)state->width;
		if (*x_out >= (int)state->width)
			*x_out -= (int)state->width;
		if (*y_out < 0)
			*y_out += (int)state->height;
		if (*y_out >= (int)state->height)
			*y_out -= (int)state->height;
	}
}


sa_bool_t sa_make_room_on_chip(sa_state_t *state, int x, int y,
                               const int *resources_required,
                               sa_vertex_t **removed_vertices) {
	
	// Create a local copy of the resource requirement on the stack
	int *resources_available = alloca(sizeof(int) * state->num_resource_types);
	memcpy(resources_available, sa_get_chip_resources_ptr(state, x, y),
	       sizeof(int) * state->num_resource_types);
	
	// See if the resources already available on the chip are sufficient alone
	sa_subtract_resources(state, resources_available, resources_required);
	
	// Keep removing vertices until all the requred resources have been found.
	*removed_vertices = NULL;
	while (!sa_positive_resources(state, resources_available)) {
		if (sa_get_chip_vertex(state, x, y) != NULL) {
			// Remove a vertex
			sa_vertex_t *new_chip_head = sa_get_chip_vertex(state, x, y)->next;
			sa_get_chip_vertex(state, x, y)->next = *removed_vertices;
			*removed_vertices = sa_get_chip_vertex(state, x, y);
			sa_set_chip_vertex(state, x, y, new_chip_head);
			
			sa_add_resources(state,
			                 resources_available,
			                 (*removed_vertices)->vertex_resources);
		} else {
			// Ran out of vertices to remove! Put them all back then report a
			// failure.
			sa_set_chip_vertex(state, x, y, *removed_vertices);
			*removed_vertices = NULL;
			return sa_false;
		}
	}
	
	// Update the resource counts for the chip if a vertex was removed
	if (*removed_vertices) {
		sa_add_resources(state, resources_available, resources_required);
		memcpy(sa_get_chip_resources_ptr(state, x, y), resources_available,
		       sizeof(int) * state->num_resource_types);
	}
	
	return sa_true;
}

int compar(const void *a, const void *b) {
	return *((int *)a) - *((int *)b);
}

void sort(sa_state_t *state, int *array, size_t length) {
  if (state->width <= 256 && state ->height <= 256) {
    // If dimensions are always 8 bits or less (true for all real SpiNNaker
    // machines), we use a fast sort algorithm.
    u1_sort(array, length);
  } else {
    // ...if something odd is being done, we use a bog-standard qsort.
    qsort(array, length, sizeof(int), &compar);
  }
}

double sa_get_net_cost(sa_state_t *state, sa_net_t *net) {
	size_t i;
	int *xs, *ys;
	int last_x, last_y, max_delta_x, max_delta_y;
	int delta_x, delta_y;
	int bbox_width, bbox_height;
	int min_x, max_x, min_y, max_y;
		
	// If 1 or 0 vertices in the net, the net can never have non-zero cost. This
	// also saves some special-case handling below.
	if (net->num_vertices <= 1)
		return 0.0;
	
	if (state->has_wrap_around_links) {
		// Torroidal network: When wrap-around links exist, we find the minimal
		// bounding box and return the HPWL weighted by the net weight. To do this
		// the largest gap between any pair of vertices is found:
		//
		//     |    x     x             x   |
		//                ^-------------^
		//                    max gap
		//
		// The minimal bounding box then goes the other way around:
		//
		//     |    x     x             x   |
		//      ----------^             ^---
		
		// Create a sorted array of the x and y positions
		xs = alloca(net->num_vertices * sizeof(int));
		ys = alloca(net->num_vertices * sizeof(int));
		
		for (i = 0; i < net->num_vertices; i++) {
			xs[i] = net->vertices[i]->x;
			ys[i] = net->vertices[i]->y;
		}
		
		sort(state, xs, net->num_vertices);
		sort(state, ys, net->num_vertices);
		
		// Find the largest gap in each
		last_x = xs[net->num_vertices - 1] - (int)state->width;
		last_y = ys[net->num_vertices - 1] - (int)state->height;
		max_delta_x = 0;
		max_delta_y = 0;
		for (i = 0; i < net->num_vertices; i++) {
			delta_x = xs[i] - last_x;
			delta_y = ys[i] - last_y;
			last_x = xs[i];
			last_y = ys[i];
			
			if (delta_x > max_delta_x)
				max_delta_x = delta_x;
			if (delta_y > max_delta_y)
				max_delta_y = delta_y;
		}
		
		// From this we can work out the bounding box size and thus the HPWL.
		bbox_width = (int)state->width - max_delta_x;
		bbox_height = (int)state->height - max_delta_y;
		return sqrt(net->num_vertices) * (bbox_width + bbox_height) * net->weight;
	} else {
		// Non-toriodal network: Compute bounding box
		min_x = net->vertices[0]->x;
		max_x = min_x;
		min_y = net->vertices[0]->y;
		max_y = min_y;
		for (i = 1; i < net->num_vertices; i++) {
			if (net->vertices[i]->x < min_x)
				min_x = net->vertices[i]->x;
			if (max_x < net->vertices[i]->x)
				max_x = net->vertices[i]->x;
			if (net->vertices[i]->y < min_y)
				min_y = net->vertices[i]->y;
			if (max_y < net->vertices[i]->y)
				max_y = net->vertices[i]->y;
		}
		
		// Compute weighted HPWL
		return sqrt(net->num_vertices) * ((max_x - min_x) + (max_y - min_y)) * net->weight;
	}
}

double sa_get_swap_cost(sa_state_t *state,
                        int ax, int ay, sa_vertex_t *va,
                        int bx, int by, sa_vertex_t *vb) {
	int which_verts;
	size_t i;
	sa_vertex_t *v;
	double after_cost;
	
	// Calculate total cost of all nets before swap
	double before_cost = 0.0;
	for (which_verts = 0; which_verts < 2; which_verts++) {
		sa_vertex_t *v = (which_verts == 0) ? va : vb;
		while (v) {
			for (i = 0; i < v->num_nets; i++) {
				if (!v->nets[i]->counted) {
					before_cost += sa_get_net_cost(state, v->nets[i]);
					v->nets[i]->counted = sa_true;
				}
			}
			v = v->next;
		}
	}
	
	// Swap the positions of all va and all vb. Note that since these vertices
	// have been removed and will later be re-added to the chip, restoring the
	// values of x and y if required.
	v = va;
	while (v) {
		v->x = bx;
		v->y = by;
		v = v->next;
	}
	v = vb;
	while (v) {
		v->x = ax;
		v->y = ay;
		v = v->next;
	}
	
	// Calculate the cost after swap
	after_cost = 0.0;
	for (which_verts = 0; which_verts < 2; which_verts++) {
		sa_vertex_t *v = (which_verts == 0) ? va : vb;
		while (v) {
			for (i = 0; i < v->num_nets; i++) {
				if (v->nets[i]->counted) { // Meaning inverted in this pass
					after_cost += sa_get_net_cost(state, v->nets[i]);
					v->nets[i]->counted = sa_false; // Meaning inverted in this pass
				}
			}
			v = v->next;
		}
	}
	
	return after_cost - before_cost;
}

sa_bool_t sa_step(sa_state_t *state, int distance_limit, double temperature, double *cost) {
	
	// Select a random vertex to swap
	sa_vertex_t *va = sa_get_random_movable_vertex(state);
	sa_vertex_t *vb;
	int ax = va->x;
	int ay = va->y;
	sa_bool_t swap_accepted;
	
	// Find a suitable chip B to place the vertex on
	int bx, by;
	sa_get_random_nearby_chip(state, ax, ay, distance_limit, &bx, &by);
	
	// Attempt to remove as many vertices from chip B as required (if any) to
	// allow our randomly selected vertex to fit. If not possible (e.g. due to
	// insufficient space even when you remove all vertices or due to a dead
	// chip), just fail the step.
	if (!sa_make_room_on_chip(state, bx, by,
	                          va->vertex_resources,
	                          &vb)) {
		*cost = 0.0;
		return sa_false;
	}
	
	// Remove the initially randomly selected vertex from its chip.
	sa_remove_vertex_from_chip(state, va);
	
	// Assess whether the swap chosen is acceptable and then proceed with the
	// final "but does it fit?" check. If the swap is not acceptable, revert and
	// give up now. Swaps that reduce the cost are always acceptable, swaps which
	// increase it are acceptable with a probability related to how bad the swap
	// is and how high the temperature is.
	*cost = sa_get_swap_cost(state, ax, ay, va, bx, by, vb);
	swap_accepted = ((*cost) <= 0.0)
	                 || ((double)rand() / RAND_MAX) < exp(-(*cost) / temperature);
	
	// Attempt to fit the vertices removed from chip B into the space left behind
	// after removing va from chip A. If not enough space (or if the swap was not
	// accepted, revert everything.
	if (!swap_accepted || !sa_add_vertices_to_chip_if_fit(state, vb, ax, ay)) {
		// The vertices didn't fit, put everything back where it came
		sa_add_vertices_to_chip(state, vb, bx, by);
		sa_add_vertices_to_chip(state, va, ax, ay);
		*cost = 0.0;
		return sa_false;
	}
	
	// Finally put va onto vb.
	sa_add_vertices_to_chip(state, va, bx, by);
	
	// Swap completed successfully
	return sa_true;
}

void sa_run_steps(sa_state_t *state, size_t num_steps, int distance_limit, double temperature,
                  size_t *num_accepted, double *cost_delta, double *cost_delta_sd) {
	size_t i;
	
	// Used to calculate a running standard-deviation of cost changes
	double mean = 0.0;
	double m2 = 0.0;
	
	double delta;
	
	*num_accepted = 0;
	*cost_delta = 0.0;
	
	for (i = 0; i < num_steps; i++) {
		double cost_change;
		sa_bool_t accepted = sa_step(state, distance_limit, temperature, &cost_change);
		
		if (accepted)
			(*num_accepted)++;
		
		*cost_delta += cost_change;
		
		delta = cost_change - mean;
		mean += delta / (i + 1.0);
		m2 += delta * (cost_change - mean);
	}
	
	// Calculate the standard deviation of cost changes
	*cost_delta_sd = sqrt(m2 / (num_steps - 1.0));
}
