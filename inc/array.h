#include <list>

// ARRAY DATA STRUCTURE DECLERATIONS
// ------------------------------------------------------------------
namespace array
{
    template <class T> class matrix;
    template <class T> class block;
    template <class T>
    using list = std::list<T>;
}

// ------------------------------------------------------------------
// matrix object. encapsulates initialization and multiplication
// ------------------------------------------------------------------
template <class T>
class array::matrix
{
public:
    // operators
    inline T& operator []( int i ) { return mbuf[i]; }
    inline void operator =( const matrix &in );
    inline matrix operator *( const matrix &in );

    // ctor/dtor
    inline matrix( void ) { reset(); }
    inline matrix( const matrix &src ) { *this = src; }
    inline matrix( float x, float y, float z ) { set( x, y, z ); }

    // initializers
    inline void reset( void );
    inline void set( float x, float y, float z );

    // angle alteration
    //rotate( float &x, float &y, float &z );

private:
    // 3x3 linear array
    static constexpr int siy = 3, six = 3;
    static constexpr int lixy=siy*six;
    T mbuf[lixy];
};

// ------------------------------------------------------------------
// matrix object: assignment operator - duplicate matrix
// ------------------------------------------------------------------
template <class T>
inline void array::matrix<T>::operator =( const matrix &src )
{
   memcpy(&mbuf, &src.mbuf, sizeof(mbuf));
}

// ------------------------------------------------------------------
// matrix object: mul operator - encapsulate matrix multiplication
// ------------------------------------------------------------------
template <class T>
inline array::matrix<T> array::matrix<T>::operator *( const matrix &in )
{
    matrix<T> out;

    // my optimized matrix multiplication
    // for(i)for(j)for(k) m[i][j] += a[i][k]*b[k][j];
    T *mp = out.mbuf;
    const T *ap = mbuf, *ape = ap + lixy;
    for ( ; ap != ape; ap+=siy )
    {
        const T *bp = in.mbuf, *bpe = bp+six;
        for ( ; bp != bpe; ++mp, ++bp )
        {
            *mp = 0.0f;
            const T *atp = ap,
                *btp = bp;

            *mp += *atp * *btp;
            btp += six; ++atp;

            *mp += *atp * *btp;
            btp += six; ++atp;

            *mp += *atp * *btp;
            btp += six; ++atp;
        }
    }

    // function inlined, so copy should not be invoked
    return out;
}

// ------------------------------------------------------------------
// matrix object: reset - initialize to kronecker delta
// ------------------------------------------------------------------
template <class T>
inline void array::matrix<T>::reset( void )
{
    // [ 1  0  0 ]
    // [ 0  1  0 ]
    // [ 0  0  1 ]
    T *pm = mbuf;
    for ( int i = 0; i < siy; ++i )
    {
        for ( int j = 0; j < six; ++j, ++pm )
        {
            if ( i == j )
                *pm = 1.0f;
            else
                *pm = 0.0f;
        }
    }
}

// ------------------------------------------------------------------
// matrix object: set - initialize and combine rotation matrices
// ------------------------------------------------------------------
template <class T>
inline void array::matrix<T>::set( float x, float y, float z )
{
    // initialize X rotation matrix
    matrix<T> mx;
    mx[4] = cosf(x);  mx[5] = -sinf(x);
    mx[7] = sinf(x);  mx[8] = cosf(x);

    // initialize Y rotation matrix
    matrix<T> my;
    my[0] = cosf(y);  my[2] = sinf(y);
    my[6] = -sinf(y); my[8] = cosf(y);

    // initialize Z rotation matrix
    matrix<T> mz;
    mz[0] = cosf(z);  mz[1] = -sinf(z);
    mz[3] = sinf(z);  mz[4] = cosf(z);

    // multiply matrices and copy result
    *this = mx * my * mz;
}

// ------------------------------------------------------------------
// block object. allows conversion between list and fixed sized array
// ------------------------------------------------------------------
template <class T>
class array::block
{
public:
    // members types
    typedef size_t size_type;
    typedef T* iterator;

    // ctor/dtor
    block();
    bool init( list<T> &source );
    ~block( void ) { delete [] pBegin; }

    // member accessors
    size_type size( void ) { return blockSize; }
    iterator begin( void ) { return pBegin; }
    iterator end( void ) { return pEnd; }

private:
    // array size and pointers
    size_type blockSize;
    iterator pBegin, pEnd;
};

// ------------------------------------------------------------------
// block object: constructor - allocate entire array block
// ------------------------------------------------------------------
template <class T>
array::block<T>::block()
{
    pEnd = pBegin = nullptr;
    blockSize = 0;
}

// ------------------------------------------------------------------
// block object: constructor - convert from list to block
// ------------------------------------------------------------------
template <class T>
bool array::block<T>::init( list<T> &source )
{
    // retrieve block size from list
    blockSize = source.size();

    // allocate array block
    pBegin = new T[blockSize];
    if ( !pBegin )
        return false;

    // set end pointer
    pEnd = pBegin + blockSize;

    // copy each item from the source into the block
    typename list<T>::iterator i = source.begin();
    for ( iterator j = begin(); j != end(); ++j, ++i )
        *j = *i;

    // success
    return true;
}
