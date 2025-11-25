# 3Dモデルファイル配置場所

このディレクトリに3Dモデルファイル（.fbx, .obj, .gltf等）を配置してください。

## サポートされているファイル形式

- `.fbx` - Autodesk FBX
- `.obj` - Wavefront OBJ  
- `.gltf` / `.glb` - glTF 2.0
- `.dae` - COLLADA
- `.3ds` - 3D Studio Max
- `.blend` - Blender
- `.x` - DirectX
- `.md5` - Doom 3
- `.ply` - Stanford PLY
- `.stl` - Stereolithography
- `.ase` - 3D Studio ASE
- `.ifc` - Industry Foundation Classes
- `.dxf` - AutoCAD DXF

## 使用例

```cpp
// character.fbx を読み込み
auto model = ModelLoader::LoadModel(device, "assets/models/character.fbx");

// サブディレクトリからの読み込み  
auto model = ModelLoader::LoadModel(device, "assets/models/vehicles/car.fbx");
```