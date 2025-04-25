// ============================================
// Bear Bones ECS - v0.1.0
//
// A simple, fast, and modern C++ Entity-Component 
// System — all in a single header file!
//
// Written by Jack Evans (2025)
// License: MIT
// ============================================

#include <vector>
#include <unordered_map>
#include <random>
#include <iostream>
#include <functional>
#include <tuple>
#include <thread>
#include <unordered_set>
#include <iomanip>

// comment this line to disable warning messages
#define ECS_DEBUG

#ifdef ECS_DEBUG
#define ECS_WARNING_IF(condition, message, retval) do { if (warnIf(condition, message, __func__)) return retval; } while (0)
#else
#define ECS_WARNING_IF(condition, message, retval)
#endif

#define ECS_ERROR_IF(condition, message) errorIf(condition, message, __func__)

#define COMPONENT_TYPE_DOESNT_EXIST(x)          "Component type '" + x +  "' doesn't exist"
#define COMPONENT_TYPE_ALREADY_EXISTS(x)        "Component type '" + x +  "' already exists"
#define COMPONENT_TYPE_IS_LOCKED(x)             "Component type '" + x +  "' is locked"
#define COMPONENT_TYPE_IS_READ_ONLY(x)          "Component type '" + x +  "' is read-only"
#define ENTITY_DOESNT_EXIST(x)                  "Entity '" + x +  "' doesn't exist"
#define ENTITY_DOESNT_CONTAIN_COMPONENT(x)      "Entity doesn't contain component '" + x +  "'"
#define ENTITY_ALREADY_CONTAINS_COMPONENT(x)    "Entity already contains component '" + x +  "'"
#define ENTITY_ALREADY_EXISTS(x)                "Entity '" + x +  "' already exists"
#define ENTITY_GUID_ALREADY_EXISTS(x)           "Entity GUID '" + x +  "' already exists"
#define ENTITY_GUID_DOESNT_EXIST(x)             "Entity GUID '" + x +  "' doesn't exist"
#define ECS_IS_RESTRICTED                       "ECS is restricted"
#define SYSTEM_BATCH_DOESNT_EXIST(x)            "System batch '" + x +  "' doesn't exist"
#define MEMBER_DOESNT_EXIST(x)                  "Member '" + x + "' doesn't exist"
#define INVALID_SYSTEM_TYPE                     "Invalid system type"

#define SYSTEM_ADD_COMPONENT 1
#define SYSTEM_REMOVE_COMPONENT 2

namespace bbECS { 

class ECS;

struct EntityID {
    size_t id;

    bool operator==(const EntityID& other) const {return id == other.id;}
};

struct EntityGUID {
    uint64_t id = 0;

    bool operator==(const EntityGUID& other) const {return id == other.id;}
};

using ToStringFunc = std::string (*)(void*, ECS&, int);
using FromStringFunc = void (*)(void*, std::string, ECS&, int);

struct MemberMeta {
    size_t offset;
    size_t size;
    bool isPointer;
    int arraySize;

    ToStringFunc toString;
    FromStringFunc fromString;
};

using SystemBatchID = uint64_t;
using ComponentTypeID = size_t;
using SystemType = uint8_t;

class ECS {
    using ComponentID = size_t;

    template <typename T>
    struct Component {
        T data;
        EntityID owner;
    };

    struct Entity {
        EntityGUID guid;
        std::unordered_map<ComponentTypeID, ComponentID> componentIDs;
        EntityGUID parentGUID;
        std::vector<EntityGUID> childrenGUIDs;
    };

    struct ComponentType {
        void *storage;
        size_t size;
        size_t componentSize;
        size_t capacity;
        float growthFactor = 1.5f;

        bool isLocked = false;
        bool isReadOnly = false;
        bool isSingular = false;

        std::string name;
        std::unordered_map<std::string, MemberMeta> members;
        
        void (*addComponentFunc)(EntityID, void*, ECS&);
        void (*removeComponentFunc)(EntityID, ECS&);

        void (*removeComponentTypeFunc)(ECS&);

        ToStringFunc toString;
        FromStringFunc fromString;
    };

    struct System{
        std::vector<ComponentTypeID> componentTypeIDs;
        std::function<void(ECS&)> func;
    };

    struct SystemBatch {
        std::vector<std::vector<System>> parallelSystems;
    };

public:
    ECS();
    ~ECS();
    std::string toString(); 
    void fromString(std::string str);
    static std::string prettyFormat(const std::string& str);

    // Entity management
    ECS &addEntity();
    ECS &addEntity(EntityGUID &guid);
    ECS &removeEntity(EntityGUID entityGUID);
    ECS &removeEntity(EntityID entityID);

    ECS &addRelationship(EntityGUID parentEntityGUID, EntityGUID childEntityGUID);
    ECS &addRelationship(EntityID parentEntityID, EntityID childEntityID);
    ECS &removeChild(EntityGUID parentEntityGUID, EntityGUID childEntityGUID);
    ECS &removeChild(EntityID parentEntityID, EntityID childEntityID);
    EntityGUID getParent(EntityGUID entityGUID);
    EntityGUID getParent(EntityID entityID);
    std::vector<EntityGUID> getChildren(EntityGUID entityGUID);
    std::vector<EntityGUID> getChildren(EntityID entityID);

    const EntityID getEntityID(EntityGUID entityGUID) const;
    std::string toString(EntityGUID entityGUID);
    std::string toString(EntityID entityID);
    std::string toTemplateString(std::vector<EntityID> entityIDs);
    void fromString(EntityGUID guid, std::string str);
    void fromString(EntityID id, std::string str);
    void fromTemplateString(std::string str);

    // Component management
    void addComponent(EntityID entityID, ComponentTypeID componentTypeID, void* component);
    template <typename T> ECS &addComponent(T &&component);
    template <typename T> ECS &addComponent(EntityGUID entityGUID, T &&component);
    template <typename T> ECS &addComponent(EntityID entityID, T &&component);
    template <typename T, typename... Args> ECS &addComponent(Args&&... args);
    template <typename T, typename... Args> ECS &addComponent(EntityGUID entityGUID, Args&&... args);
    template <typename T, typename... Args> ECS &addComponent(EntityID entityID, Args&&... args);
    void removeComponent(EntityID entityID, ComponentTypeID componentTypeID);
    template <typename T> ECS &removeComponent(EntityGUID entityID);
    template <typename T> ECS &removeComponent(EntityID entityID);

    // Component access
    void* getComponent(EntityID entityID, ComponentTypeID componentTypeID);
    template <typename T> T &getComponent();
    template <typename T> T &getComponent(EntityGUID entityGUID);
    template <typename T> T &getComponent(EntityID entityID);
    template <typename T> const T &readComponent(EntityGUID entityGUID) const;
    template <typename T> const T &readComponent(EntityID entityID) const;
    template <typename T> ECS &setReadOnly();
    template <typename T> ECS &setReadWrite();
    template <typename T> ECS &setSingular();
    template <typename T> std::string toString(T &t, int arraySize = 0);
    template <typename U, typename T> std::string toString(T &t, std::string memberName);
    template <typename T> void fromString(T &t, std::string str, int arraySize = 0);
    
    // Component type management
    template <typename T> ECS &addComponentType(size_t reserve = 10);
    template <typename T> ECS &addComponentType(std::string name, size_t reserve = 10);
    template <typename T> ECS &removeComponentType();
    template <typename T> std::string getComponentTypeName();
    template <typename T> ComponentTypeID getComponentTypeID();
    template <typename T, typename MemberType>
    ECS &addMemberMeta(MemberType T::*memberPtr, std::string name, int arraySize = 0, 
        ToStringFunc toString = nullptr, FromStringFunc fromString = nullptr);
    MemberMeta getMemberMeta(std::string name, ComponentTypeID componentTypeID);
    template <typename T> MemberMeta getMemberMeta(std::string name);

