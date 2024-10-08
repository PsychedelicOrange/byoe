#include "gameobject.h"

HashMapNode gGameRegistry[MAX_OBJECTS];
uint32_t gNumObjects = 0;

uuid_t registerGameObjectType(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn)
{
    uuid_t uuid;

    if (gNumObjects >= MAX_OBJECTS) return uuid;  // Registry is full

    // Find a free spot in the hash map
    int index = gNumObjects;
    uuid_generate(&uuid);  // Generate a UUID
    uuid_print(&uuid);

    // FIXME
    // Generate a unique UUID for the object if not provided
    // index = findFreeSpotOrUUID(uuid, true);
    //     // Ensure the UUID is unique by checking for duplicates
    //     if (index == -1) {
    //         printf("Cannot register game object: UUID already in use.");
    //         return uuid; 
    //     }

    // Populate the GameObject within the existing HashMapNode
    gGameRegistry[index].occupied = true;
    // uuid_copy((uuid_t*)gGameRegistry[index].key, &uuid);
    gGameRegistry[index].value.typeName = typeName; 
    gGameRegistry[index].value.gameObjectData = malloc(gameObjectDataSize); // Assign gameObjectData
    memset(gGameRegistry[index].value.gameObjectData, 0, sizeof(gameObjectDataSize)); /* Initialize data to zero */
    gGameRegistry[index].value.startFn = StartFn;  // Assign start function
    gGameRegistry[index].value.updateFn = UpdateFn; // Assign update function

    gNumObjects++;
    return uuid;  // Return the UUID of the registered object
}

void unRegisterGameObjectType(const uuid_t uuid) {
    int index = findFreeSpotOrUUID(uuid, true);
    if (index != -1 && gGameRegistry[index].occupied) {
        free(gGameRegistry[index].value.gameObjectData); // Free object data
        gGameRegistry[index].occupied = false;
        gNumObjects--;
    }
}