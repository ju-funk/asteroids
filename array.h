// ARRAY DATA STRUCTURE DECLERATIONS
// ------------------------------------------------------------------
namespace array
{
    template <class T> class matrix;
    template <class T> class block;
    template <class T> class list;
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
    T mbuf[9];
};

// ------------------------------------------------------------------
// matrix object: assignment operator - duplicate matrix
// ------------------------------------------------------------------
template <class T>
inline void array::matrix<T>::operator =( const matrix &src )
{
    const T *pSrc = src.mbuf;
    T *pDest = mbuf, *pEnd = pDest + 9;

    // copy each item to destination array
    while ( pDest != pEnd )  *pDest++ = *pSrc++;
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
    const T *ap = mbuf, *ape = ap+9;
    for ( ; ap != ape; ap+=3 )
    {
        const T *bp = in.mbuf, *bpe = bp+3;
        for ( ; bp != bpe; ++mp, ++bp )
        {
            *mp = 0.0f;
            const T *atp = ap, *btp = bp;

            *mp += *atp * *btp;
            btp += 3; ++atp;

            *mp += *atp * *btp;
            btp += 3; ++atp;

            *mp += *atp * *btp;
            btp += 3; ++atp;
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
    for ( int i = 0; i < 3; ++i )
    {
        for ( int j = 0; j < 3; ++j, ++pm )
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

    // member operators
    bool operator !( void ) { return !wasInitialized; }

    // ctor/dtor
    block( size_type length );
    block( list<T> &source );
    ~block( void ) { delete [] pBegin; }

    // object control
    void init( T value );
    void clear( void ) { init( 0 ); }
    bool grow( size_type extra );

    // member accessors
    size_type size( void ) { return blockSize; }
    iterator begin( void ) { return pBegin; }
    iterator end( void ) { return pEnd; }

private:
    // array size and pointers
    size_type blockSize;
    iterator pBegin, pEnd;

    // initialized success flag
    bool wasInitialized;
};

// ------------------------------------------------------------------
// block object: constructor - allocate entire array block
// ------------------------------------------------------------------
template <class T>
array::block<T>::block( size_type length )
: wasInitialized(false)
{
    // allocate array block
    pBegin = new T[length];
    if ( !pBegin )  return;

    // setup pointers and array size
    blockSize = length;
    pEnd = pBegin + blockSize;

    // set success flag
    wasInitialized = true;
}

// ------------------------------------------------------------------
// block object: constructor - convert from list to block
// ------------------------------------------------------------------
template <class T>
array::block<T>::block( list<T> &source )
: wasInitialized(false)
{
    // retrieve block size from list
    blockSize = source.size();

    // allocate array block
    pBegin = new T[blockSize];
    if ( !pBegin )  return;

    // set end pointer
    pEnd = pBegin + blockSize;

    // copy each item from the source into the block
    typename list<T>::iterator i = source.begin();
    for ( iterator j = begin(); j != end(); ++j, ++i )  *j = *i;

    // set success flag
    wasInitialized = true;
}

// ------------------------------------------------------------------
// block object: grow - allows allocation of extra space
// ------------------------------------------------------------------
template <class T>
bool array::block<T>::grow( size_type extra )
{
    size_type newSize = blockSize + extra;

    // allocate new memory block
    iterator pNewBegin = new T[newSize];
    if ( !pNewBegin )  return false;

    // create new end pointer
    iterator pNewEnd = pNewBegin + newSize;

    // copy old data to new array
    iterator i = begin(), j = pNewBegin;
    for ( ; i != end(); ++i, ++j )  *j = *i;

    // deallocate old array
    delete [] pBegin;

    // update class members
    blockSize = newSize;
    pBegin = pNewBegin;
    pEnd = pNewEnd;

    // return success
    return true;
}

// ------------------------------------------------------------------
// block object: init - initialize block to value
// ------------------------------------------------------------------
template <class T>
void array::block<T>::init( T value )
{
    for ( iterator i = begin(); i != end(); ++i )
    {
        *i = value;
    }
}

// ------------------------------------------------------------------
// list object. allows dynamic allocation of any data type
// ------------------------------------------------------------------
template <class T>
class array::list
{
private:
    // link list node
    struct node
    {
        T value;
        node *pNext;
        node *pPrev;
    };

public:
    // member types
    typedef size_t size_type;

    // nested iterator class
    class iterator
    {
    private:
        // position indicator
        node *pNode;

    public:
        // pointer initializers
        iterator( node *pos ) { pNode = pos; }

        // pointer adjustment operators
        iterator* operator =( node *pos ) { pNode = pos; return this; }
        bool operator !=( iterator pos ) { return pNode != pos.pNode; }
        bool operator !=( node *pos ) { return pNode != pos; }
        bool operator ==( node *pos ) { return pNode == pos; }
        node* operator ++( void ) { pNode = pNode->pNext; return pNode; }
        node* operator --( void ) { pNode = pNode->pPrev; return pNode; }
        T& operator *( void ) { return pNode->value; }

    //private: <- this should not be public
        // node adjustment members
        void remove( void );
    };

    // member operators
    bool operator !( void ) { return !wasInitialized; }

    // ctor/dtor
    list( void );
    ~list( void );

    // list item control
    void clear( void );
    void remove( iterator &i );
    bool push_back( T value );
    T pop_back( void );
    bool push_front( T value );
    T pop_front( void );

    // member accessors
    size_type size( void ) { return nodeCount; }
    node* begin( void ) { return pBegin; }
    node* end( void ) { return pEnd; }

private:
    // list count and pointers
    size_type nodeCount;
    node *pBegin, *pEnd;

    // initialized success flag
    bool wasInitialized;
};

// ------------------------------------------------------------------
// list object: constructor - allocate first node in the list
// ------------------------------------------------------------------
template <class T>
array::list<T>::list( void )
: wasInitialized(false), nodeCount(0)
{
    // allocate node, verify success
    pBegin = new node;
    if ( !pBegin )  return;

    // initialize node
    pBegin->pPrev = 0;
    pEnd = pBegin;

    // set success flag
    wasInitialized = true;
}

// ------------------------------------------------------------------
// list object: destructor - deallocate all nodes
// ------------------------------------------------------------------
template <class T>
array::list<T>::~list( void )
{
    // for each node, remove the previous one
    while ( pEnd->pPrev )
    {
        pEnd = pEnd->pPrev;
        delete pEnd->pNext;
    }

    // finally, delete the starting node
    delete pBegin;
}

// ------------------------------------------------------------------
// list object: clear - restore list to it's initial empty state
// ------------------------------------------------------------------
template <class T>
void array::list<T>::clear( void )
{
    // for each node, remove the previous one
    while ( pEnd->pPrev )
    {
        pEnd = pEnd->pPrev;
        delete pEnd->pNext;
    }

    // reset pointer
    pEnd = pBegin;

    // reset node count
    nodeCount = 0;
}

// ------------------------------------------------------------------
// list object: remove - remove a node from the list
// ------------------------------------------------------------------
template <class T>
void array::list<T>::remove( iterator &i )
{
    // invoke iterators remove function
    i.remove();

    // decriment node count
    --nodeCount;
}

// ------------------------------------------------------------------
// list object: push_front - add a new node to the front of the list
// ------------------------------------------------------------------
template <class T>
bool array::list<T>::push_front( T value )
{
    // allocate new node, verify success
    pBegin->pPrev = new node;
    if ( !pBegin->pPrev ) return false;

    // update node pointers
    pBegin->pPrev->pNext = pBegin;
    pBegin->pPrev->pPrev = 0;
    pBegin = pBegin->pPrev;

    // set value in current node
    pBegin->value = value;

    // incriment node count, return success
    nodeCount += 1;

    return true;
}

// ------------------------------------------------------------------
// list object: pop_front - remove first node, and return it's value
// ------------------------------------------------------------------
template <class T>
T array::list<T>::pop_front( void )
{
    // store the value in the first node
    T original = pBegin->value;

    // set pointer to next node
    pBegin = pBegin->pNext;

    // deallocate first node
    delete pBegin->pPrev;
    pBegin->pPrev = 0;

    // decriment node counter
    nodeCount -= 1;

    // return nodes value
    return original;
}

// ------------------------------------------------------------------
// list object: push_back - add a new node to the back of the list
// ------------------------------------------------------------------
template <class T>
bool array::list<T>::push_back( T value )
{
    // set value in current node
    pEnd->value = value;

    // allocate new node, verify success
    pEnd->pNext = new node;
    if ( !pEnd->pNext ) return false;

    // update node pointers
    pEnd->pNext->pPrev = pEnd;
    pEnd = pEnd->pNext;

    // incriment node count, return success
    nodeCount += 1;

    return true;
}

// ------------------------------------------------------------------
// list object: pop_back - remove last node, and return it's value
// ------------------------------------------------------------------
template <class T>
T array::list<T>::pop_back( void )
{
    // store the value in the last node
    T original = pEnd->pPrev->value;

    // set pointer to previous node
    pEnd = pEnd->pPrev;

    // deallocate last node
    delete pEnd->pNext;

    // decriment node counter
    nodeCount -= 1;

    // return nodes value
    return original;
}

// ------------------------------------------------------------------
// list object - iterator: remove this node from list.
// ------------------------------------------------------------------
template <class T>
void array::list<T>::iterator::remove( void )
{
    // set node to point to previous
    node *pOld = pNode;
    pNode = pOld->pPrev;

    // set current node to skip old node
    pNode->pNext = pOld->pNext;

    // fix next nodes previous pointer
    pOld->pNext->pPrev = pNode;

    // feinally deallocate the old node
    delete pOld;
}
