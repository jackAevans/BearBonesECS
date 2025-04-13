# Bear Bones ECS

A simple, fast, and modern C++ Entity-Component System — all in a single header file!

## Features 
 - **Header-only:** No need to link to any libraries, just include the header.
 - **Lightweight:** Fast and efficient ECS with minimal overhead.
 - **Flexible:** Easily extendable and works for both small and large projects.
- **Fast:** Supports multi-threaded, cache-friendly iteration over components, optimizing performance.
- **Intuitive API:** Add and access components, manage entities, and define systems with just a few lines of code.

## Getting Started
To get started with the ECS, simply include the ecs.hpp file in your project.
``` cpp
#include "bearBonesECS.hpp"
```
Then, create your components, entities, and systems to start building your game or application.

**Example:**

``` cpp
#include "bearBonesECS.hpp"

// Position Component
struct Position {
    float x, y;
};

// Velocity Component 
struct Velocity {
    float x, y;
};

int main() {
    bbECS::ECS ecs; // Create an ECS instance
    
    // Add component types
    ecs.addComponentType<Position>();
    ecs.addComponentType<Velocity>();

    // Add an Entity and components
    bbECS::EntityGUID guid;
    ecs.addEntity(guid)
       .addComponent<Position>(14.0f, 2.0f)
       .addComponent<Velocity>(1.0f, 2.0f);

    bbECS::SystemBatchID sbID = ecs.addSystemBatch(); // Add a SystemBatch

    // Add systems to the SystemBatch
    ecs.addSystem(sbID, [&guid](bbECS::ECS& ecs){
        std::cout << "Running my first ever system!" << std::endl;
        
        ecs.getComponent<Position>(guid).x += 1; // Modify the Position component
    });

    ecs.addSystem<Position, Velocity>(sbID, [](bbECS::ECS& ecs){
        std::cout << "Running my second ever system!" << std::endl;

        // Access and modify components
        ecs.forEach<Position, Velocity>([](Position& pos, Velocity& vel){
            pos.x += vel.x;
            pos.y += vel.y;
        });
    });

    // Run the SystemBatch
    ecs.runSystemBatch(sbID);
    
    return 0;
}
```

## Documentation
- **Examples:** Check out the `examples/` directory for more usage samples.
- **Docs** Visit the `docs/` folder for more in-depth explanations of systems, entities, components, and advanced features.

## License
Bear Bones ECS is released under the [MIT License](./LICENSE)

