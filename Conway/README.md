# Conway's Game of Life

A visualisation of the *Conway's Game of Life*. You can run the generated binary by providing a floating point number that tunes the probability of the cells being *alive*.

## Rules:

1. If the cell is *alive* and it has less than **two** *living* neighbors, it *dies*. (**Underpopulation**)
2. If the cell is *alive* and it has more than **three** *living* neighbors, it *dies*. (**Overpopulation**)
3. If the cell is *alive* and it has exactly **two** *or* **three** *living* neighbors, it stays *alive*. (**Survival**)
4. If the cell is *dead* and it has exactly **three** *living* neighbors, it comes to *life*. (**Reproduction**)

Based on these *four rules* the *Game of Life* plays. In the code all the rules are written in one line:

```C++
buffer[index] = (neighborCount==3) || (currentCell && neighborCount==2);
```

