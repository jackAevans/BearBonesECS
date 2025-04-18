#include <vector>
#include <unordered_map>
#include <random>
#include <iostream>
#include <functional>
#include <tuple>
#include <thread>
#include <unordered_set>

// comment this line to disable warning messages
#define ECS_DEBUG

#ifdef ECS_DEBUG
#define ECS_WARNING_IF(condition, message, retval) do { if (warnIf(condition, message, __func__)) return retval; } while (0)
#else
#define ECS_WARNING_IF(condition, message, retval)
#endif

#define ECS_ERROR_IF(condition, message) errorIf(condition, message, __func__)

#define COMPONENT_TYPE_DOESNT_EXIST         "Component type doesn't exist"
#define COMPONENT_TYPE_ALREADY_EXISTS       "Component type already exists"
#define COMPONENT_TYPE_IS_LOCKED            "Component type is locked"
#define COMPONENT_TYPE_IS_READ_ONLY         "Component type is read-only"
#define ENTITY_DOESNT_EXIST                 "Entity doesn't exist"
#define ENTITY_DOESNT_CONTAIN_COMPONENT     "Entity doesn't contain component"
#define ENTITY_ALREADY_CONTAINS_COMPONENT   "Entity already contains component"
#define ENTITY_ALREADY_EXISTS               "Entity already exists"
#define ENTITY_GUID_ALREADY_EXISTS          "Entity GUID already exists"
#define ENTITY_GUID_DOESNT_EXIST            "Entity GUID doesn't exist"
#define ECS_IS_RESTRICTED                   "ECS is restricted"
#define SYSTEM_BATCH_DOESNT_EXIST           "System batch doesn't exist"
#define MEMBER_DOESNT_EXIST                 "Member doesn't exist"

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

struct MemberMeta {
    size_t offset;
    size_t size;
    bool isPointer;
    int arraySize;

    std::string (*toString)(void*, ECS&, int);
};

using SystemBatchID = uint64_t;
using TypeID = size_t;

class ECS {
    using ComponentID = size_t;

    template <typename T>
    struct Component {
        T data;
        EntityID owner;
    };

    struct Entity {
        EntityGUID guid;
        std::unordered_map<TypeID, ComponentID> componentIDs;
    };

    struct ComponentType {
        void *storage;
        size_t size;
        size_t componentSize;
        size_t capacity;
        float growthFactor = 1.5f;
        bool isLocked = false;
        bool isReadOnly = false;

        std::string name;
        std::unordered_map<std::string, MemberMeta> members;
        
        std::string (*toString)(void*, ECS&, int);
        void (*removeComponentTypeFunc)(ECS &ecs);
        void (*removeComponentFunc)(EntityID id, ECS &ecs);
    };

    struct System{
        std::vector<TypeID> componentTypeIDs;
        std::function<void(ECS&)> func;
    };

    struct SystemBatch {
        std::vector<std::vector<System>> parallelSystems;
    };

public:
    ECS();
    ~ECS();
    std::string toString(); 
    static std::string prettyFormat(const std::string& input);

    // Entity management
    ECS &addEntity();
    ECS &addEntity(EntityGUID &guid);
    ECS &removeEntity(EntityGUID entityId);
    ECS &removeEntity(EntityID entityId);
    const EntityID getEntityID(EntityGUID guid) const;
    std::string toString(EntityGUID guid);
    std::string toString(EntityID guid);

    // Component management
    template <typename T, typename... Args> ECS &addComponent(Args&&... args);
    template <typename T, typename... Args> ECS &addComponent(EntityGUID entityId, Args&&... args);
    template <typename T, typename... Args> ECS &addComponent(EntityID entityId, Args&&... args);
    template <typename T> ECS &removeComponent(EntityGUID entityID);
    template <typename T> ECS &removeComponent(EntityID entityID);

    // Component access
    void* getComponent(EntityID entityId, TypeID typeID);
    template <typename T> T &getComponent(EntityGUID entityId);
    template <typename T> T &getComponent(EntityID entityId);
    template <typename T> const T &readComponent(EntityGUID entityId) const;
    template <typename T> const T &readComponent(EntityID entityId) const;
    template <typename T> ECS &setReadOnly();
    template <typename T> ECS &setReadWrite();
    template <typename T> std::string toString(T &t, int arraySize = 0);
    template <typename U, typename T> std::string toString(T &t, std::string memberName);
    
