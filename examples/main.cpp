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