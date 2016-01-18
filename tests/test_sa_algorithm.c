/**
 * Test the SA algorithm functions.
 */

#include <check.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <math.h>

#include "tests.h"

#include "sa.h"


////////////////////////////////////////////////////////////////////////////////
// Test fixture
////////////////////////////////////////////////////////////////////////////////

// Define a 2x3 problem with 4 vertices to place with 3 nets between them and 2
// resource types. The test fixture does *not* create and connect the vertices
// and nets: this is the responsibility of the tester.
static const size_t w = 2; // Width
static const size_t h = 3; // Height
static const size_t nv = 4; // Number of vertices
static const size_t nn = 4; // Number of nets
static const size_t nr = 2; // Number of resource types

static sa_state_t *s = NULL;

static void setup(void) {
	s = sa_new(w, h, nr, nv, nn);
	ck_assert(s);
}

static void teardown(void) {
	// Clean up
	sa_free(s);
}

////////////////////////////////////////////////////////////////////////////////
// Tests
////////////////////////////////////////////////////////////////////////////////

/**
 * Check the sa_get_net_cost function does as it says on the tin for the
 * special case where we just have one vertex.
 */
START_TEST (test_get_net_cost_one_vertex)
{
	sa_net_t *n = sa_new_net(s, 1);
	ck_assert(n);
	s->nets[0] = n;
	n->weight = 2.0;
	
	sa_vertex_t *v = sa_new_vertex(s, 1);
	ck_assert(v);
	s->vertices[0] = v;
	sa_add_vertex_to_net(s, n, v);
	
	v->x = 1;
	v->y = 2;
	
	// Try with and without wraparound
	s->has_wrap_around_links = false;
	ck_assert(sa_get_net_cost(s, n) == 0.0);
	
	s->has_wrap_around_links = true;
	ck_assert(sa_get_net_cost(s, n) == 0.0);
}
END_TEST

/**
 * Check the sa_get_net_cost function does as it says on the tin for a more
 * interesting case.
 */