    // Component type management
    template <typename T> ECS &addComponentType(size_t reserve = 10);
    template <typename T> ECS &addComponentType(std::string name, size_t reserve = 10);
    template <typename T> ECS &removeComponentType();
    template <typename T> std::string getName();
    template <typename T> TypeID getTypeID();
    template <typename T, typename MemberType>
    ECS &addMemberMeta(MemberType T::*memberPtr, std::string name, int arraySize_ = 0, std::string (*toString_)(void*, ECS&, int) = nullptr);
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
    template <typename... Components> ECS &addSystem(SystemBatchID batchID, std::function<void(ECS&)> func);
    ECS &runSystemBatch(SystemBatchID id);

private:
    template<typename... Components> std::tuple<Components&...> getComponents(EntityID entityId);
    std::vector<TypeID> getAllComponentTypeIDs();
    std::vector<TypeID> getParallelSystemComponentIDs(SystemBatchID id, size_t index);
    ECS &killChildren();
    template <typename... Args> ECS &split();
    ECS& split(std::vector<TypeID> componentTypesToLock);
    ECS &terminate();
    ECS &restrict();
    ECS &unrestrict();
    static EntityGUID generateGUID();
    static SystemBatchID generateSystemBatchID();
    template <typename T> static void refitComponentTypeStorage(ComponentType& componentType, float growthFactor);
    static bool haveCommonElements(const std::vector<TypeID>& vec1, const std::vector<TypeID>& vec2);
    static bool warnIf(bool condition, const std::string& message, const char* func);
    static bool errorIf(bool condition, const std::string& message, const char* func);
    template <typename T> static void removeComponentType_(ECS &ecs);
    template <typename T> static void removeComponent_(EntityID id, ECS &ecs);

    template <typename T>
    static std::string toString(void* data, ECS &ecs, int arraySize = 1);

    std::unordered_map<EntityGUID, EntityID> *entitiesMap; 
    std::vector<Entity> *entities;
    std::unordered_map<TypeID, ComponentType> componentTypes; 
    EntityID cachedEntityID = {SIZE_MAX};

    std::unordered_map<SystemBatchID, SystemBatch> systemBatches;

    bool restricted = false;
    bool isRoot = true;
    std::vector<ECS> children;

    ECS(std::unordered_map<EntityGUID, EntityID> *entitiesMap, std::vector<Entity> *entities,
        std::unordered_map<TypeID, ComponentType> componentTypes, bool restricted, bool isRoot = false);
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
                    std::unordered_map<TypeID, ComponentType> componentTypes, bool restricted, bool isRoot) {
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
    std::vector<TypeID> componentTypesToLock = {typeid(Args).hash_code()...};
    return split(componentTypesToLock);
}

ECS& ECS::split(std::vector<TypeID> componentTypesToLock){
    std::unordered_map<TypeID, ComponentType> newComponentTypes;
    
    for(size_t i = 0; i < componentTypesToLock.size(); i++){
        TypeID typeID = componentTypesToLock[i];

        auto componentTypeIt = componentTypes.find(typeID);
        ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);
        
        ComponentType &componentType = componentTypeIt->second;
        ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);
        
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
    ECS_WARNING_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST, *this);

    std::vector<TypeID> componentsToRemove;

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

const EntityID ECS::getEntityID(EntityGUID guid) const{
    auto entityIt = entitiesMap->find(guid);
    ECS_WARNING_IF(entityIt == entitiesMap->end(), ENTITY_GUID_DOESNT_EXIST, EntityID{SIZE_MAX});

    return entityIt->second;
}

template <typename T, typename... Args>
ECS &ECS::addComponent(Args&&... args) {
    addComponent<T>(cachedEntityID, std::forward<Args>(args)...);
    return *this;
}

template <typename T, typename... Args>
ECS &ECS::addComponent(EntityGUID entityId, Args&&... args) {
    addComponent<T>(getEntityID(entityId), std::forward<Args>(args)...);
    return *this;
}

template <typename T, typename... Args>
ECS &ECS::addComponent(EntityID entityId, Args&&... args) {
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;

    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

    ECS_WARNING_IF((*entities).at(entityId.id).componentIDs.find(typeID) != (*entities).at(entityId.id).componentIDs.end(), ENTITY_ALREADY_CONTAINS_COMPONENT, *this);

    if (componentType.size >= componentType.capacity) {
        refitComponentTypeStorage<T>(componentType, componentType.growthFactor);
    }

    Component<T>* componentStorage = static_cast<Component<T>*>(componentType.storage);
    new (&componentStorage[componentType.size]) Component<T>{T{std::forward<Args>(args)...}, entityId}; 

    (*entities).at(entityId.id).componentIDs[typeID] = componentType.size;
    componentType.size++;

    return *this;
}

