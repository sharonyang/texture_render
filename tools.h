#include <glm/glm.hpp>
#include "half_edge.h"

using namespace std;

void point2vector(point * pt, glm::vec3 &vert)
{
	vert.x = pt->x;
    vert.y = pt->y; 
    vert.z = pt->z;
}

float random_gen()
{
    float rand_num = float(rand()) / (RAND_MAX);
    return rand_num;
}

/* bump map by adding a random variable
 * to each component of the normal vector
 * then normalize
 */
void bump_map(glm::vec3 &vertex_normal)
{
	float rand_x = random_gen();
    float rand_y = random_gen();
    float rand_z = random_gen();

    vertex_normal.x += rand_x;
    vertex_normal.y += rand_y;
    vertex_normal.z += rand_z;

    vertex_normal = glm::normalize(vertex_normal);

}