START_TEST (test_get_net_cost)
{
	// Custom size required
	sa_state_t *s = sa_new(20, 10, nr, 4, 1);
	ck_assert(s);
	
	sa_net_t *n = sa_new_net(s, 4);
	ck_assert(n);
	s->nets[0] = n;
	n->weight = 2.0;
	
	sa_vertex_t *v0 = sa_new_vertex(s, 1); ck_assert(v0); s->vertices[0] = v0;
	sa_vertex_t *v1 = sa_new_vertex(s, 1); ck_assert(v1); s->vertices[1] = v1;
	sa_vertex_t *v2 = sa_new_vertex(s, 1); ck_assert(v2); s->vertices[2] = v2;
	sa_vertex_t *v3 = sa_new_vertex(s, 1); ck_assert(v3); s->vertices[3] = v3;
	sa_add_vertex_to_net(s, n, v0);
	sa_add_vertex_to_net(s, n, v1);
	sa_add_vertex_to_net(s, n, v2);
	sa_add_vertex_to_net(s, n, v3);
	
	/* Set vertex positions. Note that:
	 * * Not in any order of x or y positions
	 * * Have an example of a vertex on edges of both dimensions
	 * * Have an example of a vertex on both extremes of edge
	 *
	 *          x
	 *      0 ----> 19
	 * ....................
	 * ...................3
	 * ...............1....  9
	 * .................... /|\
	 * ....................  |
	 * ....................  |  y
	 * ....................  |
	 * ....................  0
	 * ...2................
	 * ..0.................
	 */
	v0->x = 2;  v0->y = 0;
	v1->x = 15; v1->y = 7;
	v2->x = 3;  v2->y = 1;
	v3->x = 19; v3->y = 8;
	
	// Without wrap-around we have a 17x8 boundingbox at weight 2.0.
	s->has_wrap_around_links = false;
	ck_assert_msg(sa_get_net_cost(s, n) == (17.0 + 8.0) * 2.0,
	              "%f != %f", sa_get_net_cost(s, n), (17.0 + 8.0) * 2.0);
	
	// With wrap-around the bounding box wraps giving a 8x4 bounding box at
	// weight 2.0
	s->has_wrap_around_links = true;
	ck_assert_msg(sa_get_net_cost(s, n) == (8.0 + 4.0) * 2.0,
	              "%f != %f", sa_get_net_cost(s, n), (8.0 + 4.0) * 2.0);
	
	/* Set alternative vertex positions. Note that:
	 * * The wrapping and non-wrapping bounding box is the same
	 *
	 *          x
	 *      0 ----> 19
	 * ....................
	 * ....................
	 * ....................  9
	 * .......2.3.......... /|\
	 * ....................  |
	 * .......0.1..........  |  y
	 * ....................  |
	 * ....................  0
	 * ....................
	 * ....................
	 */
	v0->x = 7; v0->y = 4;
	v1->x = 9; v1->y = 4;
	v2->x = 7; v2->y = 6;
	v3->x = 9; v3->y = 6;
	
	// Without wrap-around we have a 2x2 boundingbox at weight 2.0.
	s->has_wrap_around_links = false;
	ck_assert_msg(sa_get_net_cost(s, n) == (2.0 + 2.0) * 2.0,
	              "%f != %f", sa_get_net_cost(s, n), (2.0 + 2.0) * 2.0);
	s->has_wrap_around_links = true;
	ck_assert_msg(sa_get_net_cost(s, n) == (2.0 + 2.0) * 2.0,
	              "%f != %f", sa_get_net_cost(s, n), (2.0 + 2.0) * 2.0);
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_get_swap_cost function does as it says on the tin...
 */
START_TEST (test_get_swap_cost)
{
	// In this example we want to swap vertices A and B to go from
	//
	//    C zzzzzzz D          C zzzzzzz D
	//      y     x            x         y
	//        y x        to    x         y
	//        x y              x         y
	//      x     y            x         y
	//    A wwwwwww B          B wwwwwww A
	//
	// All nets have unit weight.
	sa_state_t *s = sa_new(2, 2, nr, 4, 4);
	ck_assert(s);
	
	sa_net_t *nw = sa_new_net(s, 2); ck_assert(nw); s->nets[0] = nw; nw->weight = 1.0;
	sa_net_t *nx = sa_new_net(s, 2); ck_assert(nx); s->nets[1] = nx; nx->weight = 1.0;
	sa_net_t *ny = sa_new_net(s, 2); ck_assert(ny); s->nets[2] = ny; ny->weight = 1.0;
	sa_net_t *nz = sa_new_net(s, 2); ck_assert(nz); s->nets[3] = nz; nz->weight = 1.0;
	
	sa_vertex_t *va = sa_new_vertex(s, 2); ck_assert(va); s->vertices[0] = va;
	sa_vertex_t *vb = sa_new_vertex(s, 2); ck_assert(vb); s->vertices[1] = vb;
	sa_vertex_t *vc = sa_new_vertex(s, 2); ck_assert(vc); s->vertices[2] = vc;
	sa_vertex_t *vd = sa_new_vertex(s, 2); ck_assert(vd); s->vertices[3] = vd;
	
	sa_add_vertex_to_net(s, nw, va);
	sa_add_vertex_to_net(s, nx, va);
	sa_add_vertex_to_net(s, nw, vb);
	sa_add_vertex_to_net(s, ny, vb);
	sa_add_vertex_to_net(s, ny, vc);
	sa_add_vertex_to_net(s, nz, vc);
	sa_add_vertex_to_net(s, nx, vd);
	sa_add_vertex_to_net(s, nz, vd);
	
	va->x = 0; va->y = 0;
	vb->x = 1; vb->y = 0;
	vc->x = 0; vc->y = 1;
	vd->x = 1; vd->y = 1;
	
	// Nets x and y should have their cost reduced by 1 for a total cost saving
	// of 2.
	ck_assert(sa_get_swap_cost(s, 0, 0, va, 1, 0, vb) == -2.0);
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_step function fails when no chip can fit the vertex selected.
 */
START_TEST (test_step_no_free_chips)
{
	// In this example we have a 2x1 system with the only movable vertex on chip
	// 0,0 and no other chip with enough room for that vertex.
	sa_state_t *s = sa_new(2, 1, 1, 1, 1);
	ck_assert(s);
	s->num_movable_vertices = 1;
	
	sa_vertex_t *v = sa_new_vertex(s, 0);
	ck_assert(v);
	s->vertices[0] = v;
	v->vertex_resources[0] = 1;
	sa_add_vertex_to_chip(s, v, 0, 0, true);
	
	// Swapping is made impoossible because no chip has room
	SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) = 0;
	SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) = 0;
	
	// Since a random element is involved, should run multiple times...
	for (size_t i = 0; i < 10; i++) {
		double cost = NAN;
		bool swapped = sa_step(s, 1, 1e50, &cost);
		ck_assert(!swapped);
		ck_assert(cost == 0.0);
		
		// Vertex should be exactly where it was...
		ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == v);
		ck_assert(v->next == NULL);
		ck_assert(v->x == 0);
		ck_assert(v->y == 0);
		ck_assert(SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) == 0);
		ck_assert(SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) == 0);
	}
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_step function fails when the "vb" vertices selected for swap
 * don't fit on the original chip.
 */
START_TEST (test_step_not_enough_space_on_original_chip)
{
	// In this example we have a 2x1 system with two movable vertices and where
	// the vertex on 1,0 is too large to fit on 0,0 even when that chip is empty.
	sa_state_t *s = sa_new(2, 1, 1, 2, 1);
	ck_assert(s);
	s->num_movable_vertices = 2;
	
	sa_vertex_t *v0 = sa_new_vertex(s, 0); ck_assert(v0); s->vertices[0] = v0;
	sa_vertex_t *v1 = sa_new_vertex(s, 0); ck_assert(v1); s->vertices[1] = v1;
	v0->vertex_resources[0] = 1;
	v1->vertex_resources[0] = 2;
	sa_add_vertex_to_chip(s, v0, 0, 0, true);
	sa_add_vertex_to_chip(s, v1, 1, 0, true);
	
	// Swapping is made impoossible because the first chip cannot fit a vertex
	// with "2" resources.
	SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) = 0;
	SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) = 0;
	
	// Since a random element is involved, should run multiple times...
	for (size_t i = 0; i < 10; i++) {
		double cost = NAN;
		bool swapped = sa_step(s, 1, 1e50, &cost);
		ck_assert(!swapped);
		ck_assert(cost == 0.0);
		
		// Vertices should be exactly where they were...
		ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == v0);
		ck_assert(SA_STATE_CHIP_VERTICES(s, 1, 0) == v1);
		ck_assert(v0->next == NULL);
		ck_assert(v1->next == NULL);
		ck_assert(v0->x == 0);
		ck_assert(v0->y == 0);
		ck_assert(v1->x == 1);
		ck_assert(v1->y == 0);
		ck_assert(SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) == 0);
		ck_assert(SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) == 0);
	}
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_step function fails sometimes when the cost increases.
 */
START_TEST (test_step_bad_cost)
{
	// In this example we have a 2x1 system with 1 movable and 1 fixed vertex,
	// connected via a net, and initially on the same chip. The step function
	// should attempt to move the movable vertex onto 1,0 but only *sometimes*
	// accept this since it would increase the cost of the net.
	sa_state_t *s = sa_new(2, 1, 1, 2, 1);
	ck_assert(s);
	s->num_movable_vertices = 1;
	
	sa_vertex_t *v0 = sa_new_vertex(s, 1); ck_assert(v0); s->vertices[0] = v0;
	sa_vertex_t *v1 = sa_new_vertex(s, 1); ck_assert(v1); s->vertices[1] = v1;
	v0->vertex_resources[0] = 1;
	v1->vertex_resources[0] = 1;
	sa_add_vertex_to_chip(s, v0, 0, 0, true);
	sa_add_vertex_to_chip(s, v1, 0, 0, false);
	
	sa_net_t *n = sa_new_net(s, 2);
	ck_assert(n);
	s->nets[0] = n;
	n->weight = 1.0;
	sa_add_vertex_to_net(s, n, v0);
	sa_add_vertex_to_net(s, n, v1);
	
	// Swapping should be possible
	SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) = 0;
	SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) = 1;
	
	// Since a random element is involved, should run multiple times...
	size_t num_swapped = 0;
	size_t num_not_swapped = 0;
	for (size_t i = 0; i < 10; i++) {
		double cost = NAN;
		bool swapped = sa_step(s, 1, 2.0, &cost);
		
		if (swapped) {
			num_swapped++;
			
			// Cost should have increased
			ck_assert(cost == 1.0);
			
			// Vertices should have moved
			ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == NULL);
			ck_assert(SA_STATE_CHIP_VERTICES(s, 1, 0) == v0);
			ck_assert(v0->next == NULL);
			ck_assert(v1->next == NULL);
			ck_assert(v0->x == 1);
			ck_assert(v0->y == 0);
			ck_assert(v1->x == 0);
			ck_assert(v1->y == 0);
			ck_assert(SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) == 1);
			ck_assert(SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) == 0);
			
			// Put the vertex back for the next trail
			sa_remove_vertex_from_chip(s, v0);
			sa_add_vertex_to_chip(s, v0, 0, 0, true);
		} else {
			num_not_swapped++;
			
			// Vertices should be exactly where they were...
			ck_assert_msg(cost == 0.0, "%f == %f", cost, 0.0);
			ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == v0);
			ck_assert(SA_STATE_CHIP_VERTICES(s, 1, 0) == NULL);
			ck_assert(v0->next == NULL);
			ck_assert(v1->next == NULL);
			ck_assert(v0->x == 0);
			ck_assert(v0->y == 0);
			ck_assert(v1->x == 0);
			ck_assert(v1->y == 0);
			ck_assert(SA_STATE_CHIP_RESOURCES(s, 0, 0, 0) == 0);
			ck_assert(SA_STATE_CHIP_RESOURCES(s, 1, 0, 0) == 1);
		}
	}
	
	// Should have randomly swapped or not swapped a few times
	ck_assert(num_swapped > 0);
	ck_assert(num_not_swapped > 0);
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_run_steps function does its job. Far from a complete test.
 */
