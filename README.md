# byoe : Aseroids adventure
A while ago some of us came together to make a game for BYOG (Build Your Own Game) a game jam that happend just before IGDC. Initially we hoped to make a game from scratch but realized it'll take a longer time given our requirements. 

We decided to build a mini-engine using SDF (signed distance fields) and make a clone of asteroid but with a slap of ghosts. This is an attemp to make such a game.

## Engine 
The engine is completely made in C and uses SDFs for rendering all it's primitives. This engine runs on Windows/Mac/Linux and is tested from time to time. We will add more info on how the SDF renderer works and architecture details in the PRs and link them here as we develop.

### Scripting
We have a simple *C-based Scripting system*. Check the PR#2 for details on [Scripting in engine using c](https://github.com/PsychedelicOrange/byoe/pull/2). Check the [game folder and specificaly the camera_controller](https://github.com/PsychedelicOrange/byoe/blob/sdf-renderer-draft-1/game/camera_controller.c) for examples on how scripting can be added to game objects in the game side.

We are currently building a SDF renderer to render bulles, space ships and planets.

## Tools
We also have plans to build a SDF editor that unlike others can generate functions that can be re-used in other shading languages. Most SDF editors output popular 3D formats, but in relaity outputting math formulas and funtions are the most useful to integrate them into any engine and edit them further.

## Building

Folow these instructions to build the engine and run the game

```
git clone https://github.com/PsychedelicOrange/byoe.git
mkdir build
cd build
cmake ..
```

you can configure cmake to generate project files (XCode/VS22 etc.) for you and build using your favourite IDE in the usual way 

### Running the game
```
make run
```

**Running Tests**
we use a custom test header to test the engine API, no engine start and shut down/gfx tests are being done at the moment only API tests. Run them using the following commands
```
make run.tests
```

**Running Benchmarks**
The engine has some benchmakrs such as uuid_t generation stats and hash_map for testing performance related stats. Run them using the following commands
```
make run.benchmarks
```
