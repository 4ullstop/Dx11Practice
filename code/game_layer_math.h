#if !defined (GAME_LAYER_MATH_H)

struct v3
{
    union
    {
	struct
	{
	    r32 x, y, z;
	};
	r32 e[3];
    };

    inline v3 &operator*=(r32 a);
    inline v3 &operator+=(v3 a);
    inline v3 &operator-=(v3 a);
};

inline v3
operator*(r32 a, v3 b)
{
    v3 result;
    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return(result);
}

inline v3
operator*(v3 b, r32 a)
{
    v3 result;
    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return(result);
}

inline v3
operator/(v3 a, v3 b)
{
    v3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return(result);
}

inline v3
operator-(v3 a)
{
    v3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return(result);
}

inline v3
operator+(v3 a, v3 b)
{
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return(result);
}

inline v3
operator-(v3 a, v3 b)
{
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return(result);
}

inline v3
operator/(v3 a, r32 b)
{
    v3 result = {};
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;

    return(result);
}

inline v3 &v3::
operator*=(r32 a)
{
    *this = a * *this;
    return(*this);
}

inline v3 &v3::
operator+=(v3 a)
{
    *this = *this + a;
    return(*this);
}

inline v3 &v3::
operator-=(v3 a)
{
    *this = *this - a;
    return(*this);
}

internal r32
DotV3(v3 a, v3 b)
{
    r32 result;

    result = ((a.x)*(b.x) + (a.y)*(b.y) + (a.z)*(b.z));
    return(result);
}

/*
********v2********
 */

struct v2
{
    union
    {
	struct
	{
	    r32 x, y;
	};
	r32 e[2];
    };

    inline v2 &operator*=(r32 a);
    inline v2 &operator+=(v2 a);
    inline v2 &operator-=(v2 a);    
};



inline v2
operator*(r32 a, v2 b)
{
    v2 result;
    result.x = a * b.x;
    result.y = a * b.y;


    return(result);
}

inline v2
operator*(v2 b, r32 a)
{
    v2 result;
    result.x = a * b.x;
    result.y = a * b.y;


    return(result);
}

inline v2
operator/(v2 a, v2 b)
{
    v2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return(result);
}

inline v2
operator-(v2 a)
{
    v2 result;
    result.x = -a.x;
    result.y = -a.y;


    return(result);
}

inline v2
operator+(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;


    return(result);
}

inline v2
operator-(v2 a, v2 b)
{
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;


    return(result);
}

inline v2 &v2::
operator*=(r32 a)
{
    *this = a * *this;
    return(*this);
}

inline v2 &v2::
operator+=(v2 a)
{
    *this = *this + a;
    return(*this);
}

inline v2 &v2::
operator-=(v2 a)
{
    *this = *this - a;
    return(*this);
}

inline bool32
operator==(v2 a, v2 b)
{
    bool32 result = (a.x == b.x) && (a.y == b.y);
    return(result);
}

internal r32
DotV2(v2 a, v2 b)
{
    r32 result;

    result = (a.x)*(b.x) + (a.y)*(b.y);
    return(result);
}


#define GAME_LAYER_MATH_H
#endif
