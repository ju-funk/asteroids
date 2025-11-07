// GRAPHICS ALGORITHM DECLERATIONS
// ------------------------------------------------------------------
const float M_PI = 3.1415926535897932384626433832795f;
const float M_2PI = 2 * M_PI;

struct vertex
{
    float x, y, z;
    float r, g, b;
};

// model points into main vertex array
struct model
{
    float scale; // radius used for dist test
    array::block<vertex>::size_type npoints;
    array::block<vertex>::iterator pBegin;
    array::block<vertex>::iterator pEnd;
};

// prototypes from gfx.cpp
inline float frand( void );
inline unsigned long int gfxRGB( float r, float g, float b );
bool gfxGenShip( array::list<vertex> &output, float detail );
bool gfxGenAsteroid( array::list<vertex> &output, float radius, float detail, vertex &colour );
bool gfxGenStars( array::list<vertex> &output, int num );
