#pragma once
#include <memory>

class TextureArray;
class VoxelManager;

namespace NuRenderer
{
  TextureArray* GetBlockTextures();

  void Init();
  void CompileShaders();
  void Clear();
  void DrawAll(VoxelManager& vm);

  void drawChunks(VoxelManager& vm);
  void drawChunksWater();

  // generic drawing functions
  void drawAxisIndicators();
  void drawQuad();
  void DrawCube();

  inline int drawCalls = 0;

  struct Settings
  {
    bool gammaCorrection = true;
    float fogStart = 500.f;
    float fogEnd = 3000.f;
  }inline settings;
}