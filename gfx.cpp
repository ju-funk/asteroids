#include "main.h"
#include "core.h"

// return random value between 0.0f and 1.0f
inline float frand( void )
{
    float random = (float)rand();
    return random / RAND_MAX;
}

// combine RGB colour channels in unsigned long
inline unsigned long int gfxRGB( float r, float g, float b )
{
    unsigned long red = (unsigned long int)(r * 255);
    unsigned long green = (unsigned long int)(g * 255);
    unsigned long blue = (unsigned long int)(b * 255);

    unsigned long int result = (red<<16) | (green<<8) | blue;

    return result;
}

void gfxDrawLoader( coreInfo &info, float &ticker )
{
    ticker += 0.4f;
    float twopi = M_2PI;

    if ( ticker > twopi )  ticker -= twopi;

    float newticker = ticker + 0.4f;
    for ( ; newticker < ticker+twopi; newticker += 0.4f )
    {
        for ( float i = 0.0f; i < 5.0f; i += 0.1f )
        {
            float circum = twopi * i, tinc = twopi / circum;
            for ( float j = 0.0f; j <= twopi; j += tinc )
            {
                float fx = info.iCWidth + cosf(j)*i;
                fx += cosf(newticker) * 5.0f;
                float fy = info.iCHeight + sinf(j)*i;
                fy += sinf(newticker) * 5.0f;

                int ix = (int)floor(fx);
                int iy = (int)floor(fy);

                float c = newticker / (ticker+twopi);
                info.pBuffer[ iy * info.iWidth + ix ] = gfxRGB(c,c,c);
            }
        }
    }
}

bool gfxGenShip( array::list<vertex> &output, float detail )
{
    vertex pt;
    for ( float i = 0.0f; i <= 1.0f; i+=detail )
    {
        pt.x = i;
        pt.y = -0.6f + i;
        pt.z = 0.0f;
        pt.r = pt.g = pt.b = 1.0f;
        if ( !output.push_back(pt) )
            return false;
        pt.x = -pt.x;
        if ( !output.push_back(pt) )
            return false;

        pt.x = i;
        pt.y = 0.4f;
        if ( !output.push_back(pt) )
            return false;
        pt.x = -pt.x;
        if ( !output.push_back(pt) )
            return false;
    }

    return true;
}

bool gfxGenAsteroid( array::list<vertex> &output, float radius, float detail, vertex &colour )
{
    vertex pt;
    float radinc = M_PI / detail;
    // for each point arround a semi-circle
    for ( float radi = 0.0f; radi < M_PI; radi += radinc )
    {
        // for each point arround the circumference at that point
        float circ = M_2PI * fabsf(cosf(radi)) * detail, centinc = M_2PI / circ;
        for ( float radj = 0.0f; radj < M_2PI; radj += centinc )
        {
            // random radius displacement
            float roffset = (0.5f-frand())/5;

            // 2/4 = 0.5, thus, colour value must be 0.0f-0.5f to prevent overflow
            pt.r = colour.r + (1.0f-sinf(radi))/4.0f;
            pt.g = colour.g + (1.0f-sinf(radi))/4.0f;
            pt.b = colour.b + (1.0f-sinf(radi))/4.0f;

            // use the cosine of the semi-circle as the radius
            pt.x = cosf(radj+roffset) * cosf(radi+roffset) * (radius+roffset);
            pt.y = sinf(radi+roffset) * (radius+roffset);
            pt.z = sinf(radj+roffset) * cosf(radi+roffset) * (radius+roffset);
            if ( !output.push_back(pt) )
                return false;

            // flip y to mirror the half sphere
            // flip x to reduce artifacts
            pt.x = -pt.x;
            pt.y = -pt.y;
            if ( !output.push_back(pt) )
                return false;
        }
    }

    return true;
}

void gfxBlinkStars( coreInfo &core )
{
    array::block<vertex>::iterator i = core.models.stars.pBegin;

    for ( ; i != core.models.stars.pEnd; ++i )
    {
        vertex &star = *i;
        star.r = star.g = star.b = frand();
    }
    
}

bool gfxGenStars( array::list<vertex> &output, int num )
{
    for ( int i = 0; i < num; ++i )
    {
        float xpos = (0.5f - frand()) * 100.0f;
        float ypos = (0.5f - frand()) * 100.0f;
        float colour = frand();

        vertex star = { xpos, ypos, 0.0f, colour, colour, colour };
        if ( !output.push_back(star) )
            return false;
    }

    return true;
}
