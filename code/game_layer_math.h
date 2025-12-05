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

#define GAME_LAYER_MATH_H
#endif