START_TEST (test_run_steps)
{
	// In this example we have a 4x4 system without wrap-around links and with 2
	// movable vertices, connected via a net, and initially on different chips on
	// opposite corners of the system. Each chip will have space for exactly one
	// vertex. We'll run the algorithm for a large number of steps and at high
	// and low temperatures. When high-temperature the cost deviation should be
	// high. When low temperature, the cost delta should be negative and the two
	// vertices should be placed on neighbouring chips.
	sa_state_t *s = sa_new(4, 4, 1, 2, 1);
	ck_assert(s);
	s->num_movable_vertices = 2;
	s->has_wrap_around_links = false;
	for (size_t x = 0; x < 4; x++)
		for (size_t y = 0; y < 4; y++)
			SA_STATE_CHIP_RESOURCES(s, x, y, 0) = 1;
	
	sa_vertex_t *v0 = sa_new_vertex(s, 1); ck_assert(v0); s->vertices[0] = v0;
	sa_vertex_t *v1 = sa_new_vertex(s, 1); ck_assert(v1); s->vertices[1] = v1;
	v0->vertex_resources[0] = 1;
	v1->vertex_resources[0] = 1;
	sa_add_vertex_to_chip(s, v0, 0, 0, true);
	sa_add_vertex_to_chip(s, v1, 3, 3, true);
	
	sa_net_t *n = sa_new_net(s, 2);
	ck_assert(n);
	s->nets[0] = n;
	n->weight = 1.0;
	sa_add_vertex_to_net(s, n, v0);
	sa_add_vertex_to_net(s, n, v1);
	
	// With high temperatures
	size_t num_accepted;
	double cost_delta;
	double cost_delta_sd;
	sa_run_steps(s, 1000, 4, 1e50, &num_accepted, &cost_delta, &cost_delta_sd);
	
	// Majority of swaps should be accepted (only those which try to place the
	// two vertices on the same chip will fail).
	ck_assert(num_accepted > 750);
	
	// The cost deviation should be notable
	ck_assert(cost_delta_sd > 1.0);
	
	// Restore the locations of the vertices
	sa_remove_vertex_from_chip(s, v0);
	sa_remove_vertex_from_chip(s, v1);
	sa_add_vertex_to_chip(s, v0, 0, 0, true);
	sa_add_vertex_to_chip(s, v1, 3, 3, true);
	
	// Now with low temperatures
	sa_run_steps(s, 1000, 4, 0.0, &num_accepted, &cost_delta, &cost_delta_sd);
	
	// Many swaps should be rejected since once the two vertices are close
	// together they should not be moved apart.
	ck_assert(num_accepted < 250);
	
	// The cost deviation should be small since once close they should stay close
	ck_assert(cost_delta_sd < 1.0);
	
	// The cost change overall should drop from 6 to 1.
	ck_assert_msg(cost_delta == -5.0, "%f == %f", cost_delta, -5.0);
	
	sa_free(s);
}
END_TEST



Suite *
make_sa_algorithm_suite(void)
{
	Suite *s = suite_create("sa_algorithm");
	
	// Add tests to the test case
	TCase *tc_core = tcase_create("Core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_get_net_cost_one_vertex);
	tcase_add_test(tc_core, test_get_net_cost);
	tcase_add_test(tc_core, test_get_swap_cost);
	tcase_add_test(tc_core, test_step_no_free_chips);
	tcase_add_test(tc_core, test_step_not_enough_space_on_original_chip);
	tcase_add_test(tc_core, test_step_bad_cost);
	tcase_add_test(tc_core, test_run_steps);
	
	// Add each test case to the suite
	suite_add_tcase(s, tc_core);
	
	return s;
}


