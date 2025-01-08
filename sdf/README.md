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
#### Sdfs
These are standard 3d shapes like sphere, cube, etc.. along with parameters to define their shape.
Operations can be specified on these shapes individually.
Each of them can have distinct materials.
#### Primitives
These are the collection of sdfs which are rendered together in 1 draw call.
Operations can be specified on primitives also, which will be applied after all the sdfs operations and blending(min function) are processed.
#### Mixing
These will define which blend function to use btw two sdfs within a primitive, as well as bolean operations like union intersection, etc.
If not specified, by default it will use a min function and union two objects.
#### Materials
### Apendix
#### List of Operations, Shapes, and Mixing functions
see [sdftypes](./sdf_parser/sdf_types.h)
### Examples
see [examples](./examples)
### Demo Structure
```
{
	"primitives":[
        {
            "label":"body",
            "sdfs":[
                {
                    "label":"main_body",
                    "type":"sphere",
                    "radius":"10",
                    "center":"0,0,0",
                    "operations": [
                    {
                        "type":"rotate",
                        "axisangle": [1,0,0,3.14],// or
                        "quaternion": [1,23,45,100] // or euler etc.
                    },
                    {
                        "type":"elongation",
                        "axis": [1,0,0]
                            "length": 10
                    }
                    ],
                    "material":{
                        "color": [1,0,0],
                        "transparency": 1,
                        // custom shader effects here
                        "pbr_rough_metal_texture": {
                            "path": "~/texture/spaceship_body_rough_metal.png",
                            // uv mapping parameters
                        }
                        "pbr_normal_texture": {...}
                        "pbr_albedo_texture": {...}
                    }
                },
                {
                    "label":"glass_sphere",
                    "type":"hemisphere",
                    "radius":"6",
                    "center": [0,0,0],
                    "face_dir": [1,0,0]
                }
            ],
            "operations":[
                    {
                        "type":"elongation",
                        "axis": [1,0,0]
                            "length": 10
                    }
                
            ]
            "mixing":[
                {
                    "sdf_a": 0,
                    "sdf_b": 1,
                    "function": "smooth",
                    "parameter": 0.35
                }
            ]
        },
        {...}
    ]
}
```
