#include "game_layer.h"

extern "C" GAME_INITIALIZE(GameInitialize)
{
    //Call our OBJ loader function here from the dll such that it is anonymouse and fills out
    //the necessary data for (in this case) dx11 to read

    //LoadOBJ();
    //ParseOBJ;
    //Write your file locations here

    
    
    char* fileLocation = "D:/ExternalCustomAPIs/OBJLoader/misc/cubetester_normals.obj";
    obj* result = parseOBJCode->ParseOBJData(fileLocation, objLocationArena, mainProgramMemory);
    
    return(result);
}

extern "C" GAME_UPDATE(GameUpdate)
{
    //Loading in objects?
}
