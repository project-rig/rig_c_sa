/**
 * Test the various sa_state_t datastructure manipulation functions.
 */

#include <check.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
 * Check the sa_subtract_resources function does as it says on the tin...
 */
START_TEST (test_subtract_resources)
{
	// Create two arrays of resource quantities with one-too-many resources which
	// should be left unmodifed
	int *a = calloc(nr + 1, sizeof(int));
	ck_assert(a);
	int *b = calloc(nr + 1, sizeof(int));
	ck_assert(b);
	
	for (size_t i = 0; i < nr + 1; i++) {
		a[i] = i + 1;
		b[i] = i;
	}
	
	sa_subtract_resources(s, a, b);
	
	// Make sure answer is correct and b is unmodified
	for (size_t i = 0; i < nr; i++) {
		ck_assert_int_eq(a[i], 1);
		ck_assert_int_eq(b[i], i);
	}
	
	// Make sure the extra values are untouched
	ck_assert_int_eq(a[nr], nr + 1);
	ck_assert_int_eq(b[nr], nr);
	
	free(a);
	free(b);
}
END_TEST

/**
 * Check the sa_add_resources function does as it says on the tin...
 */
START_TEST (test_add_resources)
{
	// Create two arrays of resource quantities with one-too-many resources which
	// should be left unmodifed
	int *a = calloc(nr + 1, sizeof(int));
	ck_assert(a);
	int *b = calloc(nr + 1, sizeof(int));
	ck_assert(b);
	
	for (size_t i = 0; i < nr + 1; i++) {
		a[i] = i + 1;
		b[i] = i;
	}
	
	sa_add_resources(s, a, b);
	
	// Make sure answer is correct and b is unmodified
	for (size_t i = 0; i < nr; i++) {
		ck_assert_int_eq(a[i], (i * 2) + 1);
		ck_assert_int_eq(b[i], i);
	}
	
	// Make sure the extra values are untouched
	ck_assert_int_eq(a[nr], nr + 1);
	ck_assert_int_eq(b[nr], nr);
	
	free(a);
	free(b);
}
END_TEST

/**
 * Check the sa_positive_resources function does as it says on the tin...
 */
START_TEST (test_positive_resources)
{
	// Create an arrays of resource quantities with one-too-many resources (to
	// ensure the extra one is ignored).
	int *a = calloc(nr + 1, sizeof(int));
	ck_assert(a);
	
	// Check case where all are zero
	for (size_t i = 0; i < nr; i++)
		a[i] = 0;
	a[nr] = +1; ck_assert(sa_positive_resources(s, a));
	a[nr] = -1; ck_assert(sa_positive_resources(s, a));
	
	// Check case where all are positive, non-zero
	for (size_t i = 0; i < nr; i++)
		a[i] = 100;
	a[nr] = +1; ck_assert(sa_positive_resources(s, a));
	a[nr] = -1; ck_assert(sa_positive_resources(s, a));
	
	// Check case where one is negative
	for (size_t i = 0; i < nr; i++)
		a[i] = 1;
	a[0] = -1;
	a[nr] = +1; ck_assert(!sa_positive_resources(s, a));
	a[nr] = -1; ck_assert(!sa_positive_resources(s, a));
	
	// Check case where all are negative
	for (size_t i = 0; i < nr; i++)
		a[i] = -1;
	a[nr] = +1; ck_assert(!sa_positive_resources(s, a));
	a[nr] = -1; ck_assert(!sa_positive_resources(s, a));
	
	free(a);
}
END_TEST

/**
 * Check the sa_add_vertices_to_chip function does as it says on the tin...
 */
START_TEST (test_add_vertices_to_chip)
{
	// Create a set of vertices in a linked list
	for (size_t i = 0; i < nv; i++) {
		// Create the vertex
		sa_vertex_t *v = sa_new_vertex(s, 0);
		ck_assert(v);
		s->vertices[i] = v;
		for (size_t j = 0; j < nr; j++)
			v->vertex_resources[j] = 1;
		
		// Add it to the linked-list
		if (i > 0)
			s->vertices[i - 1]->next = v;
	}
	
	// Make sure suffucuent resources are available on the target chip: 0, 1
	for (size_t i = 0; i < nr; i++)
		SA_STATE_CHIP_RESOURCES(s, 0, 1, i) = nv + 1;
	
	// Add the vertices to chip 0, 1
	sa_add_vertices_to_chip(s, s->vertices[0], 0, 1);
	
	// Make sure the resources remaining are correct
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), 1);
}
END_TEST

/**
 * Check the sa_add_vertices_to_chip_if_fit function does as it says on the tin...
 */
