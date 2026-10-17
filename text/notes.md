# September 14th

While traversing, also note other agents’ positions, if they successfully have moved, mark the map as Traversable (but
unsure what it is) – unless it’s communicated by them that it’s something unique (in order to have some understanding of
the map)

For the map assume speed is only 1 – (it will be for grading)

NOTE: If a rogue is destroyed, it does not have an opportunity to communicate this fact. ->>>>>> you can probably assume
that’s the case when it doesn't move and the message is stuck_

Colliding 2 maps when teleporting (use the sign of the teleporter - if seen before (good) - use direction, also after
enough traversal - start comparing the maps for similarities (submap)

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
- it is funny to look at this primitive agent and how it works for the default map, but for my custom - not, because of
  a simple reason by default if no trap or treasure is in sight - it moves forward an it gets stuck
- it should have a desire to disarm traps; and obv highest one - eat treasures
- it feels like im trying to teach a baby how to do calc; or more like a dog which always sniffs the same spot instead
  of moving somewhere else
- you can quite easily see why reactive agents (even for vacuum cleaners) wouldnt work well at all - as they just spin
  around and get stuck (unless you introduce pseudo randomness) - although before continuing with a model agent, i know
  that i can make a better reactive one => i will pursue it in the coming hours and maybe max 2 days before moving on
- if the desire for traps is really high, it gets stuck in the walls, so i should find a solution

# October 7th

## Mapping

To be combined with rogue communication if more than one agent is present. Graphs are great but not as performant as
basic 2D arrays. We know that maps aren't more than 50x50 (most likely) and even if they are, we could just expand "the
grid". It should start in the center and do its thing afterwards. That's why a 100x100 would work fine as it isn't that
more memory intensive, but it saves quite much on performance and potential array enlargements - useless copying and
computation.

There should be an "algorithm" to detect the end of the map / map size. I don't know how necessary it would be - as it
could just remain as "unexplored" instead of unreachable.

Map collisions with teleporting are to be examined in the hopefully near future. That would requre some sorts of a
complex matching algorithm for the arrays. todo - read articles on that topic – preferably simple (both complexity and
memory-wise) solutions.

## Rogue Communication

My biggest uncertainty is the handling of rogue communication as it is quite limited only 32 bits before the move is
executed. So, what could I do with that?

{handwritten notes to be inserted here 2.5 pages of agony}

Work on Encode // Decode Communications has started. Here follow the "coding" notes:

- Bla

# October 9th

What about a set of new actions - instead of just thinking each round; think of a *useful* way to “think” a few actions
ahead and evaluate them before running

Informed search - for traps

Uninformed - for exploring the map

How do I balance it off??

Also try to guess the trap itself, instead of just rotating

Also add "potential trap" string

# October 10th - 15th (writing project + preparing for quiz 2)

What about a set of new actions - instead of just thinking each round; think of a *useful* way to “think” a few actions
ahead and evaluate them before running Informed search - for traps Uninformed - for exploring the map How do I balance
it off?? Also try to guess the trap itself, instead of just rotating

What about a cost function, we should somehow take into account the costs provided to the agent- maybe for traversing

If time I could do a graph, I dont think its necessary tho; although it may be useful to know taken path - maybe a
separate data structure?

I think some sort of DFS would be could - until you run to a wall, but with higher priority - traps ;;; no BFS with
greedy frontier If very limited turns - hunt for traps, think of exploration - later Cost analysis at least for - dont
go back to nodes already explored unless you want to use them to reach unexplored land beyond them - either see it, or
haven’t amped Do not make loops best case, but if you have to go back - go It is gonna keep track of historic
movements - if it gets stuck and has to go back

NOTE for comm implementation -> we dont know the direction of the bots - could be different, before we use the info,
they should go forward, then backwards, and look to the left- it’s gonna know even in П shaped; TS gonna be a bit
complicated - at least an hour or 2

Iterative Deepening? No

Disarm all traps -> bad idea cuz your area might be cleared of traps and you’ll have to teleport

Goal - expanding frontier, disarming nearby traps (under 5 Manhattan?), but if treasuer is seen - highest priority to
it, unless trap distance = 1 It should be an algorithm Searching ALWAYS HAPPENS WITH THE MAP, not the percepts -
percepts ONLY feed the map => operations Even if T is seen, first look up the map - are the fields to it save, or do we
have to look for traps.

NOTE -> tie the teleported - map which one to which

An online algorithm, on the other hand, can discover successors only for a state that it physically occupies. To avoid
traveling all the way to a distant state to expand the next node, it seems better to expand nodes in a local order.
Depth-first search has exactly this property because (except when the algorithm is backtracking) the next node expanded
is a child of the previous node expanded.

Importance Todo

- Traversing and Map Handling
- Handling Teleport (multiple maps, and collision)
- Handling Communication

Next thing to do - handling comm, as I want richer map

FIX ->>>>>>> ROTATION SHOULD BE COMMUNICATED !!!! Maybe R F F F R R L L

# October 17th

