

#define in(a) (*(u32*)&(a))


#if 1

struct lowest_root_result
{
    bool32 hit;
    r32 root;
};

internal lowest_root_result
GetLowestRoot(r32 a, r32 b, r32 c, r32 maxR)
{
    lowest_root_result result = {};

    r32 determinant = b * b - 4.0f * a * c;

    if (determinant < 0.0f)
    {
	result.hit = false;
	return(result);
    }

    r32 sqrtD = (r32)sqrt(determinant);
    r32 r1 = (-b - sqrtD) / (2 * a);
    r32 r2 = (-b + sqrtD) / (2 * a);

    if (r1 > r2)
    {
	r32 temp = r2;
	r2 = r1;
	r1 = temp;
    }

    if ((r1 > 0) && (r1 < maxR))
    {
	result.root = r1;
	result.hit = true;
	return(result);
    }

    if ((r2 > 0) && (r2 < maxR))
    {
	result.root = r2;
	result.hit = true;
	return(result);
    }

    result.hit = false;
    return(result);
}




struct primitive_collision_result
{
    r32 t;
    bool32 collisionFound;
    v3 collisionPoint;
};

internal primitive_collision_result
CheckEdge(v3 p1, v3 p2, v3 base, v3 vector, r32 t)
{
    primitive_collision_result result = {};

    v3 edge = p2 - p1;
    v3 baseToVertex = p1 - base;
    r32 vectorLenSq = Dot(vector, vector);
    r32 edgeLenSq = Dot(edge, edge);
    r32 edgeDotVec = Dot(edge, vector);
    r32 edgeDotBaseToVertex = Dot(edge, baseToVertex);
    r32 a, b, c;



    a = edgeLenSq * -vectorLenSq + edgeDotVec * edgeDotVec;
    b = edgeLenSq * (2 * Dot(vector, baseToVertex)) - 2.0f * edgeDotVec * edgeDotBaseToVertex;
    c = edgeLenSq * (1 - Dot(baseToVertex, baseToVertex)) + edgeDotBaseToVertex * edgeDotBaseToVertex;

    lowest_root_result lResult = GetLowestRoot(a, b, c, t);
    if (lResult.hit)
    {
	r32 f = (edgeDotVec * lResult.root - edgeDotBaseToVertex) / edgeLenSq;

	if ((f >= 0.0f) && (f <= 1.0f))
	{
	    result.t = lResult.root;
	    result.collisionFound = true;
	    result.collisionPoint = p1 + (edge * f);
	}
	    
    }

    return(result);
}

internal primitive_collision_result
CheckVertice(v3 vector, v3 base, v3 vertex, r32 t)
{
    primitive_collision_result result = {};
    //Might sound crazy but this could be wrong, just check it incase
    r32 vectorSquaredLen = Dot(vector, vector);

    r32 a, b, c;


    a = vectorSquaredLen;
    b = 2.0f * (Dot(vector, (base - vertex)));
    v3 v3c = (vertex - base);
    c = Dot(v3c, v3c) - 1.0f;
    

    lowest_root_result lRoot = GetLowestRoot(a, b, c, t);
    if (lRoot.hit)
    {
	result.t = t;
	result.collisionFound = true;
	result.collisionPoint = vertex;
    }

    return(result);
}

bool32 CheckPointInTriangle(v3 point, m3 plane)
{
    v3 pa = plane.r1;
    v3 pb = plane.r2;
    v3 pc = plane.r3;

    v3 e10 = pb - pa;
    v3 e20 = pc - pa;

    r32 a = Dot(e10, e10);
    r32 b = Dot(e10, e20);
    r32 c = Dot(e20, e20);
    r32 ac_bb = (a * c) - (b * b);

    v3 vp = v3{point.x - pa.x, point.y - pa.y, point.z - pa.z};

    r32 d = Dot(vp, e10);
    r32 e = Dot(vp, e20);
    r32 x = (d * c) - (e * b);
    r32 y = (e * a) - (d * b);
    r32 z = x + y - ac_bb;

    return (( in(z))& ~(in(x) | in(y)) & 0x80000000);    
}
    

struct set_t_result
{
    bool32 hit;
    r64 t0, t1;
    bool32 embeddedInPlane;
};

internal set_t_result
SetT(r32 normalDotComparison, r32 signedDistToPlane, bool32 checkEmbed)
{
    set_t_result result = {};
    
    if (normalDotComparison == 0.0f)
    {
	if (fabs(signedDistToPlane) >= 1.0f)
	{
	    result.hit = false;
	    return(result);
	}
	else
	{
	    if (checkEmbed)
	    {
		result.embeddedInPlane = true;
	    }
	    result.t0 = 0.0;
	    result.t1 = 1.0;
	}
    }
    else
    {
	//we are going to collide
	result.t0 = (-1.0 - signedDistToPlane) / normalDotComparison;
	result.t1 = (1.0 - signedDistToPlane) / normalDotComparison;

	if (result.t0 > result.t1)
	{
	    r64 temp = result.t1;
	    result.t1 = result.t0;
	    result.t0 = temp;
	}

	if (result.t0 > 1.0f || result.t1 < 0.0f)
	{
	    //No collision possible return
	    result.hit = false;
	    return(result);
	}

	if (result.t0 < 0.0) result.t0 = 0.0;
	if (result.t1 < 0.0) result.t1 = 0.0;
	if (result.t0 > 1.0) result.t0 = 1.0;
	if (result.t1 > 1.0) result.t1 = 1.0;
    }

    return(result);
}