START_TEST (test_add_vertices_to_chip_if_fit)
{
	// Create a set of vertices in a linked list
	for (size_t i = 0; i < nv; i++) {
		// Create the vertex
		sa_vertex_t *v = sa_new_vertex(s, 0);
		ck_assert(v);
		s->vertices[i] = v;
		for (size_t j = 0; j < nr; j++)
			v->vertex_resources[j] = 1;
		
		// Add it to the linked-list
		if (i > 0)
			s->vertices[i - 1]->next = v;
	}
	
	// Make sure insuffucuent resources are available on the target chip: 0, 1
	for (size_t i = 0; i < nr; i++)
		SA_STATE_CHIP_RESOURCES(s, 0, 1, i) = nv - 1;
	
	// Attempt to add the vertices to chip 0, 1 (which should fail)
	ck_assert(!sa_add_vertices_to_chip_if_fit(s, s->vertices[0], 0, 1));
	ck_assert(!SA_STATE_CHIP_VERTICES(s, 0, 1));
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), nv - 1);
	
	// Now ensure suffucuent resources are available
	for (size_t i = 0; i < nr; i++)
		SA_STATE_CHIP_RESOURCES(s, 0, 1, i) = nv + 1;
	
	// Attempt to add the vertices to chip 0, 1 (which should fail)
	ck_assert(sa_add_vertices_to_chip_if_fit(s, s->vertices[0], 0, 1));
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 1) == s->vertices[0]);
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), 1);
}
END_TEST


/**
 * Check the sa_remove_vertex_from_chip function does as it says on the tin...
 */
START_TEST (test_remove_vertices_from_chip)
{
	// Make sure suffucuent resources are available on the target chip: 0, 0
	for (size_t i = 0; i < nr; i++)
		SA_STATE_CHIP_RESOURCES(s, 0, 1, i) = nv + 1;
	
	// Add a set of vertices to a chip
	for (size_t i = 0; i < nv; i++) {
		// Create the vertex
		sa_vertex_t *v = sa_new_vertex(s, 0);
		ck_assert(v);
		s->vertices[i] = v;
		for (size_t j = 0; j < nr; j++)
			v->vertex_resources[j] = 1;
		sa_add_vertex_to_chip(s, v, 0, 1, true);
	}
	
	int expected_resources = 1;
	
	// Remove a vertex inserted at some point in the middle (and thus won't be
	// the first or last vertex in the linked list)
	sa_remove_vertex_from_chip(s, s->vertices[1]);
	expected_resources++;
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), expected_resources);
	
	// Remove the first vertex added (which will be the first or last vertex in
	// the linked list)
	sa_remove_vertex_from_chip(s, s->vertices[0]);
	expected_resources++;
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), expected_resources);
	
	// Remove the last vertex added (which will be the first or last vertex in
	// the linked list)
	sa_remove_vertex_from_chip(s, s->vertices[nv - 1]);
	expected_resources++;
	for (size_t i = 0; i < nr; i++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), expected_resources);
	
	// Remove all other vertices (to make sure we can remove the last vertex from
	// a chip.
	while (SA_STATE_CHIP_VERTICES(s, 0, 1)) {
		sa_remove_vertex_from_chip(s, SA_STATE_CHIP_VERTICES(s, 0, 1));
		expected_resources++;
		for (size_t i = 0; i < nr; i++)
			ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 1, i), expected_resources);
	}
	
	// Once all vertices are removed the resources remaining on the chip should
	// match the quantity there at the beginning.
	ck_assert_int_eq(expected_resources, nv + 1);
}
END_TEST

/**
 * Check the sa_get_random_movable_vertex function does as it says on the tin...
 */
START_TEST (test_get_random_movable_vertex)
{
	// Make sure suffucuent resources are available on the target chip: 0, 0
	for (size_t i = 0; i < nr; i++)
		SA_STATE_CHIP_RESOURCES(s, 0, 1, i) = nv + 1;
	
	// Add a set of vertices to a chip of which the last will not be movable
	s->num_movable_vertices = nv - 1;
	for (size_t i = 0; i < nv; i++) {
		// Create the vertex
		sa_vertex_t *v = sa_new_vertex(s, 0);
		ck_assert(v);
		s->vertices[i] = v;
		for (size_t j = 0; j < nr; j++)
			v->vertex_resources[j] = 1;
		bool movable = i != nv - 1;
		sa_add_vertex_to_chip(s, v, 0, 1, movable);
	}
	
	// Pick a large number of random vertices and make sure all movable vertices
	// are picked (and non-movable ones don't!)
	bool *picked = calloc(nv, sizeof(bool));
	for (size_t sample = 0; sample < nv * 100; sample++) {
		sa_vertex_t *v = sa_get_random_movable_vertex(s);
		for (size_t i = 0; i < nv; i++)
			if (v == s->vertices[i])
				picked[i] = true;
	}
	for (size_t i = 0; i < nv; i++) {
		bool movable = i != nv - 1;
		ck_assert(picked[i] == movable);
	}
	
	free(picked);
}
END_TEST

