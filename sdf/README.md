# architecture
* `sdf_editor` will allow creation of complex sdf shapes and will save the file as .sdf (json file)
* `.sdf` sdf format will be the format for storing `sdf_editor` projects
* `sdf_parser` will parse the `.sdf` json file into `sdf_types`
* `sdf_render` will convert a `.sdf` file to a shader file that can be imported into your engine.
# sdf format specification
### Inspirations and reference
* the goat - https://iquilezles.org/articles/distfunctions/
* gltf spec
* cgltf lib
### Concepts
#### Primitives
These are standard 3d shapes like sphere, cube, etc.. along with parameters to define their shape.
Operations can be specified on these shapes individually.
Each of them can have distinct materials.
#### Objects
These are the collection of primitives which are rendered together in 1 draw call.
Operations can be specified on primitives also, which will be applied after all the primitive operations and blending(min function) are processed. We will output one shader file per object.
#### Blending
These will define which blend function to use btw two primitives within a object, as well as bolean operations like union intersection, etc.
If not specified, by default it will use a min function and union two primitives.
#### Materials
### Apendix
#### List of Operations, Primitives, and Blending functions
### Examples
see [examples](./examples/spaceship.sdf)
