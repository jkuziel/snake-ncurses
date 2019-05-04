# Snake in ncurses

Small project I made to teach myself ncurses (and snakes).

Some notable properties:

- Makes a copy of the game state each iteration so that a history may be kept in a future version. Currently the previous game state is copied back over.
- Quick and dirty timer implementation using `usleep`. A frame counter (incremented each loop) tracks passage of time. The program only sleeps for a brief period so that arrow keys are read and responded to without delay.
- Snake movement is tracked via a grid of directions. Each cell of the snake points toward the next cell, toward the head. This makes the stepping code a little more concise since the input direction can just be copied directly to the head cell, without figuring out the opposite direction. I haven't looked at any other implementations of snake, so there are likely more efficient ways of doing this.
- Calculating perpendicularity between two directions (input and head) is done with a simple modulus and comparison. The order that the directions are defined in the enum makes this possible.

## Known Compatibility

- macOS

## Build and Run

```
make && ./snake
```
