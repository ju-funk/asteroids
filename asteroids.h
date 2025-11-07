// ------------------------------------------------------------------
// entity object. stores sprite relevant variables
// ------------------------------------------------------------------
class entity
{
public:
    // constructor
    entity( model &source, float xpos, float ypos );

    // direction control
    void updatePos( void );
    void addPos( void );
    void setDir( float angle );
    void addDir( float angle );
    void swapDir( entity *with );
    void swapSpeed( entity *with );

    // member vars
    vertex pos;
    float rx, ry, rz;
    float speed;

    int health;
    bool isAsteroid;
    bool canCollide;

    // model pointer and scale
    float scale;
    model points;
};