    // Looping through components
    template <typename T> ECS &forEach(std::function<void(T&)> func, size_t threadCount = 1);
    template <typename T> ECS &forEach(std::function<void(EntityID, T&)> func, size_t threadCount = 1);
    template <typename T1, typename T2> ECS &forEach(std::function<void(T1&, T2&)> func, size_t threadCount = 1);
    template <typename T1, typename T2> ECS &forEach(std::function<void(EntityID, T1&, T2&)> func, size_t threadCount = 1);
    template<typename... Components, typename Func, typename = std::enable_if_t<(sizeof...(Components) > 2)>,
            std::enable_if_t<std::is_invocable_v<Func, Components&...>, long> = 0> 
    ECS &forEach(Func func, size_t threadCount = 1);
    template<typename... Components, typename Func, typename = std::enable_if_t<(sizeof...(Components) > 2)>,
            std::enable_if_t<std::is_invocable_v<Func, EntityID, Components&...>, int> = 0> 
    ECS &forEach(Func func, size_t threadCount = 1);

    // System management
    SystemBatchID addSystemBatch();
    template <typename... Components> ECS &addSystem(SystemBatchID systemBatchID, std::function<void(ECS&)> system);
    template <typename T> ECS &addSystem(SystemType systemType, std::function<void(T&)> system);
    template <typename T> ECS &addSystem(SystemType systemType, std::function<void(EntityID, T&)> system);
    template <typename T> ECS &addSystem(SystemType systemType, std::function<void(ECS&, EntityID, T&)> system);
    ECS &runSystemBatch(SystemBatchID systemBatchID);

private:
    std::string toString(EntityID entityID, EntityGUID parentGUID, std::vector<EntityGUID> childrenGUIDs);
    void fromString(EntityID id, std::string str, std::unordered_map<EntityGUID, EntityGUID> &localToGuid);
    void fromString(std::string str, std::unordered_map<EntityGUID, EntityGUID> &localToGuid);
    template <typename T> static void addComponent(EntityID entityId, void* component, ECS& ecs);
    template<typename... Components> std::tuple<Components&...> getComponents(EntityID entityId);
    std::vector<ComponentTypeID> getAllComponentTypeIDs();
    std::vector<ComponentTypeID> getParallelSystemComponentIDs(SystemBatchID id, size_t index);
    ECS &killChildren();
    template <typename... Args> ECS &split();
    ECS& split(std::vector<ComponentTypeID> componentTypesToLock);
    ECS &terminate();
    ECS &restrict();
    ECS &unrestrict();
    static EntityGUID generateGUID();
    static SystemBatchID generateSystemBatchID();
    static void refitComponentTypeStorage(ComponentType& componentType, float growthFactor);
    static bool haveCommonElements(const std::vector<ComponentTypeID>& vec1, const std::vector<ComponentTypeID>& vec2);
    static bool warnIf(bool condition, const std::string& message, const char* func);
    static bool errorIf(bool condition, const std::string& message, const char* func);
    template <typename T> static void removeComponentType_(ECS &ecs);
    template <typename T> static void removeComponent_(EntityID id, ECS &ecs);

    static std::vector<std::string> splitTopLevelCommaSections(const std::string& input);
    template <typename T> static std::string toString(void* data, ECS &ecs, int arraySize = 0);
    template <typename T> static void fromString(void* ptr, std::string str, ECS &ecs, int arraySize = 0);

    std::unordered_map<EntityGUID, EntityID> *entitiesMap; 
    std::vector<Entity> *entities;
    std::unordered_map<ComponentTypeID, ComponentType> componentTypes; 
    std::unordered_map<std::string, ComponentTypeID> componentTypeNames;
    EntityID cachedEntityID = {SIZE_MAX};

    std::unordered_map<SystemBatchID, SystemBatch> systemBatches;
    std::unordered_map<ComponentTypeID, std::function<void(ECS&, EntityID)>> addComponentSystems;
    std::unordered_map<ComponentTypeID, std::function<void(ECS&, EntityID)>> removeComponentSystems;

    bool restricted = false;
    bool isRoot = true;
    std::vector<ECS> children;

    ECS(std::unordered_map<EntityGUID, EntityID> *entitiesMap, std::vector<Entity> *entities,
        std::unordered_map<ComponentTypeID, ComponentType> componentTypes, bool restricted, bool isRoot = false);
};
}
namespace std {
    // Specialize std::hash for EntityID
    template <>
    struct hash<bbECS::EntityID> {
        size_t operator()(const bbECS::EntityID& id) const {
            return std::hash<size_t>()(id.id);
        }
    };

    // Specialize std::hash for EntityGUID
    template <>
    struct hash<bbECS::EntityGUID> {
        uint64_t operator()(const bbECS::EntityGUID& guid) const {
            return std::hash<uint64_t>()(guid.id);
        }
    };
}

namespace bbECS {

ECS::ECS() {
    entitiesMap = new std::unordered_map<EntityGUID, EntityID>();
    entities = new std::vector<Entity>();
}

ECS::ECS(std::unordered_map<EntityGUID, EntityID> *entitiesMap, std::vector<Entity> *entities,
                    std::unordered_map<ComponentTypeID, ComponentType> componentTypes, bool restricted, bool isRoot) {
    this->entitiesMap = entitiesMap;
    this->entities = entities;
    this->componentTypes = componentTypes;
    this->restricted = restricted;
    this->isRoot = isRoot;
}

ECS::~ECS() {
    if(!isRoot) return;
    terminate();
    delete entitiesMap;
    delete entities;
}

template <typename... Args>
ECS &ECS::split(){
    std::vector<ComponentTypeID> componentTypesToLock = {typeid(Args).hash_code()...};
    return split(componentTypesToLock);
}

ECS& ECS::split(std::vector<ComponentTypeID> componentTypesToLock){
    std::unordered_map<ComponentTypeID, ComponentType> newComponentTypes;
    
    for(size_t i = 0; i < componentTypesToLock.size(); i++){
        ComponentTypeID typeID = componentTypesToLock[i];

        auto componentTypeIt = componentTypes.find(typeID);
        ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);
        
        ComponentType &componentType = componentTypeIt->second;
        ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);
        
        newComponentTypes[typeID] = componentType;
    }

    for(auto &componentType : componentTypes){
        if(componentType.second.isReadOnly){
            newComponentTypes[componentType.first] = componentType.second;
        }
    }

    for(size_t i = 0; i < componentTypesToLock.size(); i++){
        ComponentType &componentType = componentTypes.at(componentTypesToLock[i]);
        componentType.isLocked = true;
    }

    restrict();
    children.push_back(ECS(entitiesMap, entities, newComponentTypes, true));
    return children.back();
}

ECS &ECS::killChildren(){
    children.clear();
    
    for(auto &componentType : componentTypes){
        componentType.second.isLocked = false;
    }
    
    unrestrict();
    return *this;
}

ECS &ECS::addEntity() {
    EntityGUID guid;
    addEntity(guid);
    return *this;
}

ECS &ECS::addEntity(EntityGUID &guid) {
    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    if(guid.id == 0){
        guid = generateGUID();
    }

    ECS_WARNING_IF(entitiesMap->find(guid) != entitiesMap->end(), ENTITY_GUID_ALREADY_EXISTS(std::to_string(guid.id)), *this);

    entities->push_back(Entity{.guid = guid});
    EntityID entityId{entities->size() - 1};
    (*entitiesMap)[(*entities)[entityId.id].guid] = entityId;
    cachedEntityID = entityId;
    return *this;
}

ECS &ECS::removeEntity(EntityGUID entityId) {
    removeEntity(getEntityID(entityId));
    return *this;
}

