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

void gfxDrawLoader( coreInfo &info, int Loop)
{

    auto _gfxDrawLoader = [&](float& ticker) -> void
    {
        ticker += 0.4f;
        float twopi = M_2PI;

        if (ticker > twopi)  ticker -= twopi;

        float newticker = ticker + 0.4f;
        for (; newticker < ticker + twopi; newticker += 0.4f)
        {
            for (float i = 0.0f; i < 5.0f; i += 0.1f)
            {
                float circum = twopi * i, tinc = twopi / circum;
                for (float j = 0.0f; j <= twopi; j += tinc)
                {
                    float fx = info.iCWidth + cosf(j) * i;
                    fx += cosf(newticker) * 5.0f;
                    float fy = info.iCHeight + sinf(j) * i;
                    fy += sinf(newticker) * 5.0f;

                    int ix = (int)floor(fx);
                    int iy = (int)floor(fy);

                    float c = newticker / (ticker + twopi);
                    info.pBuffer[iy * info.iWidth + ix] = gfxRGB(c, c, c);
                }
            }
        }
    };

    float lticker = 0.0f;
    int loop = 0;
    while (loop < Loop)
    {
        _gfxDrawLoader(lticker);
        loop += lticker > 5.9 ? 1 : 0;
        output.flipBuffers();
        Sleep(10);
    }
}

size_t gfxGenShip( array::list<vertex> &output, float &radius, float detail, vertex& Shipcol, vertex& Shildcol)  // what = 0 -> none 1, -> ship, 2 = shild, 3 = ship & shild
{
    size_t start = output.size();

    vertex pt;
    float y2 = radius / 2.5f;
    float y1 = y2 - radius;

    if (Shipcol.r > 0.0f)
    {
        pt = Shipcol;
        for (float i = 0.0f; i <= radius; i += detail)
        {
            pt.x = i;                                               /*          / \ < this */
            pt.y = y1 + i;                                          /*          --         */
            output.push_back(pt);
            pt.x = -pt.x;                                           /*   this > / \        */
            output.push_back(pt);                                   /*          --         */

                                                                    /*          / \        */
            pt.y = y2;                                              /*   this > --         */
            output.push_back(pt);
            pt.x = -pt.x;                                           /*          / \        */
            output.push_back(pt);                                   /*          --  < this */
        }
    }

    if (Shildcol.r > 0.0)
    {
        radius += 0.5f;
        pt = Shildcol;
        pt.z = 0.0f;

        for (float radi = 0.0f; radi < 0.2; radi += 0.1f)
        {
            for (float radj = 0.0f; radj < M_2PI; radj += detail)
            {
                // use the cosine of the semi-circle as the radius
                pt.x = cosf(radj) * (radius - radi);
                pt.y = sinf(radj) * (radius - radi);
                output.push_back(pt);

                // flip y to mirror the half sphere
                // flip x to reduce artifacts
                pt.x = -pt.x;
                pt.y = -pt.y;
                output.push_back(pt);
            }
        }
    }

    return output.size() - start;
}

void gfxGenItem(array::list<vertex>& output, float len, float detail, vertex& col)
{
    float x1 = -len / 2.0f;
    float y1 = x1;

    for (float i = 0.0f; i <= len; i += detail)
    {
        col.x = x1 + i;
        col.y = y1;
        output.push_back(col);
        col.y = -y1;
        output.push_back(col);

        col.x = x1;
        col.y = y1 + i;
        output.push_back(col);
        col.x = -x1;
        output.push_back(col);
    }
}


size_t gfxGenItemFire(array::list<vertex>& output, float len, float detail, vertex& colour)
{
    size_t start = output.size();

    gfxGenItem(output, len, detail, colour);

    len = 0.4f;
    colour.SetColor(0.3f, 0.0f, 0.0f);
    gfxGenAsteroid(output, len, 4.0f, colour);

    return output.size() - start;
}

size_t gfxGenItemFireGun(array::list<vertex>& output, float len, float detail, vertex& colour)
{
    size_t start = output.size();

    gfxGenItem(output, len, detail, colour);

    len = 0.3f;
    colour.SetColor(0.3f, 0.0f, 0.0f);
    colour.x = 0.5f;
    colour.y = 0.5f;
    gfxGenAsteroid(output, len, 3.0f, colour);
    colour.x = -0.5f;
    colour.y = -0.5f;
    gfxGenAsteroid(output, len, 3.0f, colour);

    return output.size() - start;
}



size_t gfxGenItemShild(array::list<vertex>& output, float len, float detail, vertex& colour)
{
    size_t start = output.size();

    vertex col;
    gfxGenItem(output, len, detail, colour);

    colour.SetColor(-1.0f, 0.0f, 0.0f);
    col.SetColor(1.0f, 1.0f, 0.0f);

    len = (len / 2) - 1.0f;
    gfxGenShip(output, len, 0.08f, colour, col);

    return output.size() - start;
}

size_t gfxGenItemShip(array::list<vertex>& output, float len, float detail, vertex& colour)
{
    size_t start = output.size();

    vertex col;
    gfxGenItem(output, len, detail, colour);

    colour.SetColor(1.0f, 0.0f, 1.0f);
    col.SetColor(-1.0f, 0.0f, 0.0f);

    len = (len / 2);
    gfxGenShip(output, len, 0.08f, colour, col);

    return output.size() - start;
}


size_t gfxGenAsteroid( array::list<vertex> &output, float radius, float detail, vertex &colour )
{
    size_t start = output.size();

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
            float x = cosf(radj+roffset) * cosf(radi+roffset) * (radius+roffset);
            float y = sinf(radi+roffset) * (radius+roffset);
            pt.x = colour.x + x;
            pt.y = colour.y + y;
            pt.z = sinf(radj+roffset) * cosf(radi+roffset) * (radius+roffset);
            output.push_back(pt);

            // flip y to mirror the half sphere
            // flip x to reduce artifacts
            pt.x = -x + colour.x;
            pt.y = -y + colour.y;
            output.push_back(pt);
        }
    }

    return output.size() - start;
}

void gfxBlinkStars( coreInfo &core )
{
    array::block<vertex>::iterator i = core.models.stars.pBegin;

    for ( ; i != core.models.stars.pEnd; ++i )
        i->r = i->g = i->b = frand();
}

size_t gfxGenStars( array::list<vertex> &output, coreInfo &core)
{
    size_t start = output.size();

    int num = static_cast<int>(1.65e-4f * core.iWidth * core.iHeight);
    for ( int i = 0; i < num; ++i )
    {
        float xpos = (0.5f - frand()) * core.fSWidth * 2;
        float ypos = (0.5f - frand()) * core.fSHeight * 2;
        float colour = frand();

        vertex star( xpos, ypos, 0.0f, colour, colour, colour );
        output.push_back(star);
    }

    return output.size() - start;
}
