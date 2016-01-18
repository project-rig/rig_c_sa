/**
 * Test the sa_state_t datastructure construction/destruction functions.
 */

#include <check.h>

#include <stdlib.h>
#include <stdbool.h>

#include "tests.h"

#include "sa.h"

START_TEST (test_constructors)
{
	// Define a 2x3 problem with 4 vertices to place with 4 nets between them and
	// 2 resource types. All but the last vertex will be movable
	size_t w = 2;
	size_t h = w + 1;
	size_t nv = 4;
	size_t nn = nv;
	size_t nr = 2;
	size_t nm = nv - 1;
	
	sa_state_t *s = sa_new(w, h, nr, nv, nn);
	ck_assert(s);
	
	// Check basic values stored in the datastructure (allocated structures will
	// be checked later)
	ck_assert(s->width == w);
	ck_assert(s->height == h);
	ck_assert(!s->has_wrap_around_links);
	ck_assert(s->num_resource_types == nr);
	ck_assert(s->num_nets == nn);
	ck_assert(s->num_vertices == nv);
	
	// Check the required memory has been allocated (Valgrind should check these
	// accesses fall in-range)
	for (size_t i = 0; i < w * h * nr; i++)
		ck_assert(s->chip_resources[i] == -1);
	for (size_t i = 0; i < w * h; i++)
		ck_assert(!s->chip_vertices[i]);
	for (size_t i = 0; i < nn; i++)
		ck_assert(!s->nets[i]);
	for (size_t i = 0; i < nv; i++)
		ck_assert(!s->vertices[i]);
	
	// Create some vertices and nets. Vertex 0 will connect to nets 0, 1, ...,
	// nn-1. Vertex 1 to nets 1, 2, ..., nn-1 and so-forth.
	s->num_movable_vertices = nm;
	for (size_t i = 0; i < nv; i++) {
		sa_vertex_t *v = sa_new_vertex(s, i + 1);
		ck_assert(v);
		ck_assert(v->num_nets == i + 1);
		s->vertices[i] = v;
		
		// The right amount of memory for resources should be allocated (Valgrind
		// will complain otherwise...)
		for (size_t j = 0; j < nr; j++)
			v->vertex_resources[j] = j;
		
		// Make sure that when added to the chip, the vertex datastructure is
		// updated accordingly.
		bool movable = i != (nv - 1);
		sa_add_vertex_to_chip(s, v, 0, 1, movable);
		ck_assert(v->x == 0);
		ck_assert(v->y == 1);
	}
	
	// Make sure that the vertices are on the right chip according to the main
	// datastructure
	for (size_t x = 0; x < w; x++) {
		for (size_t y = 0; y < h; y++) {
			if (x == 0 && y == 1) {
				// All *movable* vertices should be present.
				bool seen[nm];
				for (size_t i = 0; i < nm; i++)
					seen[i] = 0;
				
				// Iterate through the linked list of vertices on the chip
				sa_vertex_t *cur_v = SA_STATE_CHIP_VERTICES(s, x, y);
				while (cur_v) {
					// Check to see if this vertex is supposed to be here...
					bool known_vertex = false;
					for (size_t i = 0; i < nm; i++) {
						if (s->vertices[i] == cur_v) {
							ck_assert(!known_vertex);
							known_vertex = true;
							ck_assert(!seen[i]);
							seen[i] = true;
						}
					}
					ck_assert(known_vertex);
					cur_v = cur_v->next;
				}
				
				// Make sure all movable vertices were included
				for (size_t i = 0; i < nm; i++)
					ck_assert(seen[i]);
			} else {
				// Chip should be empty
				ck_assert(!SA_STATE_CHIP_VERTICES(s, x, y));
			}
		}
	}
	
	// Create the nets as described above
	for (size_t i = 0; i < nn; i++) {
		sa_net_t *n = sa_new_net(s, nv - i);
		ck_assert(n);
		s->nets[i] = n;
		
		ck_assert(n->num_vertices == nv - i);
		n->weight = i + 1.0;
		
		// Attempt to add connections between involved vertices
		for (size_t j = i; j < nv; j++)
			sa_add_vertex_to_net(s, n, s->vertices[j]);
		
		// Make sure the list of vertices in the net is correct
		for (size_t j = 0; j < nv; j++) {
			bool seen = false;
			for (size_t k = 0; k < n->num_vertices; k++) {
				if (s->vertices[j] == n->vertices[k]) {
					ck_assert(!seen);
					seen = true;
				}
			}
			if (j < i)
				ck_assert(!seen);
			else
				ck_assert(seen);
		}
	}
	
	// Check that all vertices now list the nets they are involved with (and no
	// others)
	for (size_t i = 0; i < nv; i++) {
		sa_vertex_t *v = s->vertices[i];
		
		for (size_t j = 0; j < nn; j++) {
			bool seen = false;
			for (size_t k = 0; k < v->num_nets; k++) {
				if (s->nets[j] == v->nets[k]) {
					ck_assert(!seen);
					seen = true;
				}
			}
			if (j > i)
				ck_assert(!seen);
			else
				ck_assert(seen);
		}
	}
	
	// Clean up
	sa_free(s);
}
END_TEST


Suite *
make_sa_state_suite(void)
{
	Suite *s = suite_create("sa_state");
	
	// Add tests to the test case
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, test_constructors);
	
	// Add each test case to the suite
	suite_add_tcase(s, tc_core);
	
	return s;
}
