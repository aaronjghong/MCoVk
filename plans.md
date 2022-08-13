# MCoVK

## A Minecraft Clone built on Vulkan

### Current plans
-   Setup initial Vulkan Framework
    -   Render Square -> *Complete*
    -   Setup Texturing -> *Complete*
    -   Render Cube -> *Complete*
    -   Setup MVP matrices -> *General Camera Movement Complete*
        -   [Move MVP and transform data to Push Constants](https://vkguide.dev/docs/chapter-3/push_constants/) -> *Complete*
    -   Setup Cube Chunks and Render Calculations for Culling
        -   [Store Cubes, and Cubes to be rendered. Make a func to gen Cubes to be rendered from Cubes](https://forums.minecraftforge.net/topic/73753-114-how-minecraft-renders-blocks/)
    -   Refactor and Abstractify code
-   Handle HLRs
    -   Partition needed actions *( i.e. need X pipelines for etc. )*
    -   Plan HLRs / needed modules
    -   Assign vulkan components to needed modules / handlers


### General Future Plans
-   Setup Block rendering
    -   Rendering of multiple textures 
        -   Texture Atlas -> cube
    -   Culling of unneeded faces
    -   Block generation with Perlin Noise  -> *Complete*
        -   Heightmap sampler in vertex shader?
    -   Block storage *( Storing world state / placed and broken blocks )*
    -   Chunking / Chunk id system
        -   Fog effects for introducing chunks
        -   Loading and unloading of chunks while saving state
            -   Hashtable of array / vector with key being Chunk id
        -   Better render mesh handling when chunking (isolating chunks to recreate mesh, etc)
    -   Continual Generation

-   Setup Player 
    -   Create movement system 
    -   Rendering player model / vertices
        - New pipeline? -> [For Reference](https://stackoverflow.com/questions/53307042/vulkan-when-should-i-create-a-new-pipeline) 
    -   Player view, raycasting and distance-to-block calculations
    -   
    

-   Create GUI
    -   Simple settings menu *( Render distance, fog, effects, etc )*
    -   [Should set-up dynamic states for resizing / changing settings](https://www.youtube.com/watch?v=iGDkXeLQ15w)


-   Handle Audio
    -   Details TBD


## Some notes for future reference
-   All of the helper functions that submit commands so far have been set up to execute synchronously by waiting for the queue to become idle. For practical applications it is recommended to combine these operations in a single command buffer and execute them asynchronously for higher throughput, especially the transitions and copy in the createTextureImage function. Try to experiment with this by creating a setupCommandBuffer that the helper functions record commands into, and add a flushSetupCommands to execute the commands that have been recorded so far. It's best to do this after the texture mapping works to check if the texture resources are still set up correctly.
-   [S.O. answer on multiple textures](https://stackoverflow.com/questions/36772607/vulkan-texture-rendering-on-multiple-meshes)
