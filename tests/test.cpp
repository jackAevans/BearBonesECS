#include <iostream>
#include <fstream>

#include "bearBonesECS.hpp"

#define TEST_ECS(x) std::cout << "Running test: " << #x; if(!x()) { std::cerr << " - Failed";  } else { std::cout << " - Passed"; } std::cout << std::endl;

struct Position
{
    double x, y;
};

struct Velocity
{
    double x, y;
};

struct State
{
    int state;
};

// test adding and removing entities 
bool testAddRemoveEntities()
{
    bbECS::ECS ecs;

    bbECS::EntityGUID ent1;
    bbECS::EntityGUID ent2;

    ecs.addEntity(ent1);
    ecs.addEntity(ent2);

    ecs.removeEntity(ent1);
    ecs.removeEntity(ent2);

    return true;
}
// test adding and removing component types
bool testAddRemoveComponentTypes()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    ecs.removeComponentType<Position>();
    ecs.removeComponentType<Velocity>();

    return true;
}
// test adding and removing components
bool testAddRemoveComponents()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    bbECS::EntityGUID ent;
    ecs.addEntity(ent)
        .addComponent<Position>(ent, Position{0.0, 0.0})
        .addComponent<Velocity>(ent, Velocity{1.0, 1.0});


    ecs.removeComponent<Position>(ent);
    ecs.removeComponent<Velocity>(ent);

    return true;
}
// test accessing and modifying components
bool testAccessModifyComponents()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    bbECS::EntityGUID ent;
    ecs.addEntity(ent);

    ecs.addComponent<Position>(ent, {0.0, 0.0});
    ecs.addComponent<Velocity>(ent, {1.0, 1.0});

    Position &pos = ecs.getComponent<Position>(ent);
    Velocity &vel = ecs.getComponent<Velocity>(ent);

    pos.x += vel.x;
    pos.y += vel.y;

    if(pos.x != 1.0 || pos.y != 1.0)
    {
        std::cerr << "Error: Position component not modified correctly." << std::endl;
        return false;
    }
    if(vel.x != 1.0 || vel.y != 1.0)
    {
        std::cerr << "Error: Velocity component not accessed correctly." << std::endl;
        return false;
    }

    return true;
}
// test adding and removing relationships
bool testAddRemoveRelationships()
{
    bbECS::ECS ecs;

    bbECS::EntityGUID parent;
    bbECS::EntityGUID child;

    ecs.addEntity(parent);
    ecs.addEntity(child);

    ecs.addRelationship(parent, child);
    ecs.removeChild(parent, child);

    return true;
}
// test adding and running systems
bool testAddRemoveSystems()
{
    bbECS::ECS ecs;

    bbECS::SystemBatchID sbID = ecs.addSystemBatch();

    ecs.addComponentType<Position>("Position");

    bbECS::EntityGUID ent;
    ecs.addEntity(ent)
        .addComponent<Position>(ent, {0.0, 0.0});

    ecs.addSystem(sbID, [](bbECS::ECS &ecs) {
        ecs.forEach<Position>([](Position &pos) {
            pos.x += 1;
        });
    });

    ecs.addSystem(sbID, [](bbECS::ECS &ecs) {
        ecs.forEach<Position>([](Position &pos) {
            pos.x += 1;
        });
    });

    ecs.runSystemBatch(sbID);
    if (ecs.getComponent<Position>(ent).x != 2)
    {
        std::cerr << "Error: Systems not run correctly." << std::endl;
        return false;
    }

    return true;
}

// test running systems in parallel
bool testRunSystemsInParallel()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    bbECS::EntityGUID ent1;
    ecs.addEntity(ent1)
        .addComponent<Position>(ent1, {0.0, 0.0});

    bbECS::EntityGUID ent2;
    ecs.addEntity(ent2)
        .addComponent<Velocity>(ent2, {1.0, 1.0});

    bbECS::SystemBatchID sbID = ecs.addSystemBatch();

    ecs.addSystem<Position>(sbID, [](bbECS::ECS &ecs) {
        ecs.forEach<Position>([](Position &pos) {
            pos.x += 1;
        });
    });

    ecs.addSystem<Velocity>(sbID, [](bbECS::ECS &ecs) {
        ecs.forEach<Velocity>([](Velocity &vel) {
            vel.x += 1;
        });
    });

    ecs.runSystemBatch(sbID);

    Position &pos = ecs.getComponent<Position>(ent1);
    Velocity &vel = ecs.getComponent<Velocity>(ent2);
    if (pos.x != 1.0 || vel.x != 2.0)
    {
        std::cerr << "Error: Systems not run correctly." << std::endl;
        return false;
    }

    return true;
}
// test thread safety while running systems
bool testThreadSafety()
{
    std::ostringstream out;
    std::streambuf* original = std::clog.rdbuf();

    std::clog.rdbuf(out.rdbuf());
    
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    bbECS::EntityGUID ent1;
    ecs.addEntity(ent1)
        .addComponent<Position>(ent1, {0.0, 0.0});

    bbECS::EntityGUID ent2;
    ecs.addEntity(ent2)
        .addComponent<Velocity>(ent2, {1.0, 1.0});

    bbECS::SystemBatchID sbID = ecs.addSystemBatch();

    ecs.addSystem<Position>(sbID, [](bbECS::ECS &ecs) {
        ecs.addComponent<Velocity>(bbECS::EntityID{0}, Velocity{0.0, 0.0});
        ecs.forEach<Velocity>([](Velocity&) {});
        ecs.forEach<Position>([](Position &pos) {
            pos.x += 1;
        });
    });

    ecs.addSystem<Velocity>(sbID, [](bbECS::ECS &ecs) {
        ecs.forEach<Velocity>([](Velocity &vel) {
            vel.x += 1;
        });
    });

    ecs.runSystemBatch(sbID);

    std::clog.rdbuf(original);

    if(out.str().find("ECS WARNING: ECS is restricted 'addComponent'") == std::string::npos ||
        out.str().find("ECS WARNING: Component type '249730865041013130' doesn't exist 'forEach'") == std::string::npos)
    {
        std::cout << "Error: Thread safety issue detected." << std::endl;
        return false;
    }

    return true;
}