template <typename T>
ECS &ECS::removeComponent(EntityGUID entityID) {
    removeComponent<T>(getEntityID(entityID));
    return *this;
}

template <typename T>
ECS &ECS::removeComponent(EntityID entityID) {
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    auto componentIndexIt = (*entities).at(entityID.id).componentIDs.find(typeID);
    ECS_WARNING_IF(componentIndexIt == (*entities).at(entityID.id).componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT, *this);

    ComponentType& componentType = componentTypeIt->second;
    ComponentID componentID = componentIndexIt->second;

    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

    Component<T>* componentStorage = static_cast<Component<T>*>(componentType.storage);

    componentStorage->data.~T();

    EntityID lastEntityID = componentStorage[componentType.size - 1].owner;
    (*entities).at(lastEntityID.id).componentIDs.at(typeID) = componentID;

    componentStorage[componentID] = componentStorage[componentType.size - 1];
    componentType.size--;

    if (componentType.size < componentType.capacity / componentType.growthFactor) {
        refitComponentTypeStorage<T>(componentType, 1.0f / componentType.growthFactor);
    }

    (*entities).at(entityID.id).componentIDs.erase(typeID);

    return *this;
}

template<typename... Components>
std::tuple<Components&...> ECS::getComponents(EntityID entityId) {
    return std::tuple<Components&...>{getComponent<Components>(entityId)...};
}

template <typename T>
T &ECS::getComponent(EntityGUID entityId) {
    return getComponent<T>(getEntityID(entityId));
}

template <typename T>
T &ECS::getComponent(EntityID entityId) {
    TypeID typeID = typeid(T).hash_code();

    void* componentPtr = getComponent(entityId, typeID);

    Component<T>* componentPtrCasted = static_cast<Component<T>*>(componentPtr);

    return componentPtrCasted->data;
}

void* ECS::getComponent(EntityID entityId, TypeID typeID){
    ECS_ERROR_IF(entityId.id > entities->size(), ENTITY_DOESNT_EXIST);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST);

    auto componentIndexIt = (*entities).at(entityId.id).componentIDs.find(typeID);
    ECS_ERROR_IF(componentIndexIt == (*entities).at(entityId.id).componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT);

    ComponentType& componentType = componentTypeIt->second;
    ComponentID componentID = componentIndexIt->second;

    ECS_ERROR_IF(componentType.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY);
    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED);

    char* componentStorage = static_cast<char*>(componentType.storage);
    char* componentPtr = componentStorage + (componentID * componentType.componentSize);

    return (void*)componentPtr;
}

template <typename T> const T &ECS::readComponent(EntityGUID entityId) const{
    return readComponent<T>(getEntityID(entityId));
}
template <typename T> const T &ECS::readComponent(EntityID entityId) const{
    TypeID typeID = typeid(T).hash_code();

    ECS_ERROR_IF(entityId.id >= entities->size(), ENTITY_DOESNT_EXIST);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_ERROR_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST);

    const auto& entity = (*entities)[entityId.id];
    auto componentIndexIt = entity.componentIDs.find(typeID);
    ECS_ERROR_IF(componentIndexIt == entity.componentIDs.end(), ENTITY_DOESNT_CONTAIN_COMPONENT);

    const ComponentType& componentType = componentTypeIt->second;
    ComponentID componentID = componentIndexIt->second;

    ECS_ERROR_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED);

    const Component<T>* componentStorage = static_cast<const Component<T>*>(componentType.storage);
    return componentStorage[componentID].data;
}

template <typename T> ECS &ECS::setReadOnly(){
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

    componentType.isReadOnly = true;

    return *this;
}

