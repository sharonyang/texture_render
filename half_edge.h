#ifndef __half_edge_h__
#define __half_edge_h__

#include "data.h"
#include <vector>
#include <map>

namespace half_edge
{
    void faces_to_edges(vector<face *> * faces, \
        vector< half_edge::edge_t >& edges_out);

class half_edge_t
{
public:
    // -1 for invalid index
    typedef long index_t;
    
    struct halfedge_t
    {
        index_t to_vertex;
        index_t face;
        index_t edge;
        index_t opposite_he;
        index_t next_he;
        
        halfedge_t() :
            to_vertex(-1),
            face(-1),
            edge(-1),
            opposite_he(-1),
            next_he(-1)
            {}
    };
    
    // We have edges array with face array -> build
    // half edge structure.
    void build_he_structure(const unsigned long num_vertices, \
        vector<face *> * faces, const unsigned long num_edges, \
        const half_edge::edge_t* edges);
    
    void clear()
    {
        m_halfedges.clear();
        m_vertex_halfedges.clear();
        m_face_halfedges.clear();
        m_edge_halfedges.clear();
        m_directed_edge2he_index.clear();
    }
    
    void vertex_face_neighbors(const index_t vertex_index, \
        vector< index_t >& result) const
    {
        result.clear();
        
        const index_t start_hei = m_vertex_halfedges[ vertex_index ];
        index_t hei = start_hei;
        while( true )
        {
            const halfedge_t& he = m_halfedges[ hei ];
            if( -1 != he.face ) result.push_back( he.face );
            
            hei = m_halfedges[ he.opposite_he ].next_he;
            if( hei == start_hei ) break;
        }
    }

    vector< index_t > vertex_face_neighbors(const index_t vertex_index) const
    {
        vector< index_t > result;
        vertex_face_neighbors( vertex_index, result );
        return result;
    }
    
    bool vertex_is_boundary( const index_t vertex_index ) const
    {
        return -1 == m_halfedges[ m_vertex_halfedges[ vertex_index ] ].face;
    }
        
private:
    // vector of half_edges
    vector< halfedge_t > m_halfedges;
    // for each vertex -> its half_edge.
    vector< index_t > m_vertex_halfedges;
    // for each face -> its half_edge.
    vector< index_t > m_face_halfedges;
    // for each edge -> its half_edge.
    vector< index_t > m_edge_halfedges;

    // ordered_edge -> half_edge index
    typedef map< pair< index_t, index_t >, index_t > mapping;
    mapping m_directed_edge2he_index;
};

}

#endif