/**
 * Check the sa_get_random_nearby_chip function does as it says on the tin...
 */
START_TEST (test_get_random_nearby_chip)
{
	// Create a new sa_state_t for this test since we need control over the
	// system size.
	size_t w = 8;
	size_t h = 4;
	sa_state_t *s = sa_new(w, h, nr, nv, nn);
	ck_assert(s);
	
	// Count the number of times particular chips are picked
	size_t hits[w][h];
	void _clear_hits(void) {
		for (size_t x = 0; x < w; x++)
			for (size_t y = 0; y < h; y++)
				hits[x][y] = 0;
	}
	
	
	// Run the function many times, sanity checking and recording the output
	void _take_samples(int ox, int oy, int distance_limit) {
		_clear_hits();
		
		for (size_t i = 0; i < 500; i++) {
			int x = -1;
			int y = -1;
			sa_get_random_nearby_chip(s, ox, oy, distance_limit, &x, &y);
			
			// Sanity check: In range
			ck_assert_int_ge(x, 0);
			ck_assert_int_lt(x, w);
			ck_assert_int_ge(y, 0);
			ck_assert_int_lt(y, h);
			
			// Sanity check: Not the original chip
			ck_assert(x != ox || y != oy);
			
			// Record the visit
			hits[x][y]++;
		}
	}
	
	// Check limited range
	s->has_wrap_around_links = false;
	_take_samples(4, 2, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (3 <= x && x <= 5 &&
			                               1 <= y && y <= 3 &&
			                               (x != 4 || y != 2)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	s->has_wrap_around_links = true;
	_take_samples(4, 2, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (3 <= x && x <= 5 &&
			                               1 <= y && y <= 3 &&
			                               (x != 4 || y != 2)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check non-wrapping left and bottom bounds
	s->has_wrap_around_links = false;
	_take_samples(0, 0, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (x <= 1 && y <= 1 && (x != 0 || y != 0)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check non-wrapping right and top bounds
	s->has_wrap_around_links = false;
	_take_samples(7, 3, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (6 <= x && 2 <= y && (x != 7 || y != 3)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check non-wrapping ranges which cover the whole Y-axis but not whole X
	s->has_wrap_around_links = false;
	_take_samples(4, 2, 2);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (2 <= x && x <= 6 &&
			                               (x != 4 || y != 2)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check non-wrapping range which exactly fills whole thing
	s->has_wrap_around_links = false;
	_take_samples(4, 2, 4);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (x != 4 || y != 2),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check wrapping left and bottom bounds
	s->has_wrap_around_links = true;
	_take_samples(0, 0, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == ((x <= 1 || 7 <= x) &&
			                               (y <= 1 || 3 <= y) &&
			                               (x != 0 || y != 0)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check wrapping right and top bounds
	s->has_wrap_around_links = true;
	_take_samples(7, 3, 1);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == ((x <= 0 || 6 <= x) &&
			                               (y <= 0 || 2 <= y) &&
			                               (x != 7 || y != 3)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check wrapping which (over) fills Y-axis but not X axis
	s->has_wrap_around_links = true;
	_take_samples(4, 3, 2);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (2 <= x && x <= 6 &&
			                               (x != 4 || y != 3)),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	// Check wrapping which fills everything
	s->has_wrap_around_links = true;
	_take_samples(4, 3, 4);
	for (size_t x = 0; x < w; x++)
		for (size_t y = 0; y < h; y++)
			ck_assert_msg(!!hits[x][y] == (x != 4 || y != 3),
			              "%d hits unexpectedly on %d,%d\n", hits[x][y], x, y);
	
	sa_free(s);
}
END_TEST

/**
 * Check the sa_make_room_on_chip function does as it says on the tin...
 */
START_TEST (test_make_room_on_chip)
{
	size_t nv = 4;
	size_t nr = nv; // Must be double for this test
	sa_state_t *s = sa_new(w, h, nr, nv, nn);
	ck_assert(s);
	
	// Pointer which will be set to point at removed vertices
	sa_vertex_t *removed_vertices;
	
	// Resource requirement array used throughout
	int resources_required[nr];
	
	// Special case: a dead chip always fails, even if we don't want any
	// resources anyway!
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = -1;
		resources_required[r] = 0;
	}
	ck_assert(!sa_make_room_on_chip(s, 0, 0, resources_required,
	                                &removed_vertices));
	ck_assert(!removed_vertices);
	
	// Special case: If no-resources required, always succeed
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 0;
	}
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(!removed_vertices);
	
	// Special case: If exact resources required already available, always
	// succeed
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = r;
		resources_required[r] = r;
	}
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(!removed_vertices);
	
	// Place some vertices on 0, 0. Vertex i uses 1 unit of resource (i % nr).
	for (size_t i = 0; i < nv; i++) {
		sa_vertex_t *v = sa_new_vertex(s, 0);
		ck_assert(v);
		s->vertices[i] = v;
		for (size_t r = 0; r < nr; r++)
			v->vertex_resources[r] = (i == r);
		sa_add_vertices_to_chip(s, v, 0, 0);
	}
	
	// Special case: If no-resources required, always succeed without removing
	// any vertices.
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 0;
	}
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(!removed_vertices);
	
	// Special case: If exact resources required already available, always
	// succeed without removing any vertices.
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = r;
		resources_required[r] = r;
	}
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(!removed_vertices);
	
	// Should be able to remove the first vertex from a chip when this is
	// sufficient.
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 0;
	}
	resources_required[nr - 1] = 1;
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-1]);
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(removed_vertices == s->vertices[nr - 1]);
	ck_assert(!removed_vertices->next);
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-2]);
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0)->next == s->vertices[nv-3]);
	for (size_t r = 0; r < nr; r++) {
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 0, r), (r == (nv - 1)));
	}
	
	// Put the vertex back again...
	sa_add_vertices_to_chip(s, removed_vertices, 0, 0);
	
	// The algorithm should greedily remove vertices until the resource
	// requirement is met
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 0;
	}
	resources_required[nr - 2] = 1;
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-1]);
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	ck_assert(removed_vertices == s->vertices[nr - 2]);
	ck_assert(removed_vertices->next == s->vertices[nr - 1]);
	ck_assert(!removed_vertices->next->next);
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-3]);
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0)->next == s->vertices[nv-4]);
	for (size_t r = 0; r < nr; r++) {
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 0, r), (r >= (nv - 2)));
	}
	
	// Put the vertices back again...
	sa_add_vertices_to_chip(s, removed_vertices, 0, 0);
	
	// The algorithm should greedily remove all vertices if required to meet the
	// demand.
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 0;
	}
	resources_required[0] = 1;
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-1]);
	ck_assert(sa_make_room_on_chip(s, 0, 0, resources_required,
	                               &removed_vertices));
	{
		sa_vertex_t *v = removed_vertices;
		size_t i = 0;
		while (v) {
			ck_assert(v == s->vertices[i]);
			v = v->next;
			i++;
		}
		ck_assert_int_eq(i, nv);
	}
	ck_assert(!SA_STATE_CHIP_VERTICES(s, 0, 0));
	for (size_t r = 0; r < nr; r++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 0, r), 1);
	
	// Put the vertices back again...
	sa_add_vertices_to_chip(s, removed_vertices, 0, 0);
	
	// If insufficient resources are available after removing all vertices,
	// everything should revert to how it was.
	for (size_t r = 0; r < nr; r++) {
		SA_STATE_CHIP_RESOURCES(s, 0, 0, r) = 0;
		resources_required[r] = 2;
	}
	ck_assert(SA_STATE_CHIP_VERTICES(s, 0, 0) == s->vertices[nv-1]);
	ck_assert(!sa_make_room_on_chip(s, 0, 0, resources_required,
	                                &removed_vertices));
	ck_assert(!removed_vertices);
	{
		sa_vertex_t *v = SA_STATE_CHIP_VERTICES(s, 0, 0);
		size_t i = 0;
		while (v) {
			ck_assert(v == s->vertices[i]);
			v = v->next;
			i++;
		}
		ck_assert_int_eq(i, nv);
	}
	for (size_t r = 0; r < nr; r++)
		ck_assert_int_eq(SA_STATE_CHIP_RESOURCES(s, 0, 0, r), 0);
	
	sa_free(s);
}
END_TEST


Suite *
make_sa_manipulation_suite(void)
{
	Suite *s = suite_create("sa_manipulation");
	
	// Add tests to the test case
	TCase *tc_core = tcase_create("Core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_subtract_resources);
	tcase_add_test(tc_core, test_add_resources);
	tcase_add_test(tc_core, test_positive_resources);
	tcase_add_test(tc_core, test_add_vertices_to_chip);
	tcase_add_test(tc_core, test_add_vertices_to_chip_if_fit);
	tcase_add_test(tc_core, test_remove_vertices_from_chip);
	tcase_add_test(tc_core, test_get_random_movable_vertex);
	tcase_add_test(tc_core, test_get_random_nearby_chip);
	tcase_add_test(tc_core, test_make_room_on_chip);
	
	// Add each test case to the suite
	suite_add_tcase(s, tc_core);
	
	return s;
}