template <typename T> ECS &ECS::setReadWrite(){
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

    componentType.isReadOnly = false;

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

template <typename T> ECS &ECS::addComponentType(size_t reserve){
    return addComponentType<T>(typeid(T).name(), reserve);
}

template <typename T>
ECS &ECS::addComponentType(std::string name, size_t reserve) {
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    ECS_WARNING_IF(componentTypes.find(typeID) != componentTypes.end(), COMPONENT_TYPE_ALREADY_EXISTS, *this);

    void* storage = new uint8_t[reserve * sizeof(Component<T>)];

    componentTypes[typeID] = {
        .storage = storage,
        .componentSize = sizeof(Component<T>),
        .size = 0,
        .capacity = reserve,
        .removeComponentTypeFunc = removeComponentType_<T>,
        .removeComponentFunc = removeComponent_<T>,
        .toString = toString<T>,
        .name = name,
    };

    return *this;
}

template <typename T>
ECS &ECS::removeComponentType() {
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;
    delete[] static_cast<uint8_t*>(componentType.storage);
    componentType.storage = nullptr;

    componentTypes.erase(typeID);
    return *this;
}

template <typename T> std::string ECS::getName(){
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, "");

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, "");

    ComponentType& componentType = componentTypeIt->second;

    return componentType.name;
}

template <typename T> TypeID ECS::getTypeID(){
    TypeID typeID = typeid(T).hash_code();

    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, 0);

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, 0);

    return typeID;
}

template <typename T, typename MemberType>
ECS &ECS::addMemberMeta(MemberType T::*memberPtr, std::string name, int arraySize_, std::string (*toString_)(void*, ECS&, int)) {
    ECS_WARNING_IF(restricted, ECS_IS_RESTRICTED, *this);

    TypeID componentTypeID = typeid(T).hash_code();
    auto componentTypeIt = componentTypes.find(componentTypeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;

    MemberMeta member{
        .offset = reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*memberPtr)),
        .size = sizeof(MemberType),
        .isPointer = std::is_pointer<MemberType>::value,
        .arraySize = arraySize_,
    };

    if(toString_ == nullptr){
        member.toString = toString<MemberType>;
    }else{
        member.toString = toString_;
    }

    componentType.members.insert({name, member});

    return *this;
}

template <typename T> MemberMeta ECS::getMemberMeta(std::string name){
    TypeID typeID = typeid(T).hash_code();

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, MemberMeta{});

    ComponentType& componentType = componentTypeIt->second;

    auto memberIt = componentType.members.find(name);
    ECS_WARNING_IF(memberIt == componentType.members.end(), MEMBER_DOESNT_EXIST, MemberMeta{});

    return memberIt->second;
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
    TypeID typeID = typeid(T).hash_code();

    auto componentTypeIt = componentTypes.find(typeID);
    ECS_WARNING_IF(componentTypeIt == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType = componentTypeIt->second;

    ECS_WARNING_IF(componentType.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY, *this);
    ECS_WARNING_IF(componentType.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

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
    TypeID typeID1 = typeid(Component1).hash_code();

    auto componentTypeIt1 = componentTypes.find(typeID1);
    ECS_WARNING_IF(componentTypeIt1 == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    TypeID typeID2 = typeid(Component2).hash_code();

    ECS_WARNING_IF(componentTypes.find(typeID2) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);

    ComponentType& componentType1 = componentTypeIt1->second;

    ECS_WARNING_IF(componentType1.isReadOnly || componentTypes.find(typeID2)->second.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY, *this);
    ECS_WARNING_IF(componentType1.isLocked || componentTypes.find(typeID2)->second.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);

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

    std::vector<TypeID> componentTypesToIterate = {typeid(Components).hash_code()...};

    for (size_t i = 0; i < componentTypesToIterate.size(); i++) {
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i)) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i))->second.isReadOnly, COMPONENT_TYPE_IS_READ_ONLY, *this);
        ECS_WARNING_IF(componentTypes.find(componentTypesToIterate.at(i))->second.isLocked, COMPONENT_TYPE_IS_LOCKED, *this);
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
                TypeID typeID = componentTypesToIterate.at(j);
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
                    TypeID typeID = componentTypesToIterate.at(k);
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
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST, *this);

    SystemBatch& systemBatch = systemBatchIt->second;

    std::vector<TypeID> componentTypeIDs = {typeid(Components).hash_code()...};

    if(componentTypeIDs.size() == 0){
        componentTypeIDs = getAllComponentTypeIDs();
    }

    for(size_t i = 0; i < componentTypeIDs.size(); i++){
        ECS_WARNING_IF(componentTypes.find(componentTypeIDs[i]) == componentTypes.end(), COMPONENT_TYPE_DOESNT_EXIST, *this);
    }

    for(size_t j = 0; j < systemBatch.parallelSystems.size(); j++){
        std::vector<TypeID> parallelSystemComponentIDs = getParallelSystemComponentIDs(batchID, j);

        if(!haveCommonElements(parallelSystemComponentIDs, componentTypeIDs)){
            systemBatch.parallelSystems.at(j).push_back({componentTypeIDs, func});

            return *this;
        }
    }

    systemBatch.parallelSystems.push_back({{componentTypeIDs, func}});

    return *this;
}

