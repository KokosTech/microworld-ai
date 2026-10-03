# Before October 3rd

to be transferred... (mainly ideas of how to approach the agent and the environment)

# October 3rd

The idea of RTrees (great for spacial/maps) was explored and was deemed unnecessarily complex for the scope of this
project. Additionally, it would shift the focus from agentic behavior to a data structure problem. Although I would
definitely love to explore the RTree in the future; leaving a link to a great article on
them – https://www.bartoszsypytkowski.com/r-tree/

_**!!! In the coming week the project will be created with the following assumptions (to reduce complexity):**_

- **Slip Chance** is ALWAYS ZERO – it'd add unnecessary complexity and should only be approached when everything else is
  in a "working" state
- **Agent Speed** is ALWAYS ONE – again, as Prof. Zac said – we should focus on the rest

Project was formatted to use the Google Standard for C/C++ - I just like it

Starting code implementation with testing several ideas for map movement (without keeping map in memory first) – just
testing approaches for traps, treasures and handling the percepts (single-agent mode)

Ideas and short notes while coding:

- the agent should spin in the beginning (270deg) to map the surrounding env; it's like state 0, just to be used once,
  afterward it moves to the "actual" code - maybe not the best idea if you know they are walls – to be checked later
- working on a reactive agent right now will be moved to a model / utility agent later
- just like the more complex models in class, we should be able to evaluate intentions (commands to be executed) – right
  now I am working with only a single one, but it would be a changed to a map with commands and their evaluation