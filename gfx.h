// GRAPHICS ALGORITHM DECLERATIONS
// ------------------------------------------------------------------
const float M_PI = 3.1415926535897932384626433832795f;
const float M_2PI = 2 * M_PI;

struct vertex
{
    float x, y, z;
    float r, g, b;

    template<typename T>
    inline vertex operator +(array::matrix<T>& in)
    {
        // rotate vertex using model matrix
        vertex pv = *this;
        pv.x = x * in[0] + y * in[1] + z * in[2];
        pv.y = x * in[3] + y * in[4] + z * in[5];
        pv.z = x * in[6] + y * in[7] + z * in[8];

        return pv;
    }
    
    inline vertex operator *=(float& in)
    {
        x *= in;
        y *= in;
        z *= in;

        return *this;
    }

    inline vertex operator +=(vertex& in)
    {
        x += in.x;
        y += in.y;
        z += in.z;

        return *this;
    }
};

// model points into main vertex array
struct model
{
    float scale; // radius used for dist test
    array::block<vertex>::size_type npoints;
    array::block<vertex>::iterator pBegin;
    array::block<vertex>::iterator pEnd;

    void Copy(array::block<vertex>::iterator Start)
    {
        pBegin = Start;
        pEnd   = Start + npoints;
    }

    void Sets(array::block<vertex>::size_type size, float Scale)
    {
        scale   = Scale;
        npoints = size;
    }
};

// prototypes from gfx.cpp
inline float frand( void );
inline unsigned long int gfxRGB( float r, float g, float b );
size_t gfxGenShip( array::list<vertex> &output, float &radius, float detail, bool shild );
size_t gfxGenAsteroid( array::list<vertex> &output, float radius, float detail, vertex &colour );
struct coreInfo;
size_t gfxGenStars( array::list<vertex> &output, coreInfo& core);
