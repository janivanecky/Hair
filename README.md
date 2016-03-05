#Hair simulator

This is a project for school - hair simulation based on particle systems. Implementation is based on papers by [*Muller et al.*](http://matthias-mueller-fischer.ch/publications/FTLHairFur.pdf) 
and [*Petrovic et al.*](http://graphics.pixar.com/library/Hair/paper.pdf).

##Modes

* hair is attached at one point

![alt text](https://github.com/janivanecky/Hair/blob/master/Hair/img/strand.png "Strand mode")

* hair is attached to the "head" (top half of the sphere). For stable simulation you need higher values of s_damp.

![alt text](https://github.com/janivanecky/Hair/blob/master/Hair/img/head.png "Hair mode")

##Simulation parameters

* s_repulsion - specifies how strong is the repulsive force between individual hair 
* s_damp - basically density of the environment
* s_wind - strength of the wind

![alt text](https://github.com/janivanecky/Hair/blob/master/Hair/img/animation.gif "In action")

##Controls

* R - resets the simulation
* defining HEAD in main.cpp spawns hair on the top hemisphere of the "head"

##Notes

[imgui](https://github.com/ocornut/imgui) library was used for the UI. Thanks!



