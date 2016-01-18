"""Build the CFFI interface for the simulated annealing C library.

Standalone usage:

    $ python cffi_compile.py
"""

import os

from cffi import FFI
ffi = FFI()

source_dir = os.path.dirname(__file__)

ffi.set_source(
    "_sa",
    """
        #include <stdlib.h>
        #include "sa.h"
        
        double sa_get_total_cost(sa_state_t *state) {
            double total = 0.0;
            for (size_t i = 0; i < state->num_nets; i++)
                total += sa_get_net_cost(state, state->nets[i]);
            return total;
        }
    """,
    libraries=["m"],
    sources=[os.path.join(source_dir, "sa.c")],
    include_dirs=[source_dir],
    extra_compile_args=["--std=c99", "-O3"],
)

ffi.cdef("""
    // Set C random seed
    void srand(unsigned int seed);
    
    // Datastructures
    typedef struct sa_net sa_net_t;
    typedef struct sa_vertex sa_vertex_t;
    struct sa_net {
        double weight;
        ...;
    };
    struct sa_vertex {
        int x;
        int y;
        int *vertex_resources;
        ...;
    };
    typedef struct sa_state {
        bool has_wrap_around_links;
        sa_net_t **nets;
        size_t num_movable_vertices;
        sa_vertex_t **vertices;
        ...;
    } sa_state_t;
    
    // Constructors/distructors
    sa_state_t *sa_new(size_t width, size_t height, size_t num_resource_types,
                       size_t num_vertices, size_t num_nets);
    sa_vertex_t *sa_new_vertex(const sa_state_t *state, size_t num_nets);
    sa_net_t *sa_new_net(const sa_state_t *state, size_t num_vertices);
    void sa_free(sa_state_t *state);
    
    // Initialisation functions
    void sa_add_vertex_to_net(const sa_state_t *state, sa_net_t *net, sa_vertex_t *vertex);
    void sa_add_vertex_to_chip(sa_state_t *state, sa_vertex_t *vertex, int x, int y, bool movable);
    void sa_set_chip_resources(sa_state_t *state, size_t x, size_t y,
                               size_t resource, int value);
    
    // Algorithm kernel
    void sa_run_steps(sa_state_t *state, size_t num_steps, int distance_limit, double temperature,
                      size_t *num_accepted, double *cost_delta, double *cost_delta_sd);
    
    // Utility function
    double sa_get_total_cost(sa_state_t *state);
""")

if __name__ == "__main__":
    ffi.compile()