// test read only and read write systems
bool testReadOnlyReadWriteSystems()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<State>("State");

    bbECS::EntityGUID ent1;
    ecs.addEntity(ent1)
        .addComponent<Position>(ent1, {0.0, 0.0});

    bbECS::EntityGUID ent2;
    ecs.addEntity(ent2)
        .addComponent<State>(ent2, 2);

    bbECS::SystemBatchID sbID = ecs.addSystemBatch();

    ecs.addSystem<Position>(sbID, [](bbECS::ECS &ecs) {
        ecs.readComponent<State>(bbECS::EntityID{1});
        ecs.forEach<Position>([](Position &pos) {
            pos.x += 1;
        });
    });

    ecs.setReadOnly<State>();

    ecs.runSystemBatch(sbID);

    ecs.setReadWrite<State>();


    return true;
}
// test add component and remove component systems
bool testAddRemoveComponentSystems()
{
    bbECS::ECS ecs;

    int i = 0;

    ecs.addComponentType<Position>("Position");
    ecs.addSystem<Position>(SYSTEM_ADD_COMPONENT, [&i](Position&) {
        i++;
    });
    ecs.addSystem<Position>(SYSTEM_REMOVE_COMPONENT, [&i](Position&) {
        i--;
    });

    bbECS::EntityGUID ent1;
    ecs.addEntity(ent1)
        .addComponent<Position>(ent1, {0.0, 0.0});

    ecs.removeComponent<Position>(ent1);
    if (i != 0)
    {
        std::cerr << "Error: Add/Remove component systems not run correctly." << std::endl;
        return false;
    }

    return true;
}

// test for each looping basic 
bool testForEachLooping()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");
    ecs.addComponentType<Velocity>("Velocity");

    bbECS::EntityGUID ent1;
    ecs.addEntity(ent1)
        .addComponent<Position>(ent1, {0.0, 0.0});

    bbECS::EntityGUID ent2;
    ecs.addEntity(ent2)
        .addComponent<Velocity>(ent2, {1.0, 1.0});

    ecs.forEach<Position>([](Position &pos) {
        pos.x += 1;
    });

    Position &pos = ecs.getComponent<Position>(ent1);
    if (pos.x != 1.0)
    {
        std::cerr << "Error: forEach loop not run correctly." << std::endl;
        return false;
    }

    return true;
}

// test for each looping in parallel
bool testForEachLoopingParallel()
{
    bbECS::ECS ecs;

    ecs.addComponentType<Position>("Position");

    for(int i = 0; i < 10; i++)
    {
        bbECS::EntityGUID ent1;
        ecs.addEntity(ent1)
            .addComponent<Position>(ent1, {0.0, 0.0});
    }

    ecs.forEach<Position>([](Position &pos) {
        pos.x += 1;
    }, 4);

    for(size_t i = 0; i < 10; i++)
    {
        const Position &pos = ecs.readComponent<Position>(bbECS::EntityID{i});
        if (pos.x != 1.0)
        {
            std::cerr << "Error: forEach loop not run correctly." << std::endl;
            return false;
        }
    }

    return true;
}

int main()
{
    // Run tests
    TEST_ECS(testAddRemoveEntities);
    TEST_ECS(testAddRemoveComponentTypes);
    TEST_ECS(testAddRemoveComponents);
    TEST_ECS(testAccessModifyComponents);
    TEST_ECS(testAddRemoveRelationships);
    TEST_ECS(testAddRemoveSystems);
    TEST_ECS(testRunSystemsInParallel);
    TEST_ECS(testThreadSafety);
    TEST_ECS(testReadOnlyReadWriteSystems);
    TEST_ECS(testAddRemoveComponentSystems);
    TEST_ECS(testForEachLooping);
    TEST_ECS(testForEachLoopingParallel);


    return 0;
}
