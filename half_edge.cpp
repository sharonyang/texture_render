#include "half_edge.h"
#include <set>
#include <iostream>

namespace
{
typedef map< pair< half_edge::half_edge_t::index_t, \
    half_edge::half_edge_t::index_t >, half_edge::half_edge_t::index_t > \
    mapping;
half_edge::half_edge_t::index_t edge_to_face(const mapping& edge2face, \
    half_edge::half_edge_t::index_t vertex_i, \
    half_edge::half_edge_t::index_t vertex_j )
{
    mapping::const_iterator it = \
        edge2face.find(make_pair(vertex_i, vertex_j));
    
    // If no such edge exists, then there is no such edge
    // in the half edge structure... the edge must be a
    // a boundary edge.
    // In this case, the reverse orientation edge must have a face.
    if( it == edge2face.end() )
        return -1;
    
    return it->second;
}
}

namespace half_edge
{

/* build half edge structure from the list
 * of vertices and faces
 */
void half_edge_t::build_he_structure(const unsigned long num_vertices, \
    vector<face*> * faces, const unsigned long num_edges, const edge_t* edges)
{
    int num_faces = faces->size();
    mapping edge2face;
    for( int fi = 0; fi < num_faces; ++fi )
    {
        face * curr = (*faces)[fi];
        edge2face[ make_pair( curr->ind1, curr->ind2 ) ] = fi;
        edge2face[ make_pair( curr->ind2, curr->ind3 ) ] = fi;
        edge2face[ make_pair( curr->ind3, curr->ind1 ) ] = fi;
    }
    
    clear();
    m_vertex_halfedges.resize( num_vertices, -1 );
    m_face_halfedges.resize( num_faces, -1 );
    m_edge_halfedges.resize( num_edges, -1 );
    m_halfedges.reserve( num_edges*2 );
    
    for( int ei = 0; ei < num_edges; ++ei )
    {
        const edge_t& edge = edges[ei];
        
        // Add the halfedge_t structures to the end of the list.
        const index_t he0index = m_halfedges.size();
        m_halfedges.push_back( halfedge_t() );
        halfedge_t& he0 = m_halfedges.back();
        
        const index_t he1index = m_halfedges.size();
        m_halfedges.push_back( halfedge_t() );
        halfedge_t& he1 = m_halfedges.back();
        
        // The face will be -1 if it is a boundary half-edge.
        he0.face = edge_to_face( edge2face, edge.v[0], edge.v[1] );
        he0.to_vertex = edge.v[1];
        he0.edge = ei;
        
        // The face will be -1 if it is a boundary half-edge.
        he1.face = edge_to_face( edge2face, edge.v[1], edge.v[0] );
        he1.to_vertex = edge.v[0];
        he1.edge = ei;
        
        // Store the opposite half-edge index.
        he0.opposite_he = he1index;
        he1.opposite_he = he0index;
        
        // Also store the index in our m_directed_edge2he_index map.
        m_directed_edge2he_index[make_pair( edge.v[0], edge.v[1] )] = \
            he0index;
        m_directed_edge2he_index[make_pair( edge.v[1], edge.v[0] )] = \
            he1index;
        
        // If the vertex pointed to by a half-edge doesn't yet have an
        // out-going halfedge, store the opposite halfedge.
        // Also, if the vertex is a boundary vertex, make sure its
        // out-going halfedge is a boundary halfedge.
        if( m_vertex_halfedges[ he0.to_vertex ] == -1 || -1 == he1.face )
            m_vertex_halfedges[ he0.to_vertex ] = he0.opposite_he;
        if( m_vertex_halfedges[ he1.to_vertex ] == -1 || -1 == he0.face )
            m_vertex_halfedges[ he1.to_vertex ] = he1.opposite_he;
        
        // If the face pointed to by a half-edge doesn't yet have a
        // halfedge pointing to it, store the halfedge.
        if( he0.face != -1 && m_face_halfedges[ he0.face ] == -1 )
            m_face_halfedges[ he0.face ] = he0index;
        if( he1.face != -1 && m_face_halfedges[ he1.face ] == -1 )
            m_face_halfedges[ he1.face ] = he1index;
        
        // Store one of the half-edges for the edge.
        m_edge_halfedges[ ei ] = he0index;
    }
    
    // Now that all the half-edges are created, set the remaining
    // next_he field.
    // We can't yet handle boundary halfedges, so store them for later.
    vector< index_t > boundary_heis;
    for( int hei = 0; hei < m_halfedges.size(); ++hei )
    {
        halfedge_t& he = m_halfedges.at( hei );
        // Store boundary halfedges for later.
        if( -1 == he.face )
        {
            boundary_heis.push_back( hei );
            continue;
        }
        
        face * curr_face = (*faces)[ he.face ];
        const index_t i = he.to_vertex;
        
        index_t j = -1;
        if( curr_face->ind1 == i ) j = curr_face->ind2;
        else if( curr_face->ind2 == i ) j = curr_face->ind3;
        else if( curr_face->ind3 == i ) j = curr_face->ind1;
        
        he.next_he = m_directed_edge2he_index[ make_pair(i,j) ];
    }
    
    // Make a map from vertices to boundary halfedges (indices)
    // originating from them.
    map< index_t, set< index_t > > vertex2outgoing_boundary_hei;
    for( vector< index_t >::const_iterator hei = boundary_heis.begin(); \
        hei != boundary_heis.end(); ++hei )
    {
        const index_t originating_vertex = \
            m_halfedges[ m_halfedges[ *hei ].opposite_he ].to_vertex;
        vertex2outgoing_boundary_hei[ originating_vertex ].insert( *hei );
        if( vertex2outgoing_boundary_hei[ originating_vertex ].size() > 1 )
        {
            cerr << "Non-orientable surface.\n";
        }
    }
    
    // For each boundary halfedge, make its next_he one of the boundary
    // halfedges originating at its to_vertex.
    for( vector< index_t >::const_iterator hei = boundary_heis.begin(); \
        hei != boundary_heis.end(); hei++ )
    {
        halfedge_t& he = m_halfedges[ *hei ];
        
        set< index_t >& outgoing = \
            vertex2outgoing_boundary_hei[ he.to_vertex ];
        if( !outgoing.empty() )
        {
            set< index_t >::iterator outgoing_hei = outgoing.begin();
            he.next_he = *outgoing_hei;
            
            outgoing.erase( outgoing_hei );
        }
    }
}

void faces_to_edges( vector<face *> * faces, vector< edge_t >& edges_out )
{
    typedef set< pair< index_t, index_t > > edge_set_t;
    edge_set_t edges;
    for( int t = 0; t < faces->size(); t++ )
    {
        face * curr = (*faces)[t];
        edges.insert( make_pair( min( curr->ind1, curr->ind2 ), \
            max( curr->ind1, curr->ind2 ) ) );
        edges.insert( make_pair( min( curr->ind2, curr->ind3 ), \
            max( curr->ind2, curr->ind3 ) ) );
        edges.insert( make_pair( min( curr->ind3, curr->ind1 ), \
            max( curr->ind3, curr->ind1 ) ) );
    }
    
    edges_out.resize( edges.size() );
    int e = 0;
    for( edge_set_t::const_iterator it = edges.begin(); \
        it != edges.end(); it++, e++ )
    {
        edges_out.at(e).start() = it->first;
        edges_out.at(e).end() = it->second;
    }
}

}