internal m3
GetVertsFromIndexedVoxel(voxel* currVoxel, i32 currIndex, voxel_chunk* chunk)
{
    //face i, {0, 2, 1}
    //0: x, y, z;
    //2: x, y, z...
    m3 result = {};

    //We are looking for verts at face i
    //currIndex += 3 each loop, {0, 2, 1}
    u16 i1 = currVoxel->renderedIndices[currIndex]; //0
    u16 i2 = currVoxel->renderedIndices[currIndex + 1]; //2 
    u16 i3 = currVoxel->renderedIndices[currIndex + 2]; // 1

    v3 vec1 =
    {
	chunk->verts[i1 * 3] + currVoxel->pos.x, 
	chunk->verts[i1 * 3 + 1] + currVoxel->pos.y, 
	chunk->verts[i1 * 3 + 2] + currVoxel->pos.z
    };

    v3 vec2 =
    {
	chunk->verts[i2 * 3] + currVoxel->pos.x,
	chunk->verts[i2 * 3 + 1] + currVoxel->pos.y,
	chunk->verts[i2 * 3 + 2] + currVoxel->pos.z
    };

    v3 vec3 =
    {
	chunk->verts[i3 * 3] + currVoxel->pos.x,
	chunk->verts[i3 * 3 + 1] + currVoxel->pos.y,
	chunk->verts[i3 * 3 + 2] + currVoxel->pos.z
    };

    result.r1 = vec1;
    result.r2 = vec2;
    result.r3 = vec3;

    return(result);
}

internal ray_cast
RayCastHitDetect(v3 start, v3 end, voxel_chunk* chunk)
{
    ray_cast result = {};

    //WELL: we compile 4/6/26 9:28pm but who knows what happens after that
    
    //NOTE: At some point create the ability to visualize these tris, bc I need 
    //to see if the triangles are being built correctly for hitting
    i32 indicesPerTri = 3;
    for (i32 i = 0; i < chunk->numOfRenderedVoxels; i++)
    {
	voxel* currVoxel = &chunk->voxels[chunk->renderedVoxelIndex[i]];
	
	//You should probably only test the rendered voxel faces, not the whole mesh

	i32 numOfRenderedTris = currVoxel->renderedIndiceCount / indicesPerTri;

	i32 currIndex = 0;
	
	for (i32 j = 0; j < numOfRenderedTris; j++)
	{
	    m3 plane = GetVertsFromIndexedVoxel(currVoxel, currIndex, chunk);
	    currIndex += 3;

	    v3 parameter1 = plane.r2 - plane.r1;
	    v3 parameter2 = plane.r3 - plane.r1;
//	    v3 normal = Cross((plane.r2 - plane.r1), (plane.r3 - plane.r1));
	    v3 normal = Cross(parameter1, parameter2);
	    normal = Normalize(normal);


	    r64 equationVal = -(normal.x * plane.r1.x + normal.y * plane.r1.y + normal.z * plane.r1.z);
	    r64 signedDistToTriPlane = Dot(start, normal) + equationVal;
	    r32 normalDotEnd = Dot(normal, end);


	    //Maybe make this function as opposed to magical
	    set_t_result tResult = SetT(normalDotEnd, (r32)signedDistToTriPlane, false);
	    if (!tResult.hit)
	    {
		continue;
	    }

	    bool32 collisionFound = false;
	    r32 t = 1.0f;
	    v3 collisionPoint = {};

	    v3 intersectionPoint = start + ((end - start) * (r32)tResult.t0);

	    if (CheckPointInTriangle(intersectionPoint, plane))
	    {
		collisionPoint = intersectionPoint;
		t = (r32)tResult.t0;
		collisionFound = true;
	    }
	    else
	    {
		primitive_collision_result collisionResult;
		collisionResult = CheckVertice(end, start, plane.r1, t);
		collisionResult = CheckVertice(end, start, plane.r2, t);
		collisionResult = CheckVertice(end, start, plane.r3, t);



		collisionResult = CheckEdge(plane.r1, plane.r2, start, end, t);
		collisionResult = CheckEdge(plane.r2, plane.r3, start, end, t);
		collisionResult = CheckEdge(plane.r3, plane.r1, start, end, t);

		intersectionPoint = collisionResult.collisionPoint;
		collisionFound = collisionResult.collisionFound;
		t = collisionResult.t;
	    }

	    if (collisionFound)
	    {
		r32 distToCollision = t * (r32)sqrt(Dot(end, end));
		if (result.hit == false || distToCollision < result.nearestCollision)
		{
		    result.nearestCollision = distToCollision;
		    result.hitLocation = intersectionPoint;
		    result.hit = true;
		}
		
	    }
	}
    }

    return(result);
}
#endif

