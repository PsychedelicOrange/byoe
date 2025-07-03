![Linux Build](https://github.com/parth/game-engine/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/parth/game-engine/actions/workflows/build.yml)

# BYOE : Asteroids adventure
A while ago some of us came together to make a game for BYOG (Build Your Own Game) a game jam that happend just before IGDC. Initially we hoped to make a BYOE (Build Your Own Engine) game from scratch but realized it'll take a longer time given our requirements. 

We decided to build a mini-engine using SDF (signed distance fields) and make a clone of asteroid but with a slap of ghosts. This is an attemp to make such a game and also make some tools for SDF editing.

## Built by:
Parth Sarvade   | [Github: PsychedelicOrange](https://github.com/PsychedelicOrange) | [Discord: psychedelicorange]()

Maanyavar       | [Github: ]() | [Discord: earnestly]()

Phani Srikar    | [Github: Pikachuxxxx](https://github.com/Pikachuxxxx) | [Discord: pikachuxxx]()

## Engine 
The engine is completely made in C and uses SDFs for rendering all it's primitives and uses Vulkan 1.2 (cause of VK_KHR_Dynamic_rendering extension) for rendering. This engine runs on Windows/Mac/Linux and is tested from time to time. We will add more info on how the SDF renderer works and architecture details in the PRs and link them here as we develop.

### Scripting
We have a simple *C-based Scripting system*. Check the PR#2 for details on [Scripting in engine using C](https://github.com/PsychedelicOrange/byoe/pull/2). Check the [game folder and specificaly the camera_controller](https://github.com/PsychedelicOrange/byoe/blob/sdf-renderer-draft-1/game/camera_controller.c) for examples on how scripting can be added to game objects in the game side.

We are currently building a SDF renderer to render bulles, space ships and planets.

### Renderer Design
Doing everything from withing a shader is performance heavy, we cannot have for loops in shader code. Hence the renderer issues a number of drawcalls and intuitively handles SDF combinations using node based scene description. During the early stages the scene and operations will be dynamic and no pre-computation will be done, we will use a Compute Shader approach  and handle this using simple data structures.

This is the first draft of the preliminary design for the sdf_renderer: #PR10 [SDF Renderer Abstraction Draft - 1](https://github.com/PsychedelicOrange/byoe/pull/10)

In future versions this can be done using compute shaders and 3D textures for pre-computing static geometry and handle SDFs in a perf safe way. We will also support using custom SDF functions in our SDF editor.

## Tools
We also have plans to build an **SDF editor**. Unlike most editors that use the SDF techniques and styling to create 3D models, we want our editor to create SDF functions, that can be re-used in other shading languages. Most SDF editors output popular 3D formats, but in relaity outputting math formulas and functions are the most useful thing to integrate them into any sdf rendering engine and edit them. We will try to export in GLSL/HLSL/JSON formats.

More info on the editor can be found on the sdf-editor branch here: https://github.com/PsychedelicOrange/byoe/tree/sdf-editor/sdf

## Building

Folow these instructions to build the engine and run the game. All the dependencies such as cglm, glfw etc. will be cloned and built by CMake itself. You can either use make on linux/mac and VisualStudio on Windows. We recommend using NInja on Linux/Mac for super fast compilation, windows VS project is compiled with /MP option enabled.

```
git clone https://github.com/PsychedelicOrange/byoe.git
mkdir build
cd build
cmake ..
make 
```

you can configure cmake to generate project files (XCode/VS22 etc.) for you and build using your favourite IDE in the usual way 

### Building the targets
```
make
```
Execute this command  to build all the targets that have changes (such as dependencies, engine, game, tests and benchmarks).

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
