# byoe : Aseroids adventure
A while ago some of us came together to make a game for BYOG (Build Your Own Game) a game jam that happend just before IGDC. Initially we hoped to make a game from scratch but realized it'll take a longer time given our requirements. 

We decided to build a mini-engine using SDF (signed distance fields) and make a clone of asteroid but with a slap of ghosts. This is an attemp to make such a game.

## Engine 
The engine is completely made in C and used SDF for rendering all it's primitives. This engine runs on WIndows/Mac/Linux and is tested from time to time. We will add more info on how the SDF renderer works and architexture details in the PRs and link them here as we develop.

### Scripting
We have a simple *C-based Scripting system*. Checkout the PR#2 for details on [Scripting in engine using c](https://github.com/PsychedelicOrange/byoe/pull/2). Checkout the game folder for examples on how scripting can be added to game objects.

We are currently building a SDF renderer to render bulles, sapce ships and planets.

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
we use custom test header to test the engine API, no engine start and shut down/gfx tests are being done at the moment only API tests. Run them using the following commands
```
make run.test
```

**Running Benchmarks**
The engine has some benchmakrs such as uuid_t generation stats and hash_map for testing performance related stats. Run them using the following commands
```
make run.benchmarks
```