ECS &ECS::removeEntity(EntityID entityId) {
    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);
    ECS_WARNING_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)), *this);

    std::vector<ComponentTypeID> componentsToRemove;

    for (const auto& componentID : (*entities)[entityId.id].componentIDs) {
        componentsToRemove.push_back(componentID.first);
    }

    for (int i = componentsToRemove.size() - 1; i >= 0; i--) {
        componentTypes.at(componentsToRemove.at(i)).removeComponentFunc(entityId, *this);
    }

    EntityGUID guid = (*entities)[entityId.id].guid;
    (*entities)[entityId.id] = (*entities)[entities->size() - 1];
    (*entitiesMap)[(*entities)[entityId.id].guid] = entityId;
    entitiesMap->erase(guid);
    entities->pop_back();

    cachedEntityID = entityId;

    return *this;
}

ECS &ECS::addRelationship(EntityGUID parent, EntityGUID child){
    ECS_WARNING_IF(entitiesMap->find(parent) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(parent.id)), *this);
    ECS_WARNING_IF(entitiesMap->find(child) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(child.id)), *this);

    return addRelationship(getEntityID(parent), getEntityID(child));
}
ECS &ECS::addRelationship(EntityID parent, EntityID child){
    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(parent.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(parent.id)), *this);

    ECS_WARNING_IF(child.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(child.id)), *this);

    ECS_WARNING_IF((*entities).at(parent.id).parentGUID == (*entities)[child.id].guid, 
                            "child entity is parent entity's parent", *this);

    ECS_WARNING_IF(parent == child, "parent and child are the same entity", *this);

    Entity &childEntity = (*entities)[child.id];
    Entity &parentEntity = (*entities)[parent.id];

    ECS_WARNING_IF(childEntity.parentGUID.id != 0, "child entity already has a parent", *this);

    parentEntity.childrenGUIDs.push_back(childEntity.guid);
    childEntity.parentGUID = parentEntity.guid;

    return *this;
}

ECS &ECS::removeChild(EntityGUID parentEntityGUID, EntityGUID childEntityGUID) {
    ECS_WARNING_IF(entitiesMap->find(parentEntityGUID) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(parentEntityGUID.id)), *this);

    ECS_WARNING_IF(entitiesMap->find(childEntityGUID) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(childEntityGUID.id)), *this);

    return removeChild(getEntityID(parentEntityGUID), getEntityID(childEntityGUID));
}

ECS &ECS::removeChild(EntityID parentEntityID, EntityID childEntityID){
    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(parentEntityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(parentEntityID.id)), *this);

    ECS_WARNING_IF(childEntityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(childEntityID.id)), *this);

    ECS_WARNING_IF((*entities).at(parentEntityID.id).parentGUID == (*entities)[childEntityID.id].guid, 
                            "child entity is parent entity's parent", *this);

    ECS_WARNING_IF(parentEntityID == childEntityID, "parent and child are the same entity", *this);

    Entity childEntity = (*entities)[childEntityID.id];
    Entity parentEntity = (*entities)[parentEntityID.id];

    ECS_WARNING_IF(childEntity.parentGUID.id == 0, "child entity doesn't have a parent", *this);

    ECS_WARNING_IF(parentEntity.guid != childEntity.parentGUID, "child entity is not a child of parent entity", *this);

    auto it = std::find(parentEntity.childrenGUIDs.begin(), parentEntity.childrenGUIDs.end(), childEntity.guid);
    parentEntity.childrenGUIDs.erase(it);
    childEntity.parentGUID = EntityGUID{0};

    return *this;
}

EntityGUID ECS::getParent(EntityGUID entityGUID){
    ECS_WARNING_IF(entitiesMap->find(entityGUID) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(entityGUID.id)), EntityGUID{0});

    return getParent(getEntityID(entityGUID));
}
EntityGUID ECS::getParent(EntityID entityID){
    ECS_WARNING_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityID.id)), EntityGUID{0});

    return (*entities)[entityID.id].parentGUID;
}
std::vector<EntityGUID> ECS::getChildren(EntityGUID entityGUID){
    ECS_WARNING_IF(entitiesMap->find(entityGUID) == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(entityGUID.id)), {});

    return getChildren(getEntityID(entityGUID));
}
std::vector<EntityGUID> ECS::getChildren(EntityID entityID){
    ECS_WARNING_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityID.id)), {});

    return (*entities)[entityID.id].childrenGUIDs;
}

const EntityID ECS::getEntityID(EntityGUID guid) const{
    auto entityIt = entitiesMap->find(guid);
    ECS_WARNING_IF(entityIt == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST(std::to_string(guid.id)), EntityID{SIZE_MAX});

    return entityIt->second;
}

void ECS::addComponent(EntityID entityId, ComponentTypeID typeID, void* component){
    ECS_ERROR_IF(restricted, ECS_IS_RESTRICTED);

    ECS_ERROR_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)));

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)));

    ComponentType& componentType = componentTypeIt->second;

    componentType.addComponentFunc(entityId, component, *this);
}

template <typename T> void ECS::addComponent(EntityID entityId, void* component, ECS& ecs){
    ecs.addComponent<T>(entityId, *static_cast<T*>(component));
}

template <typename T, typename... Args>
ECS &ECS::addComponent(Args&&... args) {
    addComponent<T>(cachedEntityID, std::forward<Args>(args)...);
    return *this;
}

template <typename T> ECS &ECS::addComponent(T &&component){
    return addComponent<T>(cachedEntityID, component);
}

template <typename T> ECS &ECS::addComponent(EntityGUID entityId, T &&component){
    return addComponent<T>(getEntityID(entityId), component);
}
template <typename T> ECS &ECS::addComponent(EntityID entityId, T &&component){
    return addComponent<T>(entityId, component);
}   

template <typename T, typename... Args>
ECS &ECS::addComponent(EntityGUID entityId, Args&&... args) {
    addComponent<T>(getEntityID(entityId), std::forward<Args>(args)...);
    return *this;
}

template <typename T, typename... Args>
ECS &ECS::addComponent(EntityID entityId, Args&&... args) {
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)), *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;

    ECS_WARNING_IF(componentType.isSingular && componentType.size > 0, 
                "Singular component already exists '" + componentType.name + "'", *this);

    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    ECS_WARNING_IF((*entities).at(entityId.id).componentIDs.find(typeID) != (*entities).at(entityId.id).componentIDs.end(),
                         ENTITY_ALREADY_CONTAINS_COMPONENT(componentType.name), *this);

    if (componentType.size >= componentType.capacity) {
        refitComponentTypeStorage(componentType, componentType.growthFactor);
    }

    Component<T>* componentStorage = static_cast<Component<T>*>(componentType.storage);
    new (&componentStorage[componentType.size]) Component<T>{T{std::forward<Args>(args)...}, entityId}; 

    (*entities).at(entityId.id).componentIDs[typeID] = componentType.size;
    componentType.size++;

    if (addComponentSystems.find(typeID) != addComponentSystems.end()) {
        addComponentSystems.at(typeID)(*this, entityId);
    }

    return *this;
}

void ECS::removeComponent(EntityID entityId, ComponentTypeID typeID){
    ECS_ERROR_IF(restricted, ECS_IS_RESTRICTED);

    ECS_ERROR_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)));

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)));

    ComponentType& componentType = componentTypeIt->second;

    auto componentIndexIt = (*entities).at(entityId.id).componentIDs.find(typeID);
    ECS_ERROR_IF(componentIndexIt == (*entities).at(entityId.id).componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT(componentType.name));

    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name));

    componentType.removeComponentFunc(entityId, *this);
}

template <typename T>
ECS &ECS::removeComponent(EntityGUID entityID) {
    removeComponent<T>(getEntityID(entityID));
    return *this;
}

template <typename T>
ECS &ECS::removeComponent(EntityID entityID) {
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityID.id)), *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;

    auto componentIndexIt = (*entities).at(entityID.id).componentIDs.find(typeID);
    ECS_WARNING_IF(componentIndexIt == (*entities).at(entityID.id).componentIDs.end(), 
                        ENTITY_DOESNT_CONTAIN_COMPONENT(componentType.name), *this);

    ComponentID componentID = componentIndexIt->second;

    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    if (removeComponentSystems.find(typeID) != removeComponentSystems.end()) {
        removeComponentSystems.at(typeID)(*this, entityID);
    }

    Component<T>* componentStorage = static_cast<Component<T>*>(componentType.storage);

    componentStorage->data.~T();

    EntityID lastEntityID = componentStorage[componentType.size - 1].owner;
    (*entities).at(lastEntityID.id).componentIDs.at(typeID) = componentID;

    componentStorage[componentID] = componentStorage[componentType.size - 1];
    componentType.size--;

    if (componentType.size < componentType.capacity / componentType.growthFactor) {
        refitComponentTypeStorage(componentType, 1.0f / componentType.growthFactor);
    }

    (*entities).at(entityID.id).componentIDs.erase(typeID);

    return *this;
}

template<typename... Components>
std::tuple<Components&...> ECS::getComponents(EntityID entityId) {
    return std::tuple<Components&...>{getComponent<Components>(entityId)...};
}

template <typename T> T &ECS::getComponent(){
    ComponentTypeID typeID = typeid(T).hash_code();

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)));

    ComponentType& componentType = componentTypeIt->second;

    ECS_ERROR_IF(componentType.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY(componentType.name));
    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name));

    Component<T>* componentPtrCasted = static_cast<Component<T>*>(componentType.storage);

    return componentPtrCasted->data;
}

template <typename T>
T &ECS::getComponent(EntityGUID entityId) {
    return getComponent<T>(getEntityID(entityId));
}

template <typename T>
T &ECS::getComponent(EntityID entityId) {
    ComponentTypeID typeID = typeid(T).hash_code();

    void* componentPtr = getComponent(entityId, typeID);

    Component<T>* componentPtrCasted = static_cast<Component<T>*>(componentPtr);

    return componentPtrCasted->data;
}

void* ECS::getComponent(EntityID entityId, ComponentTypeID typeID){
    ECS_ERROR_IF(entityId.id > entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)));

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)));

    ComponentType& componentType = componentTypeIt->second;

    auto componentIndexIt = (*entities).at(entityId.id).componentIDs.find(typeID);
    ECS_ERROR_IF(componentIndexIt == (*entities).at(entityId.id).componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT(componentType.name));

    ComponentID componentID = componentIndexIt->second;

    ECS_ERROR_IF(componentType.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY(componentType.name));
    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name));

    char* componentStorage = static_cast<char*>(componentType.storage);
    char* componentPtr = componentStorage + (componentID * componentType.componentSize);

    return (void*)componentPtr;
}

template <typename T> const T &ECS::readComponent(EntityGUID entityId) const{
    return readComponent<T>(getEntityID(entityId));
}
template <typename T> const T &ECS::readComponent(EntityID entityId) const{
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_ERROR_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityId.id)));

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)));

    const ComponentType& componentType = componentTypeIt->second;

    const auto& entity = (*entities)[entityId.id];
    auto componentIndexIt = entity.componentIDs.find(typeID);
    ECS_ERROR_IF(componentIndexIt == entity.componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT(componentType.name));

    ComponentID componentID = componentIndexIt->second;

    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name));

    const Component<T>* componentStorage = static_cast<const Component<T>*>(componentType.storage);
    return componentStorage[componentID].data;
}

template <typename T> ECS &ECS::setReadOnly(){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    componentType.isReadOnly = true;

    return *this;
}

template <typename T> ECS &ECS::setReadWrite(){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    componentType.isReadOnly = false;

    return *this;
}

template <typename T> ECS &ECS::setSingular(){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    ECS_WARNING_IF(componentType.size > 1, "More than one component already exists '" + componentType.name + "'", *this);

    componentType.isSingular = true;

    return *this;
}

template <typename T> std::string ECS::toString(T &t, int arraySize){
    return toString<T>((void*)&t, *this, arraySize);
}

template <typename U, typename T> std::string ECS::toString(T &t, std::string memberName){
    MemberMeta member = getMemberMeta<T>(memberName);
    void* memberPtr = reinterpret_cast<uint8_t*>(&t) + member.offset;
    return toString<U>(memberPtr, *this, member.arraySize);
}

template <typename T> void ECS::fromString(T &t, std::string str, int arraySize){
    fromString<T>((void*)&t, str, *this, arraySize);
}


template <typename T> ECS &ECS::addComponentType(size_t reserve){
    return addComponentType<T>(typeid(T).name(), reserve);
}

template <typename T>
ECS &ECS::addComponentType(std::string name, size_t reserve) {
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(componentTypes.find(typeID) != componentTypes.end(), COMPONENT_TYPE_ALREADY_EXISTS(std::to_string(typeID)), *this);

    void* storage = new uint8_t[reserve * sizeof(Component<T>)];

    componentTypes[typeID] = {
        .storage = storage,
        .componentSize = sizeof(Component<T>),
        .size = 0,
        .capacity = reserve,
        .addComponentFunc = addComponent<T>,
        .removeComponentTypeFunc = removeComponentType_<T>,
        .removeComponentFunc = removeComponent_<T>,
        .toString = toString<T>,
        .fromString = fromString<T>,
        .name = name,
    };

    componentTypeNames[name] = typeID;

    return *this;
}

template <typename T>
ECS &ECS::removeComponentType() {
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;
    delete[] static_cast<uint8_t*>(componentType.storage);
    componentType.storage = nullptr;

    componentTypes.erase(typeID);
    return *this;
}

template <typename T> std::string ECS::getComponentTypeName(){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, "");

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), "");

    ComponentType& componentType = componentTypeIt->second;

    return componentType.name;
}

template <typename T> ComponentTypeID ECS::getComponentTypeID(){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, 0);

    ECS_WARNING_IF(componentTypes.find(typeID) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), 0);

    return typeID;
}

template <typename T, typename MemberType>
ECS &ECS::addMemberMeta(MemberType T::*memberPtr, std::string name, int arraySize_, 
    ToStringFunc toString_, FromStringFunc fromString_) {

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ComponentTypeID componentTypeID = typeid(T).hash_code();
    auto componentTypeIt = componentTypes.find(componentTypeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(componentTypeID)), *this);

    ComponentType& componentType = componentTypeIt->second;

    MemberMeta member{
        .offset = reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*memberPtr)),
        .size = sizeof(MemberType),
        .isPointer = std::is_pointer<MemberType>::value,
        .arraySize = arraySize_,
    };

    if(fromString_ == nullptr){
        member.fromString = fromString<MemberType>;
    }else{
        member.fromString = fromString_;
    }

    if(toString_ == nullptr){
        member.toString = toString<MemberType>;
    }else{
        member.toString = toString_;
    }

    componentType.members.insert({name, member});

    return *this;
}

MemberMeta ECS::getMemberMeta(std::string name, ComponentTypeID typeID){
    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), MemberMeta{});

    ComponentType& componentType = componentTypeIt->second;

    auto memberIt = componentType.members.find(name);
    ECS_WARNING_IF(memberIt == componentType.members.end(), MEMBER_DOESNT_EXIST(name), MemberMeta{});

    return memberIt->second;
}

template <typename T> MemberMeta ECS::getMemberMeta(std::string name){
    return getMemberMeta(name, typeid(T).hash_code());
}

template <typename T>
ECS &ECS::forEach(std::function<void(T&)> func, size_t threadCount) {
    auto wrappedFunc = [func](EntityID, T& component) {
        func(component);
    };

    forEach<T>(wrappedFunc, threadCount);

    return *this;
}

template <typename T>
ECS &ECS::forEach(std::function<void(EntityID, T&)> func, size_t threadCount) {
    ComponentTypeID typeID = typeid(T).hash_code();

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);

    ComponentType& componentType = componentTypeIt->second;

    ECS_WARNING_IF(componentType.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY(componentType.name), *this);
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED(componentType.name), *this);

    Component<T>* componentStorage = static_cast<Component<T>*>(componentType.storage);
    size_t totalSize = componentType.size;

    threadCount = std::min(threadCount, totalSize);

    if(threadCount <= 1){
        for (size_t i = 0; i < componentType.size; i++) {
            func(componentStorage[i].owner, componentStorage[i].data);
        }

        return *this;
    }

    restrict();

    std::vector<std::thread> threads;
    size_t chunkSize = totalSize / threadCount;
    size_t remainder = totalSize % threadCount;

    size_t start = 0;
    for (size_t i = 0; i < threadCount; i++) {
        size_t end = start + chunkSize + (i < remainder ? 1 : 0); // Distribute remainder

        threads.emplace_back([=]() {
            for (size_t j = start; j < end; j++) {
                func(componentStorage[j].owner, componentStorage[j].data);
            }
        });

        start = end;
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    unrestrict();

    return *this;
}

template <typename Component1, typename Component2>
ECS &ECS::forEach(std::function<void(Component1&, Component2&)> func, size_t threadCount) {
    auto wrappedFunc = [func](EntityID, Component1& component1, Component2& component2) {
        func(component1, component2);
    };

    forEach<Component1,Component2>(wrappedFunc, threadCount);

    return *this;
}

template <typename Component1, typename Component2>
ECS &ECS::forEach(std::function<void(EntityID, Component1&, Component2&)> func, size_t threadCount) {
    ComponentTypeID typeID1 = typeid(Component1).hash_code();

    auto componentTypeIt1 = componentTypes.find(typeID1);
    ECS_WARNING_IF(componentTypeIt1 == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID1)), *this);

    ComponentTypeID typeID2 = typeid(Component2).hash_code();

    ECS_WARNING_IF(componentTypes.find(typeID2) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID2)), *this);

    ComponentType& componentType1 = componentTypeIt1->second;

    ECS_WARNING_IF(componentType1.isReadOnly || componentTypes.find(typeID2)->second.isReadOnly, 
                        COMPONENT_TYPE_IS_READ_ONLY(componentType1.name), *this);
    ECS_WARNING_IF(componentType1.isLocked || componentTypes.find(typeID2)->second.isLocked, 
                        COMPONENT_TYPE_IS_LOCKED(componentType1.name), *this);

    Component<Component1>* componentStorage1 = static_cast<Component<Component1>*>(componentType1.storage);
    size_t totalSize = componentType1.size;

    threadCount = std::min(threadCount, totalSize);

    if(threadCount <= 1){
        for (size_t i = 0; i < componentType1.size; i++) {
            EntityID entityID{componentStorage1[i].owner};

            if((*entities).at(entityID.id).componentIDs.find(typeID2) != (*entities).at(entityID.id).componentIDs.end()) {
                func(entityID, componentStorage1[i].data, getComponent<Component2>(entityID));
            }
        }

        return *this;
    }

    restrict();

    std::vector<std::thread> threads;
    size_t chunkSize = totalSize / threadCount;
    size_t remainder = totalSize % threadCount;

    size_t start = 0;
    for (size_t i = 0; i < threadCount; i++) {
        size_t end = start + chunkSize + (i < remainder ? 1 : 0); // Distribute remainder

        threads.emplace_back([=]() {
            for (size_t j = start; j < end; j++) {
                EntityID entityID{componentStorage1[j].owner};

                if((*entities).at(entityID.id).componentIDs.find(typeID2) != (*entities).at(entityID.id).componentIDs.end()) {
                    func(entityID, componentStorage1[j].data, getComponent<Component2>(entityID));
                }
            }
        });

        start = end;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    unrestrict();

    return *this;
}

template<typename... Components, typename Func, typename ,
            std::enable_if_t<std::is_invocable_v<Func, Components&...>, long>>
ECS &ECS::forEach(Func func, size_t threadCount) {
    // Create a wrapper that adds the EntityID but ignores it when calling `func`
    auto wrapper = [func](EntityID, Components&... components) {
        func(components...);
    };

    // Call the existing forEach function that includes EntityID
    return forEach<Components...>(wrapper, threadCount);
}

template<typename... Components, typename Func, typename ,
            std::enable_if_t<std::is_invocable_v<Func, EntityID, Components&...>, int>>
ECS &ECS::forEach(Func func, size_t threadCount) {

    std::vector<ComponentTypeID> componentTypesToIterate = {typeid(Components).hash_code()...};

    for (size_t i = 0; i < componentTypesToIterate.size(); i++) {
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i)) == componentTypes.end(), 
                            COMPONENT_TYPE_DOESNT_EXIST(std::to_string(componentTypesToIterate.at(i))), *this);
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i))->second.isReadOnly, 
                            COMPONENT_TYPE_IS_READ_ONLY(componentTypes.find(componentTypesToIterate.at(i))->second.name), *this);
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i))->second.isLocked, 
                            COMPONENT_TYPE_IS_LOCKED(componentTypes.find(componentTypesToIterate.at(i))->second.name), *this);
    }

    using TypeToUse = typename std::tuple_element<0, std::tuple<Components...>>;

    ComponentType &componentTypeToUse = componentTypes.at(componentTypesToIterate.at(0));

    Component<TypeToUse>* componentTypeToUseStorage = static_cast<Component<TypeToUse>*>(componentTypeToUse.storage);
    size_t totalSize = componentTypeToUse.size;

    threadCount = std::min(threadCount, totalSize);
    
    if(threadCount <= 1){
        for (size_t i = 0; i < componentTypeToUse.size; i++) {
            EntityID entityID = componentTypeToUseStorage[i].owner;

            bool hasAllComponents = true;
            for (size_t j = 0; j < componentTypesToIterate.size(); j++) {
                ComponentTypeID typeID = componentTypesToIterate.at(j);
                if ((*entities).at(entityID.id).componentIDs.find(typeID) == (*entities).at(entityID.id).componentIDs.end()) {
                    hasAllComponents = false;
                    break;
                }
            }

            if (hasAllComponents) {
                std::tuple<Components&...> components = getComponents<Components...>(entityID);

                auto extendedTuple = std::tuple_cat(std::make_tuple(entityID), components);

                std::apply(func, extendedTuple);
            }
        }

        return *this;
    }

    restrict();

    std::vector<std::thread> threads;
    size_t chunkSize = totalSize / threadCount;
    size_t remainder = totalSize % threadCount;
    
    size_t start = 0;
    for (size_t i = 0; i < threadCount; i++) {
        size_t end = start + chunkSize + (i < remainder ? 1 : 0); // Distribute remainder

        threads.emplace_back([=]() {
            for (size_t j = start; j < end; j++) {
                EntityID entityID = componentTypeToUseStorage[j].owner;

                bool hasAllComponents = true;
                for (size_t k = 0; k < componentTypesToIterate.size(); k++) {
                    ComponentTypeID typeID = componentTypesToIterate.at(k);
                    if ((*entities).at(entityID.id).componentIDs.find(typeID) == (*entities).at(entityID.id).componentIDs.end()) {
                        hasAllComponents = false;
                        break;
                    }
                }

                if (hasAllComponents) {
                    std::tuple<Components&...> components = getComponents<Components...>(entityID);

                    auto extendedTuple = std::tuple_cat(std::make_tuple(entityID), components);

                    std::apply(func, extendedTuple);
                }
            }
        });

        start = end;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    unrestrict();

    return *this;
}

SystemBatchID ECS::addSystemBatch() {
    ECS_ERROR_IF(restricted, ECS_IS_RESTRICTED);

    SystemBatchID id = generateSystemBatchID();
    systemBatches[id] = SystemBatch();
    return id;
};

template <typename... Components>
ECS &ECS::addSystem(SystemBatchID batchID, std::function<void(ECS&)> func) {

    auto systemBatchIt = systemBatches.find(batchID);
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST(std::to_string(batchID)), *this);

    SystemBatch& systemBatch = systemBatchIt->second;

    std::vector<ComponentTypeID> componentTypeIDs = {typeid(Components).hash_code()...};

    if(componentTypeIDs.size() == 0){
        componentTypeIDs = getAllComponentTypeIDs();
    }

    for(size_t i = 0; i < componentTypeIDs.size(); i++){
        ECS_WARNING_IF(componentTypes.find(componentTypeIDs[i]) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(componentTypeIDs[i])), *this);
    }

    for(size_t j = 0; j < systemBatch.parallelSystems.size(); j++){
        std::vector<ComponentTypeID> parallelSystemComponentIDs = getParallelSystemComponentIDs(batchID, j);

        if(!haveCommonElements(parallelSystemComponentIDs, componentTypeIDs)){
            systemBatch.parallelSystems.at(j).push_back({componentTypeIDs, func});

            return *this;
        }
    }

    systemBatch.parallelSystems.push_back({{componentTypeIDs, func}});

    return *this;
}

template <typename T> ECS &ECS::addSystem(uint8_t systemType, std::function<void(T&)> func){
    auto newFunc = [func](ECS&, EntityID, T& component) {
        func(component);
    };

    return addSystem<T>(systemType, newFunc);
}
template <typename T> ECS &ECS::addSystem(uint8_t systemType, std::function<void(EntityID, T&)> func){
    auto newFunc = [func](ECS&, EntityID entityID, T& component) {
        func(entityID, component);
    };

    return addSystem<T>(systemType, newFunc);
}

template <typename T> ECS &ECS::addSystem(uint8_t systemType, std::function<void(ECS&, EntityID, T&)> func){
    ComponentTypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);
    ECS_WARNING_IF(componentTypes.find(typeID) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST(std::to_string(typeID)), *this);
    ECS_WARNING_IF(componentTypes.find(typeID)->second.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY(componentTypes.find(typeID)->second.name), *this);
    ECS_WARNING_IF(componentTypes.find(typeID)->second.isLocked, COMPONENT_TYPE_IS_LOCKED(componentTypes.find(typeID)->second.name), *this);

    if(systemType == SYSTEM_ADD_COMPONENT){
        auto newFunc = [func](ECS& ecs, EntityID entityID) {
            T& component = ecs.getComponent<T>(entityID);
            func(ecs, entityID, component);
        };
        addComponentSystems.insert({typeID, newFunc});
    } else if(systemType == SYSTEM_REMOVE_COMPONENT){
        auto newFunc = [func](ECS& ecs, EntityID entityID) {
            T& component = ecs.getComponent<T>(entityID);
            func(ecs, entityID, component);
        };
        removeComponentSystems.insert({typeID, newFunc});
    } else{
        ECS_WARNING_IF(true, INVALID_SYSTEM_TYPE, *this);
    }

    return *this;
}

ECS &ECS::runSystemBatch(SystemBatchID id){
    auto systemBatchIt = systemBatches.find(id);
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST(std::to_string(id)), *this);

    SystemBatch &systemBatch = systemBatchIt->second;

    for(size_t i = 0; i < systemBatch.parallelSystems.size(); i++){
        std::vector<System> &parallelSystem = systemBatch.parallelSystems.at(i);

        std::vector<std::thread> threads;
        std::vector<ECS> ecss;

        for(size_t j = 0; j < parallelSystem.size(); j++){
            ecss.push_back(split(parallelSystem.at(j).componentTypeIDs));
        }

        for(size_t j = 0; j < parallelSystem.size(); j++){
            System &system = parallelSystem.at(j);

            ECS &splitECS = ecss.at(j);

            threads.emplace_back([&system, &splitECS]() {
                system.func(splitECS);
            });
        }

        for(auto &thread : threads){
            thread.join();
        }

        killChildren();
    }

    return *this;
}

std::vector<ComponentTypeID> ECS::getAllComponentTypeIDs() {
    std::vector<ComponentTypeID> componentTypeIDs;

    for (const auto& componentTypeID : componentTypes) {
        componentTypeIDs.push_back(componentTypeID.first);
    }

    return componentTypeIDs;
}

std::vector<ComponentTypeID> ECS::getParallelSystemComponentIDs(SystemBatchID id, size_t index){
    std::vector<ComponentTypeID> componentTypeIDs;

    auto systemBatchIt = systemBatches.find(id);
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST(std::to_string(id)), componentTypeIDs);

    SystemBatch &systemBatch = systemBatchIt->second;

    std::vector<System> &parallelSystem = systemBatch.parallelSystems.at(index);

    for(size_t i = 0; i < parallelSystem.size(); i++){
        System &system = parallelSystem.at(i);

        for(size_t j = 0; j < system.componentTypeIDs.size(); j++){
            componentTypeIDs.push_back(parallelSystem.at(i).componentTypeIDs.at(j));
        }
    }

    return componentTypeIDs;
}

bool ECS::haveCommonElements(const std::vector<ComponentTypeID>& vec1, const std::vector<ComponentTypeID>& vec2) {
    for (ComponentTypeID id1 : vec1) {
        for (ComponentTypeID id2 : vec2) {
            if (id1 == id2) {
                return true; // Found a common element
            }
        }
    }

    return false; // No common elements
}

void ECS::refitComponentTypeStorage(ComponentType& componentType, float growthFactor) {
    size_t newCapacity = std::ceil(componentType.capacity * growthFactor);
    void* newStorage = new uint8_t[newCapacity * componentType.componentSize];

    memcpy(newStorage, componentType.storage, componentType.componentSize * componentType.size);

    delete[] static_cast<uint8_t*>(componentType.storage);

    componentType.storage = newStorage;
    componentType.capacity = newCapacity;
}

EntityGUID ECS::generateGUID() {
    std::random_device rd;
    return EntityGUID{rd()};
}

SystemBatchID ECS::generateSystemBatchID() {
    std::random_device rd;
    return rd();
}

ECS &ECS::terminate() {
    for (int i = entities->size() - 1; i >= 0; i--) {
        removeEntity(EntityID{(size_t)i});
    }

    std::vector<ComponentTypeID> componentTypesToRemove;

    for (auto& componentType : componentTypes) {
        componentTypesToRemove.push_back(componentType.first);
    }

    for (int i = componentTypesToRemove.size() - 1; i >= 0; i--) {
        componentTypes.at(componentTypesToRemove.at(i)).removeComponentTypeFunc(*this);
    }

    return *this;
}

ECS &ECS::restrict() {
    restricted = true;
    return *this;
}

ECS &ECS::unrestrict() {
    restricted = false;
    return *this;
}

template <typename T>
void ECS::removeComponentType_(ECS &ecs) {
    ecs.removeComponentType<T>();
}

template <typename T>
void ECS::removeComponent_(EntityID id, ECS &ecs) {
    ecs.removeComponent<T>(id);
}

bool ECS::warnIf(bool condition, const std::string& message, const char* func){
    if (condition) {
        std::clog << "ECS WARNING: " << message;
        std::clog << " '" << func << "'" << std::endl;
        return true;
    }
    return false;
}
bool ECS::errorIf(bool condition, const std::string& message, const char* func){
    if (condition) {
        std::cerr << "ECS ERROR: " << message;
        std::cerr << " '" << func << "'" << std::endl;
        std::abort();
        return true;
    }
    return false;
}

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
struct is_unordered_map : std::false_type {};

template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Alloc>
struct is_unordered_map<std::unordered_map<Key, Value, Hash, KeyEqual, Alloc>> : std::true_type {};

template <typename T>
std::string ECS::toString(void* data, ECS &ecs, int arraySize){
    const T& value = *static_cast<T*>(data);

    ComponentTypeID typeID = typeid(T).hash_code();
    auto componentTypeIt = ecs.componentTypes.find(typeID);

    if constexpr (std::is_same_v<T, std::string>) {
        return "\"" + value + "\"";
    }
    else if constexpr (std::is_same_v<T, char>) {
        return "\"" + std::to_string(value) + "\"";
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    }
    else if constexpr (std::is_same_v<T, EntityGUID>) {
        return std::to_string(value.id);
    }
    else if constexpr (std::is_same_v<T, EntityID>) {
        return std::to_string(value.id);
    }
    else if constexpr (std::is_arithmetic_v<T>) {
        std::ostringstream oss;
        oss << std::setprecision(15) << value;
        return oss.str();
    }
    else if constexpr (std::is_array_v<T>) {
        std::string result = "[";
        constexpr std::size_t N = std::extent_v<T>;
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) result += ", ";
            using ElementType = std::remove_extent_t<T>;
            const ElementType& element = value[i];
            result += toString<ElementType>((void*)&element, ecs);
        }
        result += "]";
        return result;
    }
    else if constexpr (is_vector<T>::value) {
        std::string result = "[";
        for (size_t i = 0; i < value.size(); ++i) {
            if (i > 0) result += ", ";
            result += toString<typename T::value_type>((void*)&value[i], ecs);
        }
        result += "]";
        return result;
    }
    else if constexpr (is_unordered_map<T>::value) {
        std::string result = "{";
        bool first = true;
        for (const auto& pair : value) {
            if (!first) result += " ";
            first = false;
            result += toString<typename T::key_type>((void*)&pair.first, ecs);
            result += ": ";
            result += toString<typename T::mapped_type>((void*)&pair.second, ecs);
        }
        return result += "}";
    }
    else if constexpr (std::is_pointer<T>::value) {
        using Pointee = typename std::remove_pointer<T>::type;
        if (value == nullptr) {
            return "null";
        }
        if(arraySize == 0){
            return toString<Pointee>((void*)value, ecs);
        }

        std::string result = "[";
        for(int i = 0; i < arraySize; i++){
            if (i > 0) {
                result += ", ";
            }
            void* element = reinterpret_cast<uint8_t*>(value) + i * sizeof(Pointee);
            result += toString<Pointee>(element, ecs);
        }
        result += "]";
        return result;
    }
    else if (componentTypeIt != ecs.componentTypes.end()) {
        ComponentType& componentType = componentTypeIt->second;
        std::string result = "{";

        bool first = true;
        for (auto memberPair : componentType.members) {
            const MemberMeta& member = memberPair.second;
            if (!first) {
                result += ", ";
            }
            first = false;
            std::string memberName = memberPair.first;
            void* memberPtr = reinterpret_cast<uint8_t*>(data) + member.offset;
            std::string memberValue = member.toString(memberPtr, ecs, member.arraySize);
            result += memberName + ": " + memberValue;
        }
        result += "}";
        return result;
    }
    else {
        return "Unknown type";
    }
}

std::string ECS::toString(EntityGUID guid){
    return toString(getEntityID(guid));
}
std::string ECS::toString(EntityID entityID){
    ECS_ERROR_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityID.id)));

    Entity &entity = (*entities)[entityID.id];
    return toString(entityID, entity.parentGUID, entity.childrenGUIDs);
}
std::string ECS::toString(EntityID entityID, EntityGUID parentGUID, std::vector<EntityGUID> childrenGUIDs){
    ECS_ERROR_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(entityID.id)));

    Entity &entity = (*entities)[entityID.id];

    std::string result = "{";

    result += "parent: " + toString<EntityGUID>(parentGUID);
    result += ", ";
    result += "children: " + toString<std::vector<EntityGUID>>(childrenGUIDs);

    for(auto &componentID : entity.componentIDs){
        result += ", ";

        ComponentTypeID typeID = componentID.first;
        void* componentPtr = getComponent(entityID, typeID);

        ComponentType &componentType = componentTypes.at(typeID);

        std::string componentString = componentType.toString(componentPtr, *this, 0);
        result += componentType.name + ": " + componentString;
    }

    return result += "}";
}

std::string ECS::toTemplateString(std::vector<EntityID> entityIDs){
    std::unordered_map<EntityGUID, EntityGUID> guidToLocal;

    guidToLocal[EntityGUID{0}] = EntityGUID{0};

    for (size_t i = 0; i < entityIDs.size(); i++) {
        Entity &entity = (*entities)[entityIDs.at(i).id];
        guidToLocal[entity.guid] = EntityGUID{i + 1};
    }

    std::string result = "{";

    for (size_t i = 0; i < entityIDs.size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        Entity &entity = (*entities)[entityIDs[i].id];
        EntityGUID localID = guidToLocal.at(entity.guid);

        EntityGUID localParentID = guidToLocal.at(entity.parentGUID);
        std::vector<EntityGUID> localChildrenGUIDs;
        for (const auto& childGUID : entity.childrenGUIDs) {
            localChildrenGUIDs.push_back(guidToLocal.at(childGUID));
        }
        result += std::to_string(localID.id) + ": " + toString(getEntityID(entity.guid), localParentID, localChildrenGUIDs);
    }

    result += "}";
    return result;
}

void ECS::fromString(EntityGUID guid, std::string str){
    fromString(getEntityID(guid), str);
}
void ECS::fromString(EntityID id, std::string str){
    std::unordered_map<EntityGUID, EntityGUID> localToGuid;
    return fromString(id, str, localToGuid);
}
void ECS::fromString(EntityID id, std::string str, std::unordered_map<EntityGUID, EntityGUID> &localToGuid){
    ECS_WARNING_IF(id.id >= entities->size(), ENTITY_DOESNT_EXIST(std::to_string(id.id)), );

    str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end()); // Trim spaces
    str = str.substr(1, str.size() - 2); // Strip brackets

    std::vector<std::string> tokens = splitTopLevelCommaSections(str);

    Entity &entity = (*entities)[id.id];
    
    EntityGUID parent;
    fromString<EntityGUID>(parent, tokens.at(0).substr(tokens.at(0).find(':') + 1));
    std::vector<EntityGUID> childrenGUIDs;
    fromString<std::vector<EntityGUID>>(childrenGUIDs, tokens.at(1).substr(tokens.at(1).find(':') + 1));

    if(localToGuid.find(parent) != localToGuid.end()){
        parent = localToGuid.at(parent);
    }
    entity.parentGUID = parent;

    for(const auto& childGUID : childrenGUIDs) {
        if(localToGuid.find(childGUID) != localToGuid.end()){
            entity.childrenGUIDs.push_back(localToGuid.at(childGUID));
        }else{
            entity.childrenGUIDs.push_back(childGUID);
        }
    }

    for (size_t i = 2; i < tokens.size(); i++) {
        std::string token = tokens.at(i);
        if (token.empty()) continue;
    
        auto colon = token.find(':');
        if (colon == std::string::npos) continue;
        std::string keyStr = token.substr(0, colon);
        std::string valStr = token.substr(colon + 1);
        keyStr.erase(remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());
        valStr.erase(0, valStr.find_first_not_of(" \t"));

        ComponentTypeID typeID = componentTypeNames.at(keyStr);

        uint8_t* componentPtr = new uint8_t[componentTypes.at(typeID).componentSize];

        componentTypes.at(typeID).fromString((void*)componentPtr, valStr, *this, 0);

        addComponent(id, typeID, (void*)componentPtr);
        
        delete[] componentPtr;
    }
}

void ECS::fromTemplateString(std::string str){
    std::string ogStr = str;
    std::unordered_map<EntityGUID, EntityGUID> localToGuid;

    str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end()); // Trim spaces
    str = str.substr(1, str.size() - 2); // Strip brackets

    std::vector<std::string> tokens = splitTopLevelCommaSections(str);
    for (auto& token : tokens) {
        if (token.empty()) continue;
    
        auto colon = token.find(':');
        if (colon == std::string::npos) continue;
        std::string keyStr = token.substr(0, colon);
        keyStr.erase(remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());

        std::cout << "Key: " << keyStr << std::endl;
        EntityGUID id{std::stoull(keyStr)};

        localToGuid[id] = generateGUID();
    }

    fromString(ogStr, localToGuid);
}

std::string ECS::prettyFormat(const std::string& input) {
    std::string output;
    int indent = 0;
    bool inValue = false;

    auto writeIndent = [&]() {
        output += '\n';
        output += std::string(indent * 2, ' ');
    };

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        switch (c) {
            case '[':
            case '{':
                output += c;
                indent++;
                writeIndent();
                break;
            case '}':
            case ']':
                indent--;
                writeIndent();
                output += c;
                inValue = false;
                break;
            case ',':
                output += c;
                writeIndent();
                inValue = false;
                break;
            case ':':
                output += ": ";
                inValue = true;
                break;
            case ' ':
                // skip unnecessary spaces
                if (!inValue) break;
                [[fallthrough]];
            default:
                output += c;
                break;
        }
    }

    return output;
}

std::string ECS::toString(){
    std::string result = "{";

    for (size_t i = 0; i < entities->size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        Entity &entity = (*entities)[i];
        result += std::to_string(entity.guid.id) + ": " + toString(EntityID{i});
    }
    result += "}";

    return result;
}

void ECS::fromString(std::string str){
    std::unordered_map<EntityGUID, EntityGUID> localToGuid;
    return fromString(str, localToGuid);
}

void ECS::fromString(std::string str, std::unordered_map<EntityGUID, EntityGUID> &localToGuid){
    str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end()); // Trim spaces
    str = str.substr(1, str.size() - 2); // Strip brackets

    std::vector<std::string> tokens = splitTopLevelCommaSections(str);
    for (const auto& token : tokens) {
        if (token.empty()) continue;
    
        auto colon = token.find(':');
        if (colon == std::string::npos) continue;
        std::string keyStr = token.substr(0, colon);
        std::string valStr = token.substr(colon + 1);
        keyStr.erase(remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());
        valStr.erase(0, valStr.find_first_not_of(" \t"));

        std::cout << keyStr << std::endl;
        EntityGUID id{std::stoull(keyStr)};

        if(localToGuid.find(id) != localToGuid.end()){
            id = localToGuid.at(id);
        }

        addEntity(id);
        fromString(getEntityID(id), valStr, localToGuid);
    }
}

std::vector<std::string> ECS::splitTopLevelCommaSections(const std::string& input) {
    std::vector<std::string> sections;
    int depth = 0;
    std::string currentSection;

    for (char c : input) {
        if (c == '{' || c == '[') {
            depth++;
        } else if (c == '}' || c == ']') {
            depth--;
        } else if (c == ',' && depth == 0) {
            sections.push_back(currentSection);
            currentSection.clear();
            continue;
        }
        currentSection += c;
    }

    if (!currentSection.empty()) {
        sections.push_back(currentSection);
    }

    return sections;
}

template <typename T>
void ECS::fromString(void* ptr, std::string str, ECS &ecs, int arraySize) {
    T& value = *static_cast<T*>(ptr);

    if constexpr (std::is_same_v<T, std::string>) {
        str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end()); // Trim spaces
        if (!str.empty() && str.front() == '"' && str.back() == '"') {
            value = str.substr(1, str.size() - 2); // Remove quotes
        } else {
            value = str;
        }
    }
    else if constexpr (std::is_same_v<T, char>) {
        if (!str.empty()) {
            value = (str.front() == '"' && str.size() >= 3) ? str[1] : str[0];
        }
    }
    else if constexpr (std::is_same_v<T, bool>) {
        value = (str == "true");
    }
    else if constexpr (std::is_same_v<T, EntityGUID>) {
        value.id = std::stoull(str);
    }
    else if constexpr (std::is_same_v<T, EntityID>) {
        value.id = std::stoull(str);
    }
    else if constexpr (std::is_arithmetic_v<T>) {
        std::istringstream iss(str);
        iss >> value;
    }
    else if constexpr (std::is_array_v<T>) {
        using Elem = std::remove_extent_t<T>;
        constexpr std::size_t N = std::extent_v<T>;
        str = str.substr(1, str.size() - 2); // Strip brackets

        std::vector<std::string> sections = splitTopLevelCommaSections(str);

        int i = 0;
        for (const auto& section : sections) {
            std::string token = section;
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end()); // Trim spaces
            if (token.empty()) continue;

            if (i >= N) break; // Prevent overflow

            void* elementPtr = reinterpret_cast<uint8_t*>(value) + i * sizeof(Elem);
            fromString<Elem>(elementPtr, token, ecs);
            ++i;
        }
    }
    else if constexpr (is_vector<T>::value) {
        using Elem = typename T::value_type;
        value.clear();

        str = str.substr(1, str.size() - 2);
        std::vector<std::string> sections = splitTopLevelCommaSections(str);

        for(const auto& section : sections) {
            std::string token = section;
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end()); // Trim spaces
            if (token.empty()) continue;

            fromString<Elem>(&value.emplace_back(), token, ecs);
        }
    }
    else if constexpr (is_unordered_map<T>::value) {
        using Key = typename T::key_type;
        using Val = typename T::mapped_type;
        value.clear();

        str = str.substr(1, str.size() - 2);
        std::vector<std::string> sections = splitTopLevelCommaSections(str);

        for (const auto& section : sections) {
            std::string token = section;
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end()); // Trim spaces
            if (token.empty()) continue;

            auto colon = token.find(':');
            if (colon == std::string::npos) continue;
            std::string keyStr = token.substr(0, colon);
            std::string valStr = token.substr(colon + 1);
            keyStr.erase(remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());
            valStr.erase(0, valStr.find_first_not_of(" \t"));

            Key key;
            Val val;
            fromString<Key>(&key, keyStr, ecs);
            fromString<Val>(&val, valStr, ecs);
            value[key] = val;
        }
    }
    else if constexpr (std::is_pointer<T>::value) {
        using Pointee = std::remove_pointer_t<T>;

        if (str == "null") {
            value = nullptr;
        } else {
            if (arraySize == 0) {
                value = new Pointee;
                fromString<Pointee>(value, str, ecs);
            } else {
                value = new Pointee[arraySize];
                str = str.substr(1, str.size() - 2);

                std::vector<std::string> sections = splitTopLevelCommaSections(str);

                if (sections.size() != (size_t)arraySize) {
                    ECS_WARNING_IF(true, "Array size mismatch", );
                }

                int i = 0;
                for (const auto& section : sections) {
                    std::string token = section;
                    token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end()); // Trim spaces
                    if (token.empty()) continue;

                    void* elementPtr = reinterpret_cast<uint8_t*>(value) + i * sizeof(Pointee);
                    fromString<Pointee>(elementPtr, token, ecs);
                    ++i;
                }

            }
        }
    }
    else {
        ComponentTypeID typeID = typeid(T).hash_code();
        auto componentTypeIt = ecs.componentTypes.find(typeID);

        if (componentTypeIt != ecs.componentTypes.end()) {
            str = str.substr(1, str.size() - 2); // Remove brackets

            std::vector<std::string> sections = splitTopLevelCommaSections(str);
            for (const auto& section : sections) {
                auto colon = section.find(':');
                if (colon == std::string::npos) continue;
                std::string keyStr = section.substr(0, colon);
                std::string valStr = section.substr(colon + 1);
                keyStr.erase(remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());
                valStr.erase(0, valStr.find_first_not_of(" \t"));

                MemberMeta memberMeta = ecs.getMemberMeta(keyStr, typeID);

                ECS_WARNING_IF(memberMeta.size == 0, MEMBER_DOESNT_EXIST(keyStr), );

                uint8_t* memberPtr = reinterpret_cast<uint8_t*>(ptr) + memberMeta.offset;
                memberMeta.fromString(memberPtr, valStr, ecs, memberMeta.arraySize);
            }
        } else {
            ECS_WARNING_IF(true, "Unknown type", );
        }
    }
}


} // namespace bbecs