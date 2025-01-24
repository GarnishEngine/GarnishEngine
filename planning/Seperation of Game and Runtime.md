### Seperation of Game and Runtime
 * Engine: This is what is used to build the game, because were going more framework style this will be rather minimal

 * Runtime: This is going to be the largest thing that we need to make, this will be what actually runs the game,
         it should include everything that actually runs the game renderer, the physics system, the audio system,
         an ECS if we want to go that route ect
    * A key feature of the runtime is that it needs to be able to be ran in a debug and a release mode, in debug mode
    a few things should be available including: debug drawing within the scene, a small debug GUI that the game
    maker can use to adjust things while the game is running, some kind of logging to console system. Things things
    dont really need to exist in release mode.

* The game: There should be a pretty clear seperation between this and the runtime, the Game should include: models,
          textures, audio files, a description of each scene/level, a system to pick which level to play next ect.

A key goal to keep in mind is that we are making a game ENGINE, not a game, we will be developing a small game 
(such as pong) along side the engine, but it needs to remain clear that the small game is not the project. A key
consequence of this is that the engine needs to become a library with no main function. Because we have no GUI
for the engine, there should be no way to run it.

This leads to an important question, what should it look like for a user (someone making a game with Garnish) to 
use Garnish, ie what should the users main file look like, heres an example:
```
#include <garnish_engine.hpp>

int main() {
    garnish::Engine engine{ };
    engine.Start();

    engine.LoadScene(Pong{ });

    engine.LoadScene(PongWinningScreen{ })

    engine.End();
}
```
We should fully lay out what we want something like this to look like before we get too far into the 
development of the engine.