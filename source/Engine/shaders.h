/*
 *  shaders.h
 *  Sludge Engine
 *
 *  Created by Rikard Peterson on 2009-12-29.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#include "GLee.h"

char *shaderFileRead(char *fn); 
int buildShaders (const GLchar *vertexShader, const GLchar *fragmentShader);
