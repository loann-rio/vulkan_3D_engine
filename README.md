What if I'm stupid enough to try to make a 3D engine (with vulkan!)?


# Modular C++ Vulkan Game Engine

## Overview

This project is a modular, reusable, and extensible game engine written in C++17, designed for learning, prototyping, and building games or interactive 3D applications. The engine is built around a component-based architecture, leveraging Vulkan for rendering, and is structured to maximize code reuse and flexibility.

## Features

- **Component-based GameObject System**: Easily extendable with custom behaviors.
- **Scene Management**: Save, load, and switch between scenes with JSON serialization.
- **Asynchronous Asset Loading**: Load models and textures in the background.
- **Flexible Rendering Pipeline**: Supports OBJ and GLTF models, terrain, lights, and cameras.
- **ImGui-based UI**: In-engine object and scene management.
- **Extensible Behavior Registration**: Add new behaviors with a single line of code.
- **Modern C++ (C++17)**: Smart pointers, type safety, and STL containers.

## Project Structure

```
src/
  App.cpp, App.h           # Main application loop and setup
  objects/
    GameObject.h/cpp       # Core GameObject and component system
    objectManager.h/cpp    # Scene and object management
    BasicUI.h/cpp          # ImGui-based UI
    ...                    # Camera, Light, Texture, etc.
  model/
    Model.h/cpp            # OBJ model support
    GlTFModel.h/cpp        # GLTF model support
  GameObjectClassAssets/
    TerrainGenerator.h/cpp # Example custom behavior
    ...                    # Add your own behaviors here
  base/
    Device.h/cpp           # Vulkan device abstraction
    Buffer.h/cpp           # Buffer management
    ...                    # Swapchain, descriptors, etc.
  render/
    Camera.h/cpp           # Camera logic
    ...                    # Render systems
```

## Getting Started

### Prerequisites

- C++17 compatible compiler (Visual Studio 2019/2022, GCC 9+, Clang 10+)
- Vulkan SDK
- [GLM](https://github.com/g-truc/glm) (math library)
- [ImGui](https://github.com/ocornut/imgui) (included)
- [nlohmann/json](https://github.com/nlohmann/json) (included)
- CMake (recommended for building)

### Building

1. Clone the repository and initialize submodules if needed.
2. Install the Vulkan SDK and ensure your environment variables are set.
3. Open the project in Visual Studio and make.
4. Run the executable. The engine will create a default scene if none exists.

### Running

- The main window will open with a default camera and UI.
- Use the ImGui interface to create, select, and manage objects and scenes.
- Camera movement is controlled via keyboard (see `KeyboardMovementController`).

## Core Concepts

### GameObject and Component System

- **GameObject**: Represents any entity in the scene (model, camera, light, etc.).
- **Behaviors**: Attach custom logic to GameObjects by creating a class derived from `GameObjectBehavior`.
- **Registration**: Register new behaviors with a single line using the `REGISTER_BEHAVIOR` macro.

#### Example: Adding a Custom Behavior

``` cpp
// MyBehavior.h
class MyBehavior : public GameObjectBehavior {
public:
    void setup(Device& device, ObjectManager* objManager, GameObject* object) override { /* ... */ }
    void loop(Device& device, ObjectManager* objManager, GameObject* object) override { /* ... */ }
};

// MyBehavior.cpp
REGISTER_BEHAVIOR(MyBehavior)
```

### Scene Management

- Scenes are saved and loaded as JSON files in the `scenes/` directory.
- Each object’s transform, type, and attached behaviors are serialized.
- Switching scenes preserves the main camera and loads new objects asynchronously.

### Rendering

- Supports both OBJ and GLTF models.
- Multiple render systems for models, terrain, and lights.
- Descriptor sets and uniform buffers are managed per-frame for performance.

### UI

- ImGui-based editor for object and scene management.
- Create, select, and modify objects at runtime.
- Change scenes and inspect object properties live.

## Extending the Engine

- **Add new GameObject types**: Inherit from `GameObject` and register with the factory.
- **Add new behaviors**: Inherit from `GameObjectBehavior`, implement `setup` and `loop`, and register.
- **Add new render systems**: Implement a new render system and add it in `App::createRenderSystems`.

## Example: Creating a game object and Attaching a Behavior


``` cpp
std::shared_ptr<Model> cube = Model::createModelFromFile(device, "model/myModel.obj", "textures/myTexture.jpg"); 

auto myObject = GameObjectFactory::createGameObject<GameObject>(device);
myObject->setAttachedClass(std::make_unique<MyBehavior>(device));
myObject->setName("myObject");
myObject->setModel(model);
myObject->createDescriptorSet(pool);
objectManager.pushGameObject(std::move(myObject));
```

## Saving and Loading

- Call `objectManager.saveFullScene()` to save the current scene.
- Call `objectManager.switchScene("sceneName")` to save and load a new scene.

## Contributing

- Fork the repository and create a feature branch.
- Add new modules or behaviors in a modular fashion.
- Submit a pull request with a clear description of your changes.

## License

This project is licensed under the MIT License.

## Acknowledgements

- [Vulkan](https://www.vulkan.org/)
- [GLM](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/imgui)
- [nlohmann/json](https://github.com/nlohmann/json)

---

**Note:**  
For more details on extending or using the engine, see the code comments and the `objects/` and `GameObjectClassAssets/` directories for examples.
