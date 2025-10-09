// GRAPHICS ALGORITHM DECLERATIONS
// ------------------------------------------------------------------
const float M_PI = 3.1415926535897932384626433832795f;
const float M_2PI = 2 * M_PI;

struct vertex
{
    float x, y, z;
    float r, g, b;

    vertex(float X=0.0f, float Y=0.0f, float Z=0.0f, float R=0.0f, float G=0.0f, float B=0.0f)
    {
        SetColor(R, G, B);
        x = X;
        y = Y;
        z = Z;
    }

    void SetColor(float R, float G, float B)
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        r = R;
        g = G;
        b = B;
    }

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
size_t gfxGenShip( array::list<vertex> &output, float &radius, float detail, vertex& Shipcol, vertex& Shildcol);
size_t gfxGenAsteroid( array::list<vertex> &output, float radius, float detail, vertex &colour );
struct coreInfo;
size_t gfxGenStars( array::list<vertex> &output, coreInfo& core);
size_t gfxGenItemFire(array::list<vertex>& output, float len, float detail, vertex& colour);
size_t gfxGenItemShild(array::list<vertex>& output, float len, float detail, vertex& colour);
size_t gfxGenItemShip(array::list<vertex>& output, float len, float detail, vertex& colour);
size_t gfxGenItemFireGun(array::list<vertex>& output, float len, float detail, vertex& colour);

