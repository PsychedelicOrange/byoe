#ifndef ENGINE_H
#define ENGINE_H

#include <stdio.h>
#include <stdbool.h>

// Include all engine realted includes here
#include "core/gameobject.h"

void initEngine()
{
    printf("Welcome to Build your own engine! BYOE!!\n");
    printf("\t->version: 0.0.1\n");
}

void exitEngine()
{
    printf("Exiting BYOE...Byee!");
}

// TODO: This is set by the input system. make this explicit bool enginShouldQuit()
bool engineShouldQuit()
{
    return false;
}

void runEngine()
{
    while(!engineShouldQuit())
    {
        // Run the enginesystem;
    }
}

#endif