ECS &ECS::runSystemBatch(SystemBatchID id){
    auto systemBatchIt = systemBatches.find(id);
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST, *this);

    SystemBatch &systemBatch = systemBatchIt->second;

    for(size_t i = 0; i < systemBatch.parallelSystems.size(); i++){
        std::vector<System> &parallelSystem = systemBatch.parallelSystems.at(i);

        std::vector<std::thread> threads;
        std::vector<ECS> ecss;

        if(parallelSystem.size() == 1){
            parallelSystem.at(0).func(*this);
            continue;
        }

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

std::vector<TypeID> ECS::getAllComponentTypeIDs() {
    std::vector<TypeID> componentTypeIDs;

    for (const auto& componentTypeID : componentTypes) {
        componentTypeIDs.push_back(componentTypeID.first);
    }

    return componentTypeIDs;
}

std::vector<TypeID> ECS::getParallelSystemComponentIDs(SystemBatchID id, size_t index){
    std::vector<TypeID> componentTypeIDs;

    auto systemBatchIt = systemBatches.find(id);
    ECS_WARNING_IF(systemBatchIt == systemBatches.end(), SYSTEM_BATCH_DOESNT_EXIST, componentTypeIDs);

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

bool ECS::haveCommonElements(const std::vector<TypeID>& vec1, const std::vector<TypeID>& vec2) {
    for (TypeID id1 : vec1) {
        for (TypeID id2 : vec2) {
            if (id1 == id2) {
                return true; // Found a common element
            }
        }
    }

    return false; // No common elements
}

template <typename T>
void ECS::refitComponentTypeStorage(ComponentType& componentType, float growthFactor) {
    size_t newCapacity = std::ceil(componentType.capacity * growthFactor);
    void* newStorage = new uint8_t[newCapacity * sizeof(Component<T>)];

    memcpy(newStorage, componentType.storage, sizeof(Component<T>) * componentType.size);

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

    std::vector<TypeID> componentTypesToRemove;

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
        std::cerr << "ECS WARNING: " << message;
        std::cerr << " '" << func << "'" << std::endl;
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

    TypeID typeID = typeid(T).hash_code();
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
        return std::to_string(value);
    }
    else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
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
        std::string result;
        for (size_t i = 0; i < value.size(); ++i) {
            if (i > 0) result += " ";
            result += toString<typename T::value_type>((void*)&value[i], ecs);
        }
        return result;
    }
    else if constexpr (is_unordered_map<T>::value) {
        std::string result = "[";
        bool first = true;
        for (const auto& pair : value) {
            if (!first) result += " ";
            first = false;
            result += toString<typename T::key_type>((void*)&pair.first, ecs);
            result += ": ";
            result += toString<typename T::mapped_type>((void*)&pair.second, ecs);
        }
        return result += "]";
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
        std::string result = "[";

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
        result += "]";
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
    ECS_ERROR_IF(entityID.id >= entities->size(), ENTITY_DOESNT_EXIST);

    Entity &entity = (*entities)[entityID.id];

    std::string result = "[";

    bool first = true;

    for(auto &componentID : entity.componentIDs){
        if(!first){
            result += ", ";
        }
        first = false;
        TypeID typeID = componentID.first;
        void* componentPtr = getComponent(entityID, typeID);

        ComponentType &componentType = componentTypes.at(typeID);

        std::string componentString = componentType.toString(componentPtr, *this, 0);
        result += componentType.name + ": " + componentString;
    }

    return result += "]";
}

std::string ECS::prettyFormat(const std::string& input) {
    std::string output;
    int indent = 0;
    bool newLine = false;
    bool inValue = false;

    auto writeIndent = [&]() {
        output += '\n';
        output += std::string(indent * 2, ' ');
    };

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        switch (c) {
            case '[':
                output += c;
                indent++;
                writeIndent();
                break;
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
    std::string result = "[";

    for (size_t i = 0; i < entities->size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        Entity &entity = (*entities)[i];
        result += std::to_string(entity.guid.id) + ": " + toString(EntityID{i});
    }
    result += "]";


    return result;
}


} // namespace bbecs